#include "IMHIdentityProvider.h"

FMHAuthTicket::FMHAuthTicket(FMHAccountId InAccount, FString InAudience, TArray<uint8> InBytes,
    TUniqueFunction<void()>&& InRelease)
    : Account(MoveTemp(InAccount)), Audience(MoveTemp(InAudience)), Bytes(MoveTemp(InBytes)),
      ReleaseNative(MoveTemp(InRelease))
{
    check(IsInGameThread());
}

FMHAuthTicket::~FMHAuthTicket()
{
    Release();
}

void FMHAuthTicket::Release()
{
    check(IsInGameThread());
    if (bReleased) { return; }
    bReleased = true;
    if (!Bytes.IsEmpty()) { FMemory::Memzero(Bytes.GetData(), Bytes.Num()); }
    Bytes.Empty();
    auto Cleanup = MoveTemp(ReleaseNative);
    if (Cleanup) { Cleanup(); }
}
