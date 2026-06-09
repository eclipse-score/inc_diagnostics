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

/// @file result_test.cpp
/// @brief Unit tests for score/mw/diag/result.h
///        Covers: ByteVector, ByteView, KeyValueAttributes, NegativeResponseCode,
///                payload types, DiagnosticReply.

#include "score/mw/diag/result.h"

#include <gtest/gtest.h>

namespace score::mw::diag
{

// ── ByteVector ────────────────────────────────────────────────────────────

TEST(ResultTest, ByteVectorDefaultEmpty)
{
    const ByteVector v{};
    EXPECT_TRUE(v.empty());
}

TEST(ResultTest, ByteVectorInitializerList)
{
    const ByteVector v{0x01U, 0x02U, 0x03U};
    EXPECT_EQ(v.size(), 3U);
    EXPECT_EQ(v[0], 0x01U);
    EXPECT_EQ(v[2], 0x03U);
}

// ── ByteView ─────────────────────────────────────────────────────────────

TEST(ResultTest, ByteViewDefaultEmpty)
{
    const ByteView view{};
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0U);
}

TEST(ResultTest, ByteViewFromByteVector)
{
    const ByteVector v{0xAAU, 0xBBU, 0xCCU};
    const ByteView   view{v};
    EXPECT_EQ(view.size(), 3U);
    EXPECT_EQ(view[0], 0xAAU);
    EXPECT_EQ(view[2], 0xCCU);
}

TEST(ResultTest, ByteViewFromRawPointer)
{
    const std::array<std::uint8_t, 2> arr{0x11U, 0x22U};
    const ByteView view{arr.data(), arr.size()};
    EXPECT_EQ(view.size(), 2U);
    EXPECT_EQ(view[1], 0x22U);
}

TEST(ResultTest, ByteViewIterationMatchesData)
{
    const ByteVector v{0x01U, 0x02U, 0x03U};
    const ByteView   view{v};
    std::size_t      i = 0U;
    for (const auto byte : view)
    {
        EXPECT_EQ(byte, v[i++]);
    }
    EXPECT_EQ(i, v.size());
}

// ── NegativeResponseCode ──────────────────────────────────────────────────

TEST(ResultTest, NrcGeneralRejectValue)
{
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::GeneralReject), 0x10U);
}

TEST(ResultTest, NrcFullIso14229Coverage)
{
    // Spot-check key values from all sections of ISO 14229-1:2020 Table A.1
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::ServiceNotSupported),          0x11U);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::RequestOutOfRange),            0x31U);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::AuthenticationRequired),       0x34U);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::CertificateVerificationFailedInvalidTimePeriod), 0x50U);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::OwnershipVerificationFailed),  0x58U);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::DeAuthenticationFailed),       0x5DU);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::RequestCorrectlyReceivedResponsePending), 0x78U);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::ServiceNotSupportedInActiveSession),      0x7FU);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::VehicleSpeedTooHigh),          0x88U);
    EXPECT_EQ(static_cast<std::uint8_t>(uds::NegativeResponseCode::ResourceTemporarilyNotAvailable), 0x94U);
}

// ── JsonSchemaRequired ────────────────────────────────────────────────────

TEST(ResultTest, JsonSchemaRequiredDistinctValues)
{
    EXPECT_NE(static_cast<std::uint8_t>(JsonSchemaRequired::Yes),
              static_cast<std::uint8_t>(JsonSchemaRequired::No));
}

// ── RequestMessagePayload ─────────────────────────────────────────────────

TEST(ResultTest, RequestPayloadFromBytesKind)
{
    const auto payload = RequestMessagePayload::from_bytes({0xCAU, 0xFEU});
    EXPECT_EQ(payload.kind, RequestMessagePayload::Kind::Binary);
    EXPECT_EQ(payload.binary_data.size(), 2U);
    EXPECT_EQ(payload.binary_data[0], 0xCAU);
}

TEST(ResultTest, RequestPayloadFromJsonKind)
{
    const auto payload = RequestMessagePayload::from_json("{\"key\":1}");
    EXPECT_EQ(payload.kind, RequestMessagePayload::Kind::Json);
    EXPECT_EQ(payload.text_data, "{\"key\":1}");
    EXPECT_TRUE(payload.binary_data.empty());
}

TEST(ResultTest, RequestPayloadFromStringKind)
{
    const auto payload = RequestMessagePayload::from_string("hello");
    EXPECT_EQ(payload.kind, RequestMessagePayload::Kind::Utf8);
    EXPECT_EQ(payload.text_data, "hello");
}

// ── ReplyMessageEncoding ──────────────────────────────────────────────────

TEST(ResultTest, ReplyEncodingBinaryKind)
{
    const auto enc = ReplyMessageEncoding::binary();
    EXPECT_EQ(enc.kind(), ReplyMessageEncoding::Kind::Binary);
    EXPECT_EQ(enc.json_schema_required(), JsonSchemaRequired::No);
    EXPECT_TRUE(enc.is_binary());
}

TEST(ResultTest, ReplyEncodingJsonWithSchema)
{
    const auto enc = ReplyMessageEncoding::json(JsonSchemaRequired::Yes);
    EXPECT_EQ(enc.kind(), ReplyMessageEncoding::Kind::Json);
    EXPECT_EQ(enc.json_schema_required(), JsonSchemaRequired::Yes);
    EXPECT_TRUE(enc.is_json());
}

TEST(ResultTest, ReplyEncodingJsonWithoutSchema)
{
    const auto enc = ReplyMessageEncoding::json(JsonSchemaRequired::No);
    EXPECT_EQ(enc.kind(), ReplyMessageEncoding::Kind::Json);
    EXPECT_EQ(enc.json_schema_required(), JsonSchemaRequired::No);
}

TEST(ResultTest, ReplyEncodingUtf8Kind)
{
    const auto enc = ReplyMessageEncoding::utf8();
    EXPECT_EQ(enc.kind(), ReplyMessageEncoding::Kind::Utf8);
    EXPECT_TRUE(enc.is_utf8());
}

// ── ReplyMessagePayload ───────────────────────────────────────────────────

TEST(ResultTest, ReplyPayloadFromByteVector)
{
    const auto payload = ReplyMessagePayload::from_byte_vector({0xDEU, 0xADU});
    EXPECT_EQ(payload.kind, ReplyMessagePayload::Kind::Binary);
    EXPECT_EQ(payload.binary_data.size(), 2U);
    EXPECT_FALSE(payload.json_schema.has_value());
}

TEST(ResultTest, ReplyPayloadFromJsonNoSchema)
{
    const auto payload = ReplyMessagePayload::from_json("{\"a\":1}");
    EXPECT_EQ(payload.kind, ReplyMessagePayload::Kind::Json);
    EXPECT_EQ(payload.text_data, "{\"a\":1}");
    EXPECT_FALSE(payload.json_schema.has_value());
}

TEST(ResultTest, ReplyPayloadFromJsonWithSchema)
{
    const auto payload = ReplyMessagePayload::from_json("{\"a\":1}", "{\"type\":\"object\"}");
    EXPECT_EQ(payload.kind, ReplyMessagePayload::Kind::Json);
    ASSERT_TRUE(payload.json_schema.has_value());
    EXPECT_EQ(payload.json_schema.value(), "{\"type\":\"object\"}");
}

TEST(ResultTest, ReplyPayloadFromString)
{
    const auto payload = ReplyMessagePayload::from_string("result");
    EXPECT_EQ(payload.kind, ReplyMessagePayload::Kind::Utf8);
    EXPECT_EQ(payload.text_data, "result");
}

// ── DiagnosticReply ───────────────────────────────────────────────────────

TEST(ResultTest, DiagnosticReplyDefaultEmpty)
{
    const DiagnosticReply reply{};
    EXPECT_FALSE(reply.message_payload.has_value());
    EXPECT_FALSE(reply.additional_attrs.has_value());
}

TEST(ResultTest, DiagnosticReplyWithPayload)
{
    DiagnosticReply reply{};
    reply.message_payload = ReplyMessagePayload::from_string("ok");
    ASSERT_TRUE(reply.message_payload.has_value());
    EXPECT_EQ(reply.message_payload->text_data, "ok");
}

TEST(ResultTest, DiagnosticReplyWithAttrs)
{
    DiagnosticReply reply{};
    reply.additional_attrs.emplace();
    reply.additional_attrs->emplace("status", "ok");
    ASSERT_TRUE(reply.additional_attrs.has_value());
    EXPECT_EQ(reply.additional_attrs->at("status"), "ok");
}

}  // namespace score::mw::diag
