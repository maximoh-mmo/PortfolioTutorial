#include "MHSessionTokenService.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHSessionTokenTest, "MH.Backend.SessionTokens",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHSessionTokenTest::RunTest(const FString& Parameters)
{
    const FMHSessionTokenService Service({TEXT("test-secret-for-mh-framework"), 300});
    // Independent .NET HMACSHA256 fixture using the existing Onset four-field wire format.
    const FString Expected = TEXT("NzY1NjExOTgwMDAwMDAwMDB8U3RlYW18MnwxNzAwMDAwMzAw.Qcyv/PLZNvAA86kSY2Tmh+WI3as8+9WW8rugGPbLLbY=");
    const auto Issued = Service.Issue(TEXT("76561198000000000"), TEXT("Steam"), TEXT("2"), 1700000000);
    if (!TestTrue(TEXT("Issue succeeds"), Issued.IsSuccess())) { return false; }
    TestEqual(TEXT("Legacy wire format matches independent HMAC fixture"), *Issued.GetValue(), Expected);
    const auto Valid = Service.Validate(Expected, 1700000001);
    if (!TestTrue(TEXT("Fixture validates"), Valid.IsSuccess())) { return false; }
    TestEqual(TEXT("Subject preserved"), Valid.GetValue()->Subject, FString(TEXT("76561198000000000")));
    TestEqual(TEXT("Provider preserved"), Valid.GetValue()->Provider, FString(TEXT("Steam")));
    TestEqual(TEXT("Opaque context preserved"), Valid.GetValue()->Context, FString(TEXT("2")));
    TestEqual(TEXT("Expiry preserved"), Valid.GetValue()->ExpiryUnix, int64(1700000300));
    TestTrue(TEXT("Existing expiry boundary preserved"), Service.Validate(Expected, 1700000300).IsSuccess());
    TestFalse(TEXT("Expired rejected"), Service.Validate(Expected, 1700000301).IsSuccess());
    FString Tampered = Expected;
    Tampered[0] = TEXT('A');
    TestFalse(TEXT("Tampered payload rejected"), Service.Validate(Tampered, 1700000000).IsSuccess());
    Tampered = Expected;
    Tampered[Tampered.Len() - 2] = TEXT('A');
    TestFalse(TEXT("Tampered signature rejected"), Service.Validate(Tampered, 1700000000).IsSuccess());
    TestFalse(TEXT("Wrong key rejected"), FMHSessionTokenService({TEXT("different-secret"), 300}).Validate(Expected, 1700000000).IsSuccess());
    TestFalse(TEXT("Malformed rejected"), Service.Validate(TEXT("garbage"), 1700000000).IsSuccess());
    TestFalse(TEXT("Empty signature rejected"), Service.Validate(TEXT("YQ==."), 1700000000).IsSuccess());
    TestFalse(TEXT("Oversized input rejected"), Service.Validate(FString::ChrN(16385, TEXT('A')), 1700000000).IsSuccess());
    TestFalse(TEXT("Delimiter injection rejected"), Service.Issue(TEXT("a|b"), TEXT("Steam"), TEXT("2"), 0).IsSuccess());
    TestFalse(TEXT("Empty claim rejected"), Service.Issue(TEXT(""), TEXT("Steam"), TEXT("2"), 0).IsSuccess());
    TestFalse(TEXT("Overflow rejected"), Service.Issue(TEXT("a"), TEXT("Steam"), TEXT("2"), MAX_int64).IsSuccess());
    TestFalse(TEXT("Negative clock rejected"), Service.Validate(Expected, -1).IsSuccess());
    TestFalse(TEXT("Empty config fails closed"), FMHSessionTokenService({}).Issue(TEXT("a"), TEXT("Steam"), TEXT("2"), 0).IsSuccess());
    TestFalse(TEXT("Empty validation secret fails closed"), FMHSessionTokenService({}).Validate(Expected, 0).IsSuccess());
    TestFalse(TEXT("Nonpositive lifetime rejected"), FMHSessionTokenService({TEXT("secret"), 0}).Issue(TEXT("a"), TEXT("Steam"), TEXT("2"), 0).IsSuccess());
    const auto Unicode = Service.Issue(TEXT("\u00fcser"), TEXT("Steam"), TEXT("opaque-context"), 0);
    TestTrue(TEXT("Unicode round trip"), Unicode.IsSuccess() && Service.Validate(*Unicode.GetValue(), 1).IsSuccess());
    const FMHSessionTokenService LongKey({FString::ChrN(128, TEXT('k')), 300});
    const auto LongKeyToken = LongKey.Issue(TEXT("user"), TEXT("Steam"), TEXT("0"), 0);
    TestTrue(TEXT("Long signing key"), LongKeyToken.IsSuccess() && LongKey.Validate(*LongKeyToken.GetValue(), 1).IsSuccess());
    // Signed malformed fixtures exercise parsing after successful signature verification.
    const TCHAR* MalformedFixtures[] = {
        TEXT("dXNlcnxTdGVhbXwyfDE3MDAwMDAzMDBqdW5r.8cnx3JwkGVckIZ8H0r4rQhbY3YZSUcBgy3kL6qUnvsA="),
        TEXT("dXNlcnxTdGVhbXx8MTcwMDAwMDMwMA==.hgo2Sxs9Gg09fPWi24rK15Nkkno0ZXvQpqlH67FyJAQ="),
        TEXT("dXNlcnxTdGVhbXwyfDE3MDAwMDAzMDB8ZXh0cmE=.7oZw6qtUzo6cXfIpRlCqSnFjmEqJ4ZXWMkaH0XBPZAU="),
        TEXT("dXNlcnxTdGVhbXwyfDk5OTk5OTk5OTk5OTk5OTk5OTk5OTk5OTk=.0kxjmO1e8RFoi0+z1uaTE0BmC5H5rzPJAkPcuoeunno="),
        TEXT("dXNlcgBoaWRkZW58U3RlYW18MnwxNzAwMDAwMzAw.uoOFgvVz46zyU+f76/sQ2Rt8emfRKJbcnKMON8dUtxQ=")
    };
    for (const TCHAR* Fixture : MalformedFixtures)
    {
        TestFalse(TEXT("Signed malformed claims rejected"), Service.Validate(Fixture, 1700000000).IsSuccess());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHSessionConfigTest, "MH.Backend.SessionConfig",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHSessionConfigTest::RunTest(const FString& Parameters)
{
    const FString Filename = FConfigCacheIni::NormalizeConfigIniPath(
        FPaths::ProjectSavedDir() / (TEXT("MHAuthTest-") + FGuid::NewGuid().ToString() + TEXT(".ini")));
    ON_SCOPE_EXIT
    {
        GConfig->UnloadFile(Filename);
        IFileManager::Get().Delete(*Filename);
    };
    if (!TestTrue(TEXT("Write isolated config fixture"), FFileHelper::SaveStringToFile(
        TEXT("[MH.Auth]\nSecret=fixture-secret\nLifetimeSeconds=120\n"), *Filename))) { return false; }
    const auto Loaded = FMHSessionTokenService::LoadConfig(Filename);
    if (!TestTrue(TEXT("Explicit config loads"), Loaded.IsSuccess())) { return false; }
    TestEqual(TEXT("Lifetime read"), Loaded.GetValue()->LifetimeSeconds, 120);
    TestEqual(TEXT("Secret read"), Loaded.GetValue()->Secret, FString(TEXT("fixture-secret")));
    GConfig->SetInt(TEXT("MH.Auth"), TEXT("LifetimeSeconds"), 0, Filename);
    TestFalse(TEXT("Invalid configured lifetime rejected"), FMHSessionTokenService::LoadConfig(Filename).IsSuccess());
    GConfig->SetInt(TEXT("MH.Auth"), TEXT("LifetimeSeconds"), 120, Filename);
    GConfig->SetString(TEXT("MH.Auth"), TEXT("Secret"), TEXT(""), Filename);
    TestFalse(TEXT("Missing configured secret rejected"), FMHSessionTokenService::LoadConfig(Filename).IsSuccess());
    return true;
}
#endif
