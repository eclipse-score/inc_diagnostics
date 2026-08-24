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

#include "score/mw/diag/future.h"
#include "score/mw/diag/diag_result.h"

#include <gtest/gtest.h>

namespace score::mw::diag::test
{

TEST(FutureTest, WrapAsFutureReturnsSatisfiedFuture)
{
    auto future = WrapAsFuture(Result<int>{42});
    score::cpp::stop_token stop_token{};
    auto outer_res = future.Get(stop_token);
    ASSERT_TRUE(outer_res.has_value());
    EXPECT_EQ(outer_res.value().value(), 42);
}

TEST(FutureTest, GetFutureHandlesAlreadySatisfiedPromise)
{
    Promise<Result<int>> promise;
    // Set an error on promise BEFORE retrieving future
    score::cpp::ignore = promise.SetError(score::concurrency::MakeError(score::concurrency::Error::kPromiseBroken));

    score::cpp::stop_token stop_token{};
    // GetFuture attempts SetValue (fails), then returns the future
    auto future = details::GetFuture(promise, Result<int>{200});
    auto outer_res = future.Get(stop_token);

    // The future contains the error previously stored on the promise
    ASSERT_FALSE(outer_res.has_value());
    EXPECT_EQ(*outer_res.error(), static_cast<score::result::ErrorCode>(score::concurrency::Error::kPromiseBroken));
}

}  // namespace score::mw::diag::test
