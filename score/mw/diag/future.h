/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

/// @file future.h
/// @brief Async promise/future type aliases for diagnostic service operations.

#ifndef SCORE_MW_DIAG_FUTURE_H
#define SCORE_MW_DIAG_FUTURE_H

#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"

#include <utility>

namespace score::mw::diag
{

/// Public type alias for async diagnostic promises.
template <typename Value>
using Promise = score::concurrency::InterruptiblePromise<Value>;

/// Public type alias for async diagnostic futures.
template <typename Value>
using Future = score::concurrency::InterruptibleFuture<Value>;

/// Continuation callback type for use with mw::diag::Future::Then().
template <typename Value>
using FutureContinuation = typename score::concurrency::InterruptibleState<Value>::ScopedContinuationCallback;

namespace details
{

/// Helper function to satisfy a promise with a value and retrieve its associated future.
/// If satisfying the promise fails, sets the corresponding error on the promise.
/// @param promise The promise to satisfy.
/// @param value The value to store in the promise.
/// @return An associated InterruptibleFuture containing the value or error.
template <typename Value>
Future<Value> GetFuture(Promise<Value>& promise, Value&& value)
{
    if (const auto result = promise.SetValue(std::forward<Value>(value)); !result.has_value())
    {
        score::cpp::ignore = promise.SetError(score::concurrency::MakeError(result.error()));
    }
    return promise.GetInterruptibleFuture().value();
}

}  // namespace details

/// Wraps an immediate value or Result into a satisfied diagnostic Future.
/// @tparam Value The type of value or Result to wrap.
/// @param value Value or Result instance to wrap into a future.
/// @return A satisfied Future containing the value or result.
template <typename Value>
Future<Value> WrapAsFuture(Value&& value)
{
    Promise<Value> promise;
    return details::GetFuture(promise, std::forward<Value>(value));
}

}  // namespace score::mw::diag

#endif  // SCORE_MW_DIAG_FUTURE_H
