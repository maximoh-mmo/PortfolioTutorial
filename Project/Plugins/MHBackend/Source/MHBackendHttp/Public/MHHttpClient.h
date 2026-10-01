#pragma once

#include "MHRequest.h"
#include "Misc/Optional.h"

struct FMHHttpRequest
{
    FString Url;
    FString Verb = TEXT("GET");
    TMap<FString, FString> Headers;
    TArray<uint8> Body;
    float TimeoutSeconds = 30.0f;
};

/** A received response, including non-2xx statuses. Bodies/headers may contain sensitive data. */
struct MHBACKENDHTTP_API FMHHttpResponse
{
    int32 StatusCode = 0;
    TArray<FString> Headers;
    TArray<uint8> Body;

    bool IsSuccessStatus() const { return StatusCode >= 200 && StatusCode < 300; }
    /** Optional service-layer interpretation, while retaining the original response. */
    TOptional<FMHError> GetStatusError() const;
};

namespace MH::Private { class FHttpClientState; }

/** Owned service. Construct, send and destroy on the game thread; handles may cancel on any thread. */
class MHBACKENDHTTP_API FMHHttpClient
{
    TSharedRef<MH::Private::FHttpClientState, ESPMode::ThreadSafe> State;

public:
    FMHHttpClient();
    ~FMHHttpClient();
    FMHHttpClient(const FMHHttpClient&) = delete;
    FMHHttpClient& operator=(const FMHHttpClient&) = delete;

    /** No retry policy. Success means a response was received; inspect its HTTP status separately. */
    FMHRequestHandle Send(const FMHHttpRequest& Request,
        TUniqueFunction<void(TMHResult<FMHHttpResponse>)>&& Completion);
    /** Cancels outstanding operations and rejects new work. Call before engine ticker shutdown. */
    void Shutdown();
};
