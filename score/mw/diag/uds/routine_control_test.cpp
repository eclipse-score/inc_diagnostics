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

/// @file routine_control_test.cpp
/// @brief Unit tests for score/mw/diag/routine_control.h
///        Covers: StartRoutine struct and RoutineControl via RoutineControlMock.

#include "score/mw/diag/uds/routine_control_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::An;
using ::testing::Return;

namespace score::mw::diag::uds
{

// ── StartRoutine ─────────────────────────────────────────────────────────

TEST(RoutineControlTest, StartRoutineDefaultNoReply)
{
    const StartRoutine result{};
    EXPECT_FALSE(result.reply.has_value());
    EXPECT_EQ(result.result_provider, nullptr);
}

TEST(RoutineControlTest, StartRoutineWithReplyAndProvider)
{
    StartRoutine result{};
    result.reply = ByteVector{std::byte{0xBE}, std::byte{0xEF}};
    result.result_provider = []() -> StopResult {
        return std::optional<ByteVector>{ByteVector{std::byte{0xCA}, std::byte{0xFE}}};
    };
    ASSERT_TRUE(result.reply.has_value());
    EXPECT_EQ(result.reply->size(), 2U);
    EXPECT_EQ((*result.reply)[0], std::byte{0xBE});

    const auto exec_result = result.result_provider();
    ASSERT_TRUE(exec_result.has_value());
    ASSERT_TRUE(exec_result->has_value());
    EXPECT_EQ((**exec_result)[0], std::byte{0xCA});
}

TEST(RoutineControlTest, StartRoutineProviderReturnsNone)
{
    StartRoutine result{};
    result.result_provider = []() -> StopResult {
        return std::optional<ByteVector>{std::nullopt};
    };
    const auto exec_result = result.result_provider();
    ASSERT_TRUE(exec_result.has_value());
    EXPECT_FALSE(exec_result->has_value());
}

TEST(RoutineControlTest, StartRoutineProviderReturnsError)
{
    StartRoutine result{};
    result.result_provider = []() -> StopResult {
        return StopResult{score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
    };
    const auto exec_result = result.result_provider();
    EXPECT_FALSE(exec_result.has_value());
}

// ── RoutineControlMock ────────────────────────────────────────────────────

TEST(RoutineControlTest, StartReturnsSuccessWithReply)
{
    RoutineControlMock mock{};
    StartRoutine expected_result{};
    expected_result.reply = ByteVector{std::byte{0xBE}, std::byte{0xEF}};
    expected_result.result_provider = []() -> StopResult {
        return std::optional<ByteVector>{ByteVector{std::byte{0xCA}, std::byte{0xFE}}};
    };

    const ByteVector input_data{std::byte{0x01}};
    const ByteView input_view{input_data};
    EXPECT_CALL(mock, Start(An<std::optional<ByteView>>())).WillOnce(Return(StartResult{std::move(expected_result)}));

    const auto result = mock.Start(std::optional<ByteView>{input_view});

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->reply.has_value());
}

TEST(RoutineControlTest, StartReturnsOkWithNoInput)
{
    RoutineControlMock mock{};
    // ByteView lacks operator==; use typed wildcard and verify via return value.
    EXPECT_CALL(mock, Start(An<std::optional<ByteView>>())).WillOnce(Return(StartResult{StartRoutine{}}));

    const auto result = mock.Start(std::nullopt);
    EXPECT_TRUE(result.has_value());
}

TEST(RoutineControlTest, StartReturnsError)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, Start(An<std::optional<ByteView>>()))
        .WillOnce(Return(StartResult{score::unexpect, NegativeResponseCode::ConditionsNotCorrect}));

    const auto result = mock.Start(std::nullopt);
    EXPECT_FALSE(result.has_value());
}

TEST(RoutineControlTest, StopReturnsBytes)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, Stop(An<std::optional<ByteView>>()))
        .WillOnce(Return(StopResult{std::optional<ByteVector>{ByteVector{std::byte{0xDA}, std::byte{0xDA}}}}));

    const auto result = mock.Stop(std::nullopt);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ((**result)[0], std::byte{0xDA});
}

TEST(RoutineControlTest, StopReturnsNone)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, Stop(An<std::optional<ByteView>>())).WillOnce(Return(StopResult{std::optional<ByteVector>{}}));

    const auto result = mock.Stop(std::nullopt);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value());
}

TEST(RoutineControlTest, CompletionPercentageProvided)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, CompletionPercentage()).WillOnce(Return(std::optional<std::uint8_t>{75U}));

    const auto pct = mock.CompletionPercentage();
    ASSERT_TRUE(pct.has_value());
    EXPECT_EQ(pct.value(), 75U);
}

TEST(RoutineControlTest, CompletionPercentageNotAvailable)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, CompletionPercentage()).WillOnce(Return(std::optional<std::uint8_t>{}));

    EXPECT_FALSE(mock.CompletionPercentage().has_value());
}

}  // namespace score::mw::diag::uds
