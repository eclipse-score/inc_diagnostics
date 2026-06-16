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

/// @file uds_adapters_test.cpp
/// @brief Unit tests for score/mw/diag/uds_adapters.h
///        Covers: DataResourceAdapter (RDBI/WDBI bridging to DataResource),
///                RoutineControlAdapter (RoutineControl bridging to SimpleOperation).

#include "score/mw/diag/uds_adapters.h"

#include "score/mw/diag/uds_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::_;

namespace score::mw::diag
{

// ═══════════════════════════════════════════════════════════════════════════
// DataResourceAdapter
// ═══════════════════════════════════════════════════════════════════════════

// ── read(): no RDBI registered ────────────────────────────────────────────

TEST(DataResourceAdapterTest, ReadWithoutRdbiReturnsError)
{
    DataResourceAdapter adapter{};
    const auto result = adapter.read(ReadValueArgs::with_encoding(ReplyMessageEncoding::binary()));
    EXPECT_TRUE(is_err(result));
    EXPECT_TRUE(is_sovd_error(get_error(result).code));
    EXPECT_EQ(get_sovd_error(get_error(result).code).sovd_error, "precondition-not-fulfilled");
}

// ── read(): non-binary encoding rejected ─────────────────────────────────

TEST(DataResourceAdapterTest, ReadWithJsonEncodingReturnsError)
{
    auto rdbi_mock = std::make_unique<ReadDataByIdentifierMock>();
    DataResourceAdapter adapter{};
    adapter.with_rdbi(std::move(rdbi_mock));

    const auto result = adapter.read(
        ReadValueArgs::with_encoding(ReplyMessageEncoding::json(JsonSchemaRequired::No)));
    EXPECT_TRUE(is_err(result));
    EXPECT_EQ(get_sovd_error(get_error(result).code).sovd_error, "precondition-not-fulfilled");
}

// ── read(): successful RDBI delegation ───────────────────────────────────

TEST(DataResourceAdapterTest, ReadDelegatesToRdbi)
{
    auto rdbi_mock = std::make_unique<ReadDataByIdentifierMock>();
    EXPECT_CALL(*rdbi_mock, read())
        .WillOnce(Return(ReadDataByIdentifierMock::ReadResult{
            ByteVector{std::byte{0xAB}, std::byte{0xCD}}}));

    DataResourceAdapter adapter{};
    adapter.with_rdbi(std::move(rdbi_mock));

    const auto result = adapter.read(ReadValueArgs::with_encoding(ReplyMessageEncoding::binary()));
    ASSERT_TRUE(is_ok(result));
    EXPECT_EQ(get_value(result).data.kind, ReplyMessagePayload::Kind::Binary);
    EXPECT_EQ(get_value(result).data.binary_data.size(), 2U);
    EXPECT_EQ(get_value(result).data.binary_data[0], std::byte{0xAB});
    EXPECT_FALSE(get_value(result).errors.has_value());
}

// ── read(): RDBI returns error propagated ────────────────────────────────

TEST(DataResourceAdapterTest, ReadPropagatesRdbiError)
{
    auto rdbi_mock = std::make_unique<ReadDataByIdentifierMock>();
    EXPECT_CALL(*rdbi_mock, read())
        .WillOnce(Return(ReadDataByIdentifierMock::ReadResult{
            score::unexpect, Error::from_nrc(uds::NegativeResponseCode::SecurityAccessDenied)}));

    DataResourceAdapter adapter{};
    adapter.with_rdbi(std::move(rdbi_mock));

    const auto result = adapter.read(ReadValueArgs::with_encoding(ReplyMessageEncoding::binary()));
    EXPECT_TRUE(is_err(result));
    EXPECT_TRUE(is_uds_error(get_error(result).code));
    EXPECT_EQ(get_uds_nrc(get_error(result).code), uds::NegativeResponseCode::SecurityAccessDenied);
}

// ── write(): no WDBI registered ──────────────────────────────────────────

TEST(DataResourceAdapterTest, WriteWithoutWdbiReturnsDataError)
{
    DataResourceAdapter adapter{};
    const auto result = adapter.write(WriteValueArgs{});
    ASSERT_TRUE(std::holds_alternative<sovd::DataError>(result));
    const auto& de = std::get<sovd::DataError>(result);
    ASSERT_TRUE(de.error.has_value());
    EXPECT_EQ(de.error->sovd_error, "precondition-not-fulfilled");
}

// ── write(): non-binary input rejected ───────────────────────────────────

TEST(DataResourceAdapterTest, WriteWithJsonInputReturnsDataError)
{
    auto wdbi_mock = std::make_unique<WriteDataByIdentifierMock>();
    DataResourceAdapter adapter{};
    adapter.with_wdbi(std::move(wdbi_mock));

    WriteValueArgs args{};
    args.user_data = RequestMessagePayload::from_json("{\"key\":1}");
    const auto result = adapter.write(std::move(args));
    ASSERT_TRUE(std::holds_alternative<sovd::DataError>(result));
    EXPECT_EQ(std::get<sovd::DataError>(result).error->sovd_error, "incomplete-request");
}

// ── write(): successful WDBI delegation ──────────────────────────────────

TEST(DataResourceAdapterTest, WriteDelegatesToWdbi)
{
    auto wdbi_mock = std::make_unique<WriteDataByIdentifierMock>();
    EXPECT_CALL(*wdbi_mock, write(_))
        .WillOnce(Return(ResultBlank{std::monostate{}}));

    DataResourceAdapter adapter{};
    adapter.with_wdbi(std::move(wdbi_mock));

    WriteValueArgs args{};
    args.user_data = RequestMessagePayload::from_bytes(
        {std::byte{0x01}, std::byte{0x02}});
    const auto result = adapter.write(std::move(args));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

// ── write(): WDBI error mapped to DataError ───────────────────────────────

TEST(DataResourceAdapterTest, WritePropagatesWdbiErrorAsDataError)
{
    auto wdbi_mock = std::make_unique<WriteDataByIdentifierMock>();
    EXPECT_CALL(*wdbi_mock, write(_))
        .WillOnce(Return(ResultBlank{
            score::unexpect, Error::from_nrc(uds::NegativeResponseCode::GeneralProgrammingFailure)}));

    DataResourceAdapter adapter{};
    adapter.with_wdbi(std::move(wdbi_mock));

    WriteValueArgs args{};
    args.user_data = RequestMessagePayload::from_bytes({std::byte{0xFF}});
    const auto result = adapter.write(std::move(args));
    ASSERT_TRUE(std::holds_alternative<sovd::DataError>(result));
    ASSERT_TRUE(std::get<sovd::DataError>(result).error.has_value());
    // Mapped to error-response DataError
    EXPECT_EQ(std::get<sovd::DataError>(result).error->sovd_error, "error-response");
}

// ═══════════════════════════════════════════════════════════════════════════
// RoutineControlAdapter
// ═══════════════════════════════════════════════════════════════════════════

// ── start(): no input ────────────────────────────────────────────────────

TEST(RoutineControlAdapterTest, StartWithNoInputDelegatesToRoutineControl)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, start(_))
        .WillOnce([](std::optional<ByteView> /*input*/) -> RoutineControlMock::StartResult
        {
            StartRoutine sr{};
            sr.result_provider = []() -> Result<std::optional<ByteVector>>
            {
                return std::optional<ByteVector>{
                    ByteVector{std::byte{0xBE}, std::byte{0xEF}}};
            };
            return sr;
        });

    RoutineControlAdapter adapter{std::move(rc_mock)};
    auto result = adapter.start(ExecuteArguments{});
    ASSERT_TRUE(is_ok(result));

    // Invoke the future to get the operation result
    const auto exec_result = get_value(result).future();
    ASSERT_TRUE(is_ok(exec_result));
    ASSERT_TRUE(get_value(exec_result).message_payload.has_value());
    EXPECT_EQ(get_value(exec_result).message_payload->kind, ReplyMessagePayload::Kind::Binary);
    EXPECT_EQ(get_value(exec_result).message_payload->binary_data[0], std::byte{0xBE});
}

// ── start(): with binary input ────────────────────────────────────────────

TEST(RoutineControlAdapterTest, StartWithBinaryInputPassedThrough)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, start(_))
        .WillOnce([](std::optional<ByteView> view) -> RoutineControlMock::StartResult
        {
            EXPECT_TRUE(view.has_value());
            EXPECT_EQ(view->size(), 1U);
            EXPECT_EQ(view->data()[0], std::byte{0x42});
            StartRoutine sr{};
            sr.result_provider = []() -> Result<std::optional<ByteVector>> { return std::nullopt; };
            return sr;
        });

    RoutineControlAdapter adapter{std::move(rc_mock)};
    ExecuteArguments args{};
    args.user_parameters = RequestMessagePayload::from_bytes({std::byte{0x42}});
    auto result = adapter.start(std::move(args));
    ASSERT_TRUE(is_ok(result));
}

// ── start(): initial reply propagated ────────────────────────────────────

TEST(RoutineControlAdapterTest, StartInitialReplyPropagated)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, start(_))
        .WillOnce([](std::optional<ByteView>) -> RoutineControlMock::StartResult
        {
            StartRoutine sr{};
            sr.reply = ByteVector{std::byte{0xCA}, std::byte{0xFE}};
            sr.result_provider = []() -> Result<std::optional<ByteVector>> { return std::nullopt; };
            return sr;
        });

    RoutineControlAdapter adapter{std::move(rc_mock)};
    auto result = adapter.start(ExecuteArguments{});
    ASSERT_TRUE(is_ok(result));
    ASSERT_TRUE(get_value(result).reply.has_value());
    ASSERT_TRUE(get_value(result).reply->message_payload.has_value());
    EXPECT_EQ(get_value(result).reply->message_payload->binary_data[0], std::byte{0xCA});
}

// ── start(): non-binary input rejected ───────────────────────────────────

TEST(RoutineControlAdapterTest, StartWithJsonInputReturnsError)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    RoutineControlAdapter adapter{std::move(rc_mock)};

    ExecuteArguments args{};
    args.user_parameters = RequestMessagePayload::from_json("{\"x\":1}");
    const auto result = adapter.start(std::move(args));
    EXPECT_TRUE(is_err(result));
    EXPECT_TRUE(is_sovd_error(result.error().code));
    EXPECT_EQ(get_sovd_error(result.error().code).sovd_error, "precondition-not-fulfilled");
}

// ── start(): RoutineControl error propagated ─────────────────────────────

TEST(RoutineControlAdapterTest, StartErrorPropagated)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, start(_))
        .WillOnce(Return(RoutineControlMock::StartResult{
            score::unexpect, Error::from_nrc(uds::NegativeResponseCode::ConditionsNotCorrect)}));

    RoutineControlAdapter adapter{std::move(rc_mock)};
    auto result = adapter.start(ExecuteArguments{});
    EXPECT_TRUE(is_err(result));
    EXPECT_EQ(get_uds_nrc(result.error().code), uds::NegativeResponseCode::ConditionsNotCorrect);
}

// ── stop(): no reply ─────────────────────────────────────────────────────

TEST(RoutineControlAdapterTest, StopWithNoReplyReturnsNullopt)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, stop(_))
        .WillOnce(Return(RoutineControlMock::StopResult{
            std::optional<ByteVector>{std::nullopt}}));

    RoutineControlAdapter adapter{std::move(rc_mock)};
    auto result = adapter.stop(std::nullopt);
    ASSERT_TRUE(is_ok(result));
    EXPECT_FALSE(get_value(result).has_value());
}

// ── stop(): with reply bytes ──────────────────────────────────────────────

TEST(RoutineControlAdapterTest, StopWithReplyBytesPropagated)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, stop(_))
        .WillOnce(Return(RoutineControlMock::StopResult{
            std::optional<ByteVector>{ByteVector{std::byte{0x01}, std::byte{0x02}}}}));

    RoutineControlAdapter adapter{std::move(rc_mock)};
    auto result = adapter.stop(std::nullopt);
    ASSERT_TRUE(is_ok(result));
    ASSERT_TRUE(get_value(result).has_value());
    ASSERT_TRUE(get_value(result)->message_payload.has_value());
    EXPECT_EQ(get_value(result)->message_payload->binary_data.size(), 2U);
    EXPECT_EQ(get_value(result)->message_payload->binary_data[0], std::byte{0x01});
}

// ── completion_percentage(): delegation ──────────────────────────────────

TEST(RoutineControlAdapterTest, CompletionPercentageDelegated)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, completion_percentage())
        .WillRepeatedly(Return(std::optional<std::uint8_t>{66U}));

    RoutineControlAdapter adapter{std::move(rc_mock)};
    ASSERT_TRUE(adapter.completion_percentage().has_value());
    EXPECT_EQ(*adapter.completion_percentage(), 66U);
}

TEST(RoutineControlAdapterTest, CompletionPercentageNulloptDelegated)
{
    auto rc_mock = std::make_unique<RoutineControlMock>();
    EXPECT_CALL(*rc_mock, completion_percentage())
        .WillRepeatedly(Return(std::optional<std::uint8_t>{}));

    RoutineControlAdapter adapter{std::move(rc_mock)};
    EXPECT_FALSE(adapter.completion_percentage().has_value());
}

// ── VehicleManufacturerSpecificCNC (in result.h) ─────────────────────────

TEST(VehicleManufacturerSpecificCNCTest, ConstructAndReadValue)
{
    const auto cnc = uds::VehicleManufacturerSpecificCNC::from(0xF2U);
    EXPECT_EQ(cnc.value(), 0xF2U);
}

TEST(VehicleManufacturerSpecificCNCTest, EqualityOperators)
{
    const auto cnc1 = uds::VehicleManufacturerSpecificCNC::from(0xF0U);
    const auto cnc2 = uds::VehicleManufacturerSpecificCNC::from(0xF0U);
    const auto cnc3 = uds::VehicleManufacturerSpecificCNC::from(0xFEU);
    EXPECT_EQ(cnc1, cnc2);
    EXPECT_NE(cnc1, cnc3);
}

}  // namespace score::mw::diag
