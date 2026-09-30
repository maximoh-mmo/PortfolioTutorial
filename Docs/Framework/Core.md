# MHCore

`MHError.h` defines stable error categories and safe diagnostic text. `MHResult.h`
provides `TMHResult<T>` with exactly one value or error; use `FMHUnit` for a success
without a payload. Inspect `GetValue()` / `GetError()` before dereferencing. Move-only
payloads are supported. These are C++ APIs, not reflected Blueprint types.

`TMHRequest<T>` is the provider completion source. Return its `FMHRequestHandle` to
the caller and retain a source copy in provider callbacks. Source/handle copies share
one thread-safe terminal gate. The first `Complete` or `Cancel` wins; later attempts
return false. Callbacks execute once on the next available core-ticker game-thread
pass, including validation errors. They never run inline inside `Complete` or `Cancel`.

```cpp
TMHRequest<FString> Request([](TMHResult<FString> Result)
{
    if (const FString* Value = Result.GetValue())
    {
        // Consume the successful value on the game thread.
    }
});
FMHRequestHandle Handle = Request.GetHandle();
Request.Complete(TMHResult<FString>::Success(TEXT("Ready")));
```

`IsPending()` reports whether a terminal result has been chosen, not whether its
callback has already executed. An empty handle is invalid and cannot cancel.
Handle/source destruction does not cancel automatically. Owners must retain handles
and cancel during teardown, and callbacks must guard their owners with weak references.
Logical cancellation does not abort underlying I/O or undo remote writes. Providers
remain responsible for timeouts, transport cleanup and shutting down outstanding work.

A missing completion function is accepted and discards the terminal result. No callback
can be promised after the engine core ticker stops. Module unload/hot reload with
pending callbacks is unsupported; quiesce providers while the ticker is still running.

Automation tests: `MH.Core.Result` and `MH.Core.RequestLifecycle`. They cover result
invariants, move-only payloads, empty handles, delayed game-thread delivery, duplicate
completion, validation error, cancellation, late responses, weak owner teardown and
sixteen worker-thread completion/cancellation races.
