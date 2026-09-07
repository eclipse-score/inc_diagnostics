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

#include "score/mw/diag/uds/routine_control.h"
#include "score/mw/diag/uds/negative_response_code.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds::test
{

namespace
{

class SimpleRoutineControlForTest : public SimpleRoutineControl
{
  public:
    Result<ByteVector> start_result{ByteVector{std::byte{0x01}}};
    Result<ByteVector> stop_result{ByteVector{std::byte{0x02}}};
    Result<ByteVector> request_results_result{ByteVector{std::byte{0x03}}};

    Result<ByteVector> Start(ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        return start_result;
    }

    Result<ByteVector> Stop(ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        return stop_result;
    }

    Result<ByteVector> RequestResults(ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        return request_results_result;
    }
};

}  // namespace

TEST(RoutineControlTest, SimpleRoutineControlBridgesResultToFuture)
{
    SimpleRoutineControlForTest routine;
    MetaData meta_data{};
    score::cpp::stop_token stop_token{};
    ByteVector payload{std::byte{0xFF}};

    RoutineControl& full_interface = routine;

    auto start_future = full_interface.Start(payload, meta_data, stop_token);
    auto start_outer = start_future.Get(stop_token);
    ASSERT_TRUE(start_outer.has_value());
    auto start_res = start_outer.value();
    ASSERT_TRUE(start_res.has_value());
    EXPECT_EQ(start_res.value()[0], std::byte{0x01});

    auto stop_future = full_interface.Stop(payload, meta_data, stop_token);
    auto stop_outer = stop_future.Get(stop_token);
    ASSERT_TRUE(stop_outer.has_value());
    auto stop_res = stop_outer.value();
    ASSERT_TRUE(stop_res.has_value());
    EXPECT_EQ(stop_res.value()[0], std::byte{0x02});

    auto req_future = full_interface.RequestResults(payload, meta_data, stop_token);
    auto req_outer = req_future.Get(stop_token);
    ASSERT_TRUE(req_outer.has_value());
    auto req_res = req_outer.value();
    ASSERT_TRUE(req_res.has_value());
    EXPECT_EQ(req_res.value()[0], std::byte{0x03});
}

TEST(RoutineControlTest, SimpleRoutineControlPropagatesError)
{
    SimpleRoutineControlForTest routine;
    routine.start_result = score::MakeUnexpected(NegativeResponseCode::ConditionsNotCorrect);

    MetaData meta_data{};
    score::cpp::stop_token stop_token{};
    ByteVector payload{std::byte{0xFF}};

    RoutineControl& full_interface = routine;

    auto start_future = full_interface.Start(payload, meta_data, stop_token);
    auto start_outer = start_future.Get(stop_token);
    ASSERT_TRUE(start_outer.has_value());
    auto start_res = start_outer.value();
    ASSERT_FALSE(start_res.has_value());
    EXPECT_EQ(ToNegativeResponseCode(*start_res.error()), NegativeResponseCode::ConditionsNotCorrect);
}

}  // namespace score::mw::diag::uds::test
