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

/// @file data_resource_test.cpp
/// @brief Unit tests for score/mw/diag/data_resource.h
///        Covers: sovd::DataCategory, DataCategoryInfo, DataResourceMetadata,
///                ReadValueArgs, ReadValueReply, WriteValueArgs,
///                DataResource default write behaviour, DataResourceMock.

#include "score/mw/diag/data_resource_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::_;

namespace score::mw::diag
{

// ── sovd::DataCategory to_string ─────────────────────────────────────────

TEST(DataResourceTest, DataCategoryToStringAllStandard)
{
    EXPECT_EQ(sovd::to_string(sovd::DataCategory::IdentData),   "identData");
    EXPECT_EQ(sovd::to_string(sovd::DataCategory::CurrentData), "currentData");
    EXPECT_EQ(sovd::to_string(sovd::DataCategory::StoredData),  "storedData");
    EXPECT_EQ(sovd::to_string(sovd::DataCategory::SysInfo),     "sysInfo");
}

TEST(DataResourceTest, DataCategoryCustomToStringReturnsName)
{
    const auto cat = sovd::DataCategory::custom("diagnosticHistory");
    EXPECT_EQ(sovd::to_string(cat), "diagnosticHistory");
    EXPECT_EQ(cat.to_string(), "diagnosticHistory");
    EXPECT_TRUE(cat.is_custom());
}

// ── sovd::data_category_from_string ──────────────────────────────────────

TEST(DataResourceTest, DataCategoryFromStringKnown)
{
    EXPECT_EQ(sovd::data_category_from_string("identData"),   sovd::DataCategory::IdentData);
    EXPECT_EQ(sovd::data_category_from_string("currentData"), sovd::DataCategory::CurrentData);
    EXPECT_EQ(sovd::data_category_from_string("storedData"),  sovd::DataCategory::StoredData);
    EXPECT_EQ(sovd::data_category_from_string("sysInfo"),     sovd::DataCategory::SysInfo);
}

TEST(DataResourceTest, DataCategoryFromStringUnknownIsCustomWithName)
{
    // Non-standard strings become Custom with the name preserved 
    const auto cat = sovd::data_category_from_string("myCustomCategory");
    EXPECT_TRUE(cat.is_custom());
    EXPECT_EQ(cat.custom_name(), "myCustomCategory");

    const auto empty_cat = sovd::data_category_from_string("");
    EXPECT_TRUE(empty_cat.is_custom());
    EXPECT_TRUE(empty_cat.custom_name().empty());
}

// ── sovd::DataCategory class ────────────────────────────────────────────────────

TEST(DataResourceTest, DataCategoryStandardIsNotCustom)
{
    EXPECT_FALSE(sovd::DataCategory::SysInfo.is_custom());
    EXPECT_FALSE(sovd::DataCategory::IdentData.is_custom());
    EXPECT_FALSE(sovd::DataCategory::CurrentData.is_custom());
    EXPECT_FALSE(sovd::DataCategory::StoredData.is_custom());
}

TEST(DataResourceTest, DataCategoryCustomCarriesName)
{
    const auto cat = sovd::DataCategory::custom("diagnosticHistory");
    EXPECT_TRUE(cat.is_custom());
    EXPECT_EQ(cat.custom_name(), "diagnosticHistory");
    EXPECT_EQ(cat.to_string(), "diagnosticHistory");
}

TEST(DataResourceTest, DataCategoryEqualityStandard)
{
    EXPECT_EQ(sovd::DataCategory::IdentData, sovd::DataCategory::IdentData);
    EXPECT_NE(sovd::DataCategory::IdentData, sovd::DataCategory::CurrentData);
}

TEST(DataResourceTest, DataCategoryEqualityCustom)
{
    EXPECT_EQ(sovd::DataCategory::custom("foo"), sovd::DataCategory::custom("foo"));
    EXPECT_NE(sovd::DataCategory::custom("foo"), sovd::DataCategory::custom("bar"));
    EXPECT_NE(sovd::DataCategory::custom("foo"), sovd::DataCategory::IdentData);
}

// ── sovd::DataResourceMetadata ────────────────────────────────────────────

TEST(DataResourceTest, DataResourceMetadataConstruction)
{
    sovd::DataResourceMetadata meta{};
    meta.id        = "voltage_supply";
    meta.name      = "Voltage Supply";
    meta.read_only = true;
    meta.category  = sovd::DataCategory::CurrentData;
    EXPECT_EQ(meta.id,   "voltage_supply");
    EXPECT_EQ(meta.name, "Voltage Supply");
    EXPECT_TRUE(meta.read_only);
    EXPECT_EQ(meta.category, sovd::DataCategory::CurrentData);
    EXPECT_FALSE(meta.category.is_custom());
    EXPECT_FALSE(meta.translation_id.has_value());
    EXPECT_FALSE(meta.groups.has_value());
}

TEST(DataResourceTest, DataResourceMetadataWithOptionals)
{
    sovd::DataResourceMetadata meta{};
    meta.id             = "vin";
    meta.name           = "VIN";
    meta.translation_id = "vin_tid";
    meta.groups         = std::vector<std::string>{"ident", "static"};
    ASSERT_TRUE(meta.translation_id.has_value());
    EXPECT_EQ(meta.translation_id.value(), "vin_tid");
    ASSERT_TRUE(meta.groups.has_value());
    EXPECT_EQ(meta.groups->size(), 2U);
}

// ── ReadValueArgs ─────────────────────────────────────────────────────────

TEST(DataResourceTest, ReadValueArgsWithBinaryEncoding)
{
    const auto args = ReadValueArgs::with_encoding(ReplyMessageEncoding::binary());
    EXPECT_EQ(args.reply_encoding.kind(), ReplyMessageEncoding::Kind::Binary);
    EXPECT_FALSE(args.additional_attrs.has_value());
}

TEST(DataResourceTest, ReadValueArgsWithUtf8Encoding)
{
    const auto args = ReadValueArgs::with_encoding(ReplyMessageEncoding::utf8());
    EXPECT_EQ(args.reply_encoding.kind(), ReplyMessageEncoding::Kind::Utf8);
}

TEST(DataResourceTest, ReadValueArgsWithAdditionalAttrs)
{
    KeyValueAttributes attrs;
    attrs.emplace("session", "default");
    auto args = ReadValueArgs::with_encoding(ReplyMessageEncoding::binary());
    args = std::move(args).with_additional_attrs(std::move(attrs));
    ASSERT_TRUE(args.additional_attrs.has_value());
    EXPECT_EQ(args.additional_attrs->at("session"), "default");
}

// ── ReadValueReply ────────────────────────────────────────────────────────

TEST(DataResourceTest, ReadValueReplyBinaryNoErrors)
{
    const ReadValueReply reply{ReplyMessagePayload::from_byte_vector({std::byte{0xAA}}), std::nullopt};
    EXPECT_EQ(reply.data.kind, ReplyMessagePayload::Kind::Binary);
    EXPECT_FALSE(reply.errors.has_value());
}

TEST(DataResourceTest, ReadValueReplyWithErrors)
{
    std::vector<sovd::DataError> errors;
    errors.push_back(sovd::DataError::from_path("/field/1"));
    const ReadValueReply reply{ReplyMessagePayload::from_string("partial"),
                               std::move(errors)};
    ASSERT_TRUE(reply.errors.has_value());
    EXPECT_EQ(reply.errors->size(), 1U);
    EXPECT_EQ(reply.errors->at(0).path, "/field/1");
}

// ── WriteValueArgs ────────────────────────────────────────────────────────

TEST(DataResourceTest, WriteValueArgsDefaultEmpty)
{
    const WriteValueArgs args{};
    EXPECT_FALSE(args.user_data.has_value());
    EXPECT_FALSE(args.user_data_signature.has_value());
    EXPECT_FALSE(args.additional_attrs.has_value());
}

TEST(DataResourceTest, WriteValueArgsWithBinaryData)
{
    WriteValueArgs args{};
    args.user_data = RequestMessagePayload::from_bytes({std::byte{0x01}, std::byte{0x02}});
    ASSERT_TRUE(args.user_data.has_value());
    ASSERT_TRUE(std::holds_alternative<ByteVector>(*args.user_data));
    EXPECT_EQ(std::get<ByteVector>(*args.user_data).size(), 2U);
}

// ── DataResource default write ────────────────────────────────────────────

TEST(DataResourceTest, DefaultWriteReturnsPreconditionNotFulfilledError)
{
    // Concrete subclass that only overrides read() — relies on default write().
    struct ReadOnlyResource final : public DataResource
    {
        Result<ReadValueReply> read(ReadValueArgs /*input*/) override
        {
            return ReadValueReply{ReplyMessagePayload::from_string("42"), std::nullopt};
        }
    };

    ReadOnlyResource resource{};
    const auto result = resource.write(WriteValueArgs{});
    ASSERT_TRUE(std::holds_alternative<sovd::DataError>(result));
    const auto& data_err = std::get<sovd::DataError>(result);
    ASSERT_TRUE(data_err.error.has_value());
    EXPECT_EQ(data_err.error->sovd_error, "precondition-not-fulfilled");
}

// ── DataResourceMock ──────────────────────────────────────────────────────

TEST(DataResourceTest, MockReadReturnsExpectedReply)
{
    DataResourceMock mock{};
    const ReadValueReply expected{ReplyMessagePayload::from_string("42"), std::nullopt};

    EXPECT_CALL(mock, read(_))
        .WillOnce(Return(DataResourceMock::ReadResult{expected}));

    const auto result = mock.read(ReadValueArgs::with_encoding(ReplyMessageEncoding::utf8()));
    ASSERT_TRUE(is_ok(result));
    EXPECT_EQ(get_value(result).data.text_data, "42");
}

TEST(DataResourceTest, MockReadReturnsError)
{
    DataResourceMock mock{};
    EXPECT_CALL(mock, read(_))
        .WillOnce(Return(DataResourceMock::ReadResult{
            score::unexpect,
            Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::NotResponding, "device not responding"))}));

    const auto result = mock.read(ReadValueArgs::with_encoding(ReplyMessageEncoding::binary()));
    EXPECT_TRUE(is_err(result));
    EXPECT_TRUE(is_sovd_error(get_error(result).code));
    EXPECT_EQ(get_sovd_error(get_error(result).code).sovd_error, "not-responding");
}

TEST(DataResourceTest, MockWriteReturnsOk)
{
    DataResourceMock mock{};
    EXPECT_CALL(mock, write(_))
        .WillOnce(Return(DataResourceMock::WriteResult{std::monostate{}}));

    WriteValueArgs args{};
    args.user_data = RequestMessagePayload::from_bytes({std::byte{0xAA}});
    const auto result = mock.write(std::move(args));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(DataResourceTest, MockWriteReturnsDataError)
{
    DataResourceMock mock{};
    EXPECT_CALL(mock, write(_))
        .WillOnce(Return(DataResourceMock::WriteResult{
            sovd::DataError::from_error(
                sovd::GenericError::from_code(
                    sovd::ErrorCode::LockBroken, "resource locked"))}));

    const auto result = mock.write(WriteValueArgs{});
    ASSERT_TRUE(std::holds_alternative<sovd::DataError>(result));
    EXPECT_EQ(std::get<sovd::DataError>(result).error->sovd_error, "lock-broken");
}

}  // namespace score::mw::diag
