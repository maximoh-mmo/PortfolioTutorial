#include "MHSessionTokenService.h"

#include "Misc/Base64.h"
#include "Misc/ConfigCacheIni.h"
#include "String/LexFromString.h"
#include "Containers/StringConv.h"

THIRD_PARTY_INCLUDES_START
#include <openssl/hmac.h>
#include <openssl/crypto.h>
THIRD_PARTY_INCLUDES_END

namespace
{
    constexpr int32 MaxTokenLength = 16384;
    constexpr int32 MaxClaimLength = 1024;

    bool IsValidConfig(const FMHSessionTokenConfig& Config)
    {
        return !Config.Secret.IsEmpty() && Config.LifetimeSeconds > 0;
    }

    bool IsValidClaim(const FString& Value)
    {
        if (Value.IsEmpty() || Value.Len() > MaxClaimLength) { return false; }
        for (TCHAR Character : Value)
        {
            if (Character == TEXT('|') || Character < 32) { return false; }
        }
        return true;
    }

    TArray<uint8> Utf8(const FString& Text)
    {
        FTCHARToUTF8 Converted(*Text);
        TArray<uint8> Bytes;
        Bytes.Append(reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
        return Bytes;
    }

    bool Sign(const FString& Secret, const TArray<uint8>& Payload, TArray<uint8>& Signature)
    {
        const auto Key = Utf8(Secret);
        Signature.SetNumUninitialized(32);
        unsigned int Length = 0;
        return HMAC(EVP_sha256(), Key.GetData(), Key.Num(), Payload.GetData(), Payload.Num(),
            Signature.GetData(), &Length) != nullptr && Length == 32;
    }
}

FMHSessionTokenService::FMHSessionTokenService(FMHSessionTokenConfig InConfig)
    : Config(MoveTemp(InConfig)) {}

TMHResult<FMHSessionTokenConfig> FMHSessionTokenService::LoadConfig(const FString& IniFilename)
{
    FMHSessionTokenConfig Loaded;
    const FString ConfigPath = FConfigCacheIni::NormalizeConfigIniPath(IniFilename);
    if (GConfig)
    {
        GConfig->GetString(TEXT("MH.Auth"), TEXT("Secret"), Loaded.Secret, ConfigPath);
        GConfig->GetInt(TEXT("MH.Auth"), TEXT("LifetimeSeconds"), Loaded.LifetimeSeconds, ConfigPath);
    }
    if (!IsValidConfig(Loaded))
    {
        return TMHResult<FMHSessionTokenConfig>::Failure({EMHErrorCode::NotConfigured,
            TEXT("Session signing requires a secret and a positive lifetime")});
    }
    return TMHResult<FMHSessionTokenConfig>::Success(MoveTemp(Loaded));
}

TMHResult<FString> FMHSessionTokenService::Issue(const FString& Subject, const FString& Provider,
    const FString& Context, int64 NowUnix) const
{
    if (!IsValidConfig(Config))
    {
        return TMHResult<FString>::Failure({EMHErrorCode::NotConfigured, TEXT("Session signing is not configured")});
    }
    if (!IsValidClaim(Subject) || !IsValidClaim(Provider) || !IsValidClaim(Context) ||
        NowUnix < 0 || NowUnix > MAX_int64 - Config.LifetimeSeconds)
    {
        return TMHResult<FString>::Failure({EMHErrorCode::InvalidArgument, TEXT("Invalid session claims or timestamp")});
    }
    const FString PayloadText = FString::Printf(TEXT("%s|%s|%s|%lld"),
        *Subject, *Provider, *Context, NowUnix + Config.LifetimeSeconds);
    const auto Payload = Utf8(PayloadText);
    TArray<uint8> Signature;
    if (!Sign(Config.Secret, Payload, Signature))
    {
        return TMHResult<FString>::Failure({EMHErrorCode::Internal, TEXT("Session signing failed")});
    }
    return TMHResult<FString>::Success(FBase64::Encode(Payload) + TEXT(".") + FBase64::Encode(Signature));
}

TMHResult<FMHSessionClaims> FMHSessionTokenService::Validate(const FString& Token, int64 NowUnix) const
{
    using FResult = TMHResult<FMHSessionClaims>;
    auto Invalid = []() { return FResult::Failure({EMHErrorCode::Unauthorized, TEXT("Invalid or expired session token")}); };
    if (!IsValidConfig(Config))
    {
        return FResult::Failure({EMHErrorCode::NotConfigured, TEXT("Session signing is not configured")});
    }
    if (NowUnix < 0 || Token.Len() > MaxTokenLength) { return Invalid(); }
    FString PayloadBase64, SignatureBase64;
    TArray<uint8> Payload, ActualSignature, ExpectedSignature;
    if (!Token.Split(TEXT("."), &PayloadBase64, &SignatureBase64) ||
        !FBase64::Decode(PayloadBase64, Payload) || !FBase64::Decode(SignatureBase64, ActualSignature) ||
        Payload.IsEmpty() || ActualSignature.Num() != 32)
    {
        return Invalid();
    }
    if (!Sign(Config.Secret, Payload, ExpectedSignature))
    {
        return FResult::Failure({EMHErrorCode::Internal, TEXT("Session verification failed")});
    }
    if (CRYPTO_memcmp(ActualSignature.GetData(), ExpectedSignature.GetData(), 32) != 0) { return Invalid(); }

    // Reject embedded NUL and invalid/non-canonical UTF-8 before interpreting claims.
    if (Payload.Contains(0)) { return Invalid(); }
    const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Payload.GetData()), Payload.Num());
    const FString PayloadText(Converted.Length(), Converted.Get());
    if (Utf8(PayloadText) != Payload) { return Invalid(); }
    TArray<FString> Parts;
    PayloadText.ParseIntoArray(Parts, TEXT("|"), false);
    if (Parts.Num() != 4 || !IsValidClaim(Parts[0]) || !IsValidClaim(Parts[1]) || !IsValidClaim(Parts[2]))
    {
        return Invalid();
    }
    int64 Expiry = 0;
    if (!LexTryParseString(Expiry, *Parts[3]) || Expiry < NowUnix ||
        FString::Printf(TEXT("%lld"), Expiry) != Parts[3]) { return Invalid(); }
    return FResult::Success({MoveTemp(Parts[0]), MoveTemp(Parts[1]), MoveTemp(Parts[2]), Expiry});
}
