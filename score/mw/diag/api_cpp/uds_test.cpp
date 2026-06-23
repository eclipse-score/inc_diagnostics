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

/// @file uds_test.cpp
/// @brief Unit tests for score/mw/diag/uds.h
///        Covers: StartRoutine, ReadDataByIdentifier, WriteDataByIdentifier,
///                RoutineControl via ReadDataByIdentifierMock, WriteDataByIdentifierMock,
///                RoutineControlMock.

#include "score/mw/diag/uds_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::_;

namespace score::mw::diag
{

// ── StartRoutine ────────────────────────────────────────────────────

TEST(UdsTest, StartRoutineResultDefaultNoReply)
{
    const StartRoutine result{};
    EXPECT_FALSE(result.reply.has_value());
    EXPECT_FALSE(static_cast<bool>(result.result_provider));
}

TEST(UdsTest, StartRoutineResultWithReplyAndProvider)
{
    StartRoutine result{};
    result.reply             = ByteVector{std::byte{0xBE}, std::byte{0xEF}};
    result.result_provider   = []() -> Result<std::optional<ByteVector>> {
        return std::optional<ByteVector>{ByteVector{std::byte{0xCA}, std::byte{0xFE}}};
    };
    ASSERT_TRUE(result.reply.has_value());
    EXPECT_EQ(result.reply->size(), 2U);
    EXPECT_EQ((*result.reply)[0], std::byte{0xBE});

    const auto exec_result = result.result_provider();
    ASSERT_TRUE(is_ok(exec_result));
    ASSERT_TRUE(get_value(exec_result).has_value());
    EXPECT_EQ((*get_value(exec_result))[0], std::byte{0xCA});
}

TEST(UdsTest, StartRoutineResultProviderReturnsNone)
{
    StartRoutine result{};
    result.result_provider = []() -> Result<std::optional<ByteVector>> {
        return std::optional<ByteVector>{std::nullopt};
    };
    const auto exec_result = result.result_provider();
    ASSERT_TRUE(is_ok(exec_result));
    EXPECT_FALSE(get_value(exec_result).has_value());
}

TEST(UdsTest, StartRoutineResultProviderReturnsError)
{
    StartRoutine result{};
    result.result_provider = []() -> Result<std::optional<ByteVector>> {
        return Result<std::optional<ByteVector>>{score::unexpect, Error::from_nrc(uds::NegativeResponseCode::ConditionsNotCorrect)};
    };
    const auto exec_result = result.result_provider();
    EXPECT_TRUE(is_err(exec_result));
}

// ── ReadDataByIdentifierMock ──────────────────────────────────────────────

TEST(UdsTest, RdbiReadReturnsSuccessBytes)
{
    ReadDataByIdentifierMock mock{};
    EXPECT_CALL(mock, read())
        .WillOnce(Return(Result<ByteVector>{ByteVector{std::byte{0xDE}, std::byte{0xAD}}}));

    const auto result = mock.read();
    ASSERT_TRUE(is_ok(result));
    EXPECT_EQ(get_value(result).size(), 2U);
    EXPECT_EQ(get_value(result)[0], std::byte{0xDE});
}

TEST(UdsTest, RdbiReadReturnsError)
{
    ReadDataByIdentifierMock mock{};
    EXPECT_CALL(mock, read())
        .WillOnce(Return(Result<ByteVector>{
            score::unexpect, Error::from_nrc(uds::NegativeResponseCode::SecurityAccessDenied)}));

    const auto result = mock.read();
    EXPECT_TRUE(is_err(result));
    EXPECT_EQ(get_uds_nrc(get_error(result).code),
              uds::NegativeResponseCode::SecurityAccessDenied);
}

TEST(UdsTest, RdbiReadCalledMultipleTimes)
{
    ReadDataByIdentifierMock mock{};
    EXPECT_CALL(mock, read()).Times(3)
        .WillRepeatedly(Return(Result<ByteVector>{ByteVector{std::byte{0x01}}}));

    for (int i = 0; i < 3; ++i)
    {
        EXPECT_TRUE(is_ok(mock.read()));
    }
}

// ── WriteDataByIdentifierMock ─────────────────────────────────────────────

TEST(UdsTest, WdbiWriteReturnsOk)
{
    WriteDataByIdentifierMock mock{};
    const ByteVector          data{std::byte{0x01}, std::byte{0x02}};
    EXPECT_CALL(mock, write(_))
        .WillOnce(Return(ResultBlank{}));

    const auto result = mock.write(ByteView{data});
    EXPECT_TRUE(is_ok(result));
}

TEST(UdsTest, WdbiWriteReturnsError)
{
    WriteDataByIdentifierMock mock{};
    const ByteVector          data{std::byte{0xFF}};
    EXPECT_CALL(mock, write(_))
        .WillOnce(Return(ResultBlank{
            score::unexpect, Error::from_nrc(uds::NegativeResponseCode::GeneralProgrammingFailure)}));

    const auto result = mock.write(ByteView{data});
    EXPECT_TRUE(is_err(result));
    EXPECT_EQ(get_uds_nrc(get_error(result).code),
              uds::NegativeResponseCode::GeneralProgrammingFailure);
}

TEST(UdsTest, WdbiWriteEmptyInput)
{
    WriteDataByIdentifierMock mock{};
    EXPECT_CALL(mock, write(_))
        .WillOnce(Return(ResultBlank{}));

    const ByteView empty_view{};
    EXPECT_TRUE(is_ok(mock.write(empty_view)));
}

// ── RoutineControlMock ────────────────────────────────────────────────────

TEST(UdsTest, RoutineControlStartReturnsSuccessWithReply)
{
    RoutineControlMock mock{};
    StartRoutine expected_result{};
    expected_result.reply = ByteVector{std::byte{0xBE}, std::byte{0xEF}};
    expected_result.result_provider = []() -> Result<std::optional<ByteVector>> {
        return std::optional<ByteVector>{ByteVector{std::byte{0xCA}, std::byte{0xFE}}};
    };

    EXPECT_CALL(mock, start(_))
        .WillOnce(Return(Result<StartRoutine>{std::move(expected_result)}));

    const ByteVector   input_data{std::byte{0x01}};
    const ByteView     input_view{input_data};
    const auto result = mock.start(std::optional<ByteView>{input_view});

    ASSERT_TRUE(is_ok(result));
    EXPECT_TRUE(get_value(result).reply.has_value());
}

TEST(UdsTest, RoutineControlStartReturnsOkWithNoInput)
{
    RoutineControlMock mock{};
    // ByteView lacks operator==; use wildcard matcher and verify via return value.
    EXPECT_CALL(mock, start(_))
        .WillOnce(Return(Result<StartRoutine>{StartRoutine{}}));

    const auto result = mock.start(std::nullopt);
    EXPECT_TRUE(is_ok(result));
}

TEST(UdsTest, RoutineControlStartReturnsError)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, start(_))
        .WillOnce(Return(Result<StartRoutine>{
            score::unexpect, Error::from_nrc(uds::NegativeResponseCode::ConditionsNotCorrect)}));

    const auto result = mock.start(std::nullopt);
    EXPECT_TRUE(is_err(result));
}

TEST(UdsTest, RoutineControlStopReturnsBytes)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, stop(_))
        .WillOnce(Return(Result<std::optional<ByteVector>>{
            std::optional<ByteVector>{ByteVector{std::byte{0xDA}, std::byte{0xDA}}}}));

    const auto result = mock.stop(std::nullopt);
    ASSERT_TRUE(is_ok(result));
    ASSERT_TRUE(get_value(result).has_value());
    EXPECT_EQ((*get_value(result))[0], std::byte{0xDA});
}

TEST(UdsTest, RoutineControlStopReturnsNone)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, stop(_))
        .WillOnce(Return(Result<std::optional<ByteVector>>{std::optional<ByteVector>{}}));

    const auto result = mock.stop(std::nullopt);
    ASSERT_TRUE(is_ok(result));
    EXPECT_FALSE(get_value(result).has_value());
}

TEST(UdsTest, RoutineControlCompletionPercentageProvided)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, completion_percentage())
        .WillOnce(Return(std::optional<std::uint8_t>{75U}));

    const auto pct = mock.completion_percentage();
    ASSERT_TRUE(pct.has_value());
    EXPECT_EQ(pct.value(), 75U);
}

TEST(UdsTest, RoutineControlCompletionPercentageNotAvailable)
{
    RoutineControlMock mock{};
    EXPECT_CALL(mock, completion_percentage())
        .WillOnce(Return(std::optional<std::uint8_t>{}));

    EXPECT_FALSE(mock.completion_percentage().has_value());
}

// ── UdsService ────────────────────────────────────────────────────────

TEST(UdsTest, UdsServiceDefaultHandleMessageReturnsSubFunctionNotSupported)
{
    // Concrete subclass that does not override handle_message —
    // relies on the default implementation.
    struct DefaultUdsService final : public UdsService {};

    DefaultUdsService svc{};
    const ByteVector request{std::byte{0xB2}, std::byte{0x01}};
    const auto result = svc.handle_message(ByteView{request});
    EXPECT_TRUE(is_err(result));
    EXPECT_TRUE(is_uds_error(get_error(result).code));
    EXPECT_EQ(get_uds_nrc(get_error(result).code),
              uds::NegativeResponseCode::SubFunctionNotSupported);
}

TEST(UdsTest, UdsServiceMockHandleMessageReturnsBytes)
{
    UdsServiceMock mock{};
    const ByteVector response{std::byte{0xF2}, std::byte{0x00}};
    EXPECT_CALL(mock, handle_message(_))
        .WillOnce(Return(Result<ByteVector>{response}));

    const ByteVector request{std::byte{0xB2}, std::byte{0x01}};
    const auto result = mock.handle_message(ByteView{request});
    ASSERT_TRUE(is_ok(result));
    EXPECT_EQ(get_value(result).size(), 2U);
    EXPECT_EQ(get_value(result)[0], std::byte{0xF2});
}

TEST(UdsTest, UdsServiceMockHandleMessageReturnsError)
{
    UdsServiceMock mock{};
    EXPECT_CALL(mock, handle_message(_))
        .WillOnce(Return(Result<ByteVector>{
            score::unexpect,
            Error::from_nrc(uds::NegativeResponseCode::SecurityAccessDenied)}));

    const ByteVector request{std::byte{0xB2}};
    const auto result = mock.handle_message(ByteView{request});
    EXPECT_TRUE(is_err(result));
    EXPECT_EQ(get_uds_nrc(get_error(result).code),
              uds::NegativeResponseCode::SecurityAccessDenied);
}

}  // namespace score::mw::diag
