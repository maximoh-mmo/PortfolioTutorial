#pragma once

#include "MHError.h"
#include "Misc/TVariant.h"

/** Exactly one success value or error. Use FMHUnit for operations without a payload. */
template <typename T>
class TMHResult
{
    struct FSuccess { T Value; };
    TVariant<FSuccess, FMHError> Storage;

    explicit TMHResult(T Value)
        : Storage(TInPlaceType<FSuccess>(), FSuccess{MoveTemp(Value)}) {}
    explicit TMHResult(FMHError Error, bool)
        : Storage(TInPlaceType<FMHError>(), MoveTemp(Error)) {}

public:
    static TMHResult Success(T Value) { return TMHResult(MoveTemp(Value)); }
    static TMHResult Failure(FMHError Error) { return TMHResult(MoveTemp(Error), false); }

    bool IsSuccess() const { return Storage.template IsType<FSuccess>(); }
    T* GetValue()
    {
        return IsSuccess() ? &Storage.template Get<FSuccess>().Value : nullptr;
    }
    const T* GetValue() const
    {
        return IsSuccess() ? &Storage.template Get<FSuccess>().Value : nullptr;
    }
    const FMHError* GetError() const
    {
        return IsSuccess() ? nullptr : &Storage.template Get<FMHError>();
    }
};
