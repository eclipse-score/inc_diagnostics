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

/// @file sovd_error_test.cpp
/// @brief Unit tests for score/mw/diag/sovd_error.h
///        Covers: sovd::ErrorCode, GenericError, DataError,
///                ErrorCode struct, Error, Result<T>, ResultBlank helpers.

#include "score/mw/diag/sovd_error.h"

#include <gtest/gtest.h>

namespace score::mw::diag
{

// ── sovd::ErrorCode to_string ────────────────────────────────────────────

TEST(SovdErrorTest, ErrorCodeToStringAllVariants)
{
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::ErrorResponse),           "error-response");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::IncompleteRequest),       "incomplete-request");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::InsufficientAccessRights),"insufficient-access-rights");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::InvalidResponseContent),  "invalid-response-content");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::InvalidSignature),        "invalid-signature");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::LockBroken),              "lock-broken");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::NotResponding),           "not-responding");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::PreconditionNotFulfilled),"precondition-not-fulfilled");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::SovdServerFailure),       "sovd-server-failure");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::SovdServerMisconfigured), "sovd-server-misconfigured");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::UpdateAutomatedNotSupported),  "update-automated-not-supported");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::UpdateExecutionInProgress),    "update-execution-in-progress");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::UpdatePreparationInProgress),  "update-preparation-in-progress");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::UpdateProcessInProgress),      "update-process-in-progress");
    EXPECT_EQ(sovd::to_string(sovd::ErrorCode::VendorSpecific),          "vendor-specific");
}

// ── sovd::GenericError constructors ──────────────────────────────────────

TEST(SovdErrorTest, GenericErrorFromCode)
{
    const auto err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "test msg");
    EXPECT_EQ(err.sovd_error,   "error-response");
    EXPECT_EQ(err.message_text, "test msg");
    EXPECT_FALSE(err.vendor_error.has_value());
    EXPECT_FALSE(err.translation_id.has_value());
    EXPECT_FALSE(err.additional_attrs.has_value());
}

TEST(SovdErrorTest, GenericErrorFromVendorError)
{
    const auto err = sovd::GenericError::from_vendor_error("BMW-42", "custom err");
    EXPECT_EQ(err.sovd_error,          "vendor-specific");
    EXPECT_EQ(err.message_text,        "custom err");
    EXPECT_EQ(err.vendor_error.value(), "BMW-42");
    EXPECT_FALSE(err.translation_id.has_value());
}

TEST(SovdErrorTest, GenericErrorFromCodeWithTranslation)
{
    const auto err = sovd::GenericError::from_code_with_translation(
        sovd::ErrorCode::LockBroken, "locked", "tid_001");
    EXPECT_EQ(err.sovd_error,            "lock-broken");
    EXPECT_EQ(err.message_text,          "locked");
    EXPECT_EQ(err.translation_id.value(), "tid_001");
    EXPECT_FALSE(err.vendor_error.has_value());
}

// ── sovd::GenericError builders ───────────────────────────────────────────

TEST(SovdErrorTest, GenericErrorWithTranslationIdBuilder)
{
    auto err = sovd::GenericError::from_code(sovd::ErrorCode::NotResponding, "msg");
    err = std::move(err).with_translation_id("t_42");
    EXPECT_EQ(err.translation_id.value(), "t_42");
}

TEST(SovdErrorTest, GenericErrorWithAdditionalAttrsBuilder)
{
    KeyValueAttributes attrs;
    attrs.emplace("k1", "v1");
    auto err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg");
    err = std::move(err).with_additional_attrs(std::move(attrs));
    ASSERT_TRUE(err.additional_attrs.has_value());
    EXPECT_EQ(err.additional_attrs->at("k1"), "v1");
}

TEST(SovdErrorTest, GenericErrorAddAdditionalAttrCreatesMap)
{
    auto err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg");
    EXPECT_FALSE(err.additional_attrs.has_value());
    err.add_additional_attr("key", "val");
    ASSERT_TRUE(err.additional_attrs.has_value());
    EXPECT_EQ(err.additional_attrs->at("key"), "val");
}

TEST(SovdErrorTest, GenericErrorAddAdditionalAttrAppends)
{
    auto err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg");
    err.add_additional_attr("k1", "v1");
    err.add_additional_attr("k2", "v2");
    ASSERT_TRUE(err.additional_attrs.has_value());
    EXPECT_EQ(err.additional_attrs->size(), 2U);
    EXPECT_EQ(err.additional_attrs->at("k1"), "v1");
    EXPECT_EQ(err.additional_attrs->at("k2"), "v2");
}

TEST(SovdErrorTest, GenericErrorSetAdditionalAttrsReplaces)
{
    auto err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg");
    err.add_additional_attr("old", "val");
    KeyValueAttributes new_attrs;
    new_attrs.emplace("new", "new_val");
    err.set_additional_attrs(std::move(new_attrs));
    ASSERT_TRUE(err.additional_attrs.has_value());
    EXPECT_EQ(err.additional_attrs->size(), 1U);
    EXPECT_EQ(err.additional_attrs->at("new"), "new_val");
    EXPECT_EQ(err.additional_attrs->count("old"), 0U);
}

// ── sovd::DataError constructors ──────────────────────────────────────────

TEST(SovdErrorTest, DataErrorFromPath)
{
    const auto de = sovd::DataError::from_path("/entity/data/1");
    EXPECT_EQ(de.path, "/entity/data/1");
    EXPECT_FALSE(de.error.has_value());
}

TEST(SovdErrorTest, DataErrorFromError)
{
    const auto de = sovd::DataError::from_error(
        sovd::GenericError::from_code(sovd::ErrorCode::LockBroken, "locked"));
    EXPECT_TRUE(de.path.empty());
    ASSERT_TRUE(de.error.has_value());
    EXPECT_EQ(de.error->sovd_error, "lock-broken");
}

TEST(SovdErrorTest, DataErrorWithPathAndError)
{
    const auto de = sovd::DataError::with_path_and_error(
        "/path",
        sovd::GenericError::from_code(sovd::ErrorCode::InvalidSignature, "sig fail"));
    EXPECT_EQ(de.path, "/path");
    ASSERT_TRUE(de.error.has_value());
    EXPECT_EQ(de.error->sovd_error, "invalid-signature");
}

TEST(SovdErrorTest, DataErrorWithErrorBuilder)
{
    auto de = sovd::DataError::from_path("/p");
    de = std::move(de).with_error(
        sovd::GenericError::from_code(sovd::ErrorCode::PreconditionNotFulfilled, "nope"));
    ASSERT_TRUE(de.error.has_value());
    EXPECT_EQ(de.error->sovd_error, "precondition-not-fulfilled");
}

TEST(SovdErrorTest, DataErrorWithPathBuilder)
{
    auto de = sovd::DataError::from_error(
        sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "err"));
    de = std::move(de).with_path("/new/path");
    EXPECT_EQ(de.path, "/new/path");
    EXPECT_TRUE(de.error.has_value());
}

// ── ErrorCode (std::variant discriminated union) ─────────────────────────

TEST(SovdErrorTest, ErrorCodeDefaultHoldsSovdGenericError)
{
    // std::variant default-constructs the first alternative: sovd::GenericError
    const ErrorCode code{};
    EXPECT_TRUE(is_sovd_error(code));
    EXPECT_FALSE(is_uds_error(code));
}

TEST(SovdErrorTest, ErrorCodeHoldsNrc)
{
    const ErrorCode code{uds::NegativeResponseCode::RequestOutOfRange};
    EXPECT_FALSE(is_sovd_error(code));
    EXPECT_TRUE(is_uds_error(code));
    EXPECT_EQ(get_uds_nrc(code), uds::NegativeResponseCode::RequestOutOfRange);
}

// ── Error factories ───────────────────────────────────────────────────────

TEST(SovdErrorTest, ErrorFromSovdError)
{
    const auto err = Error::from_error(
        sovd::GenericError::from_code(sovd::ErrorCode::SovdServerFailure, "down"));
    EXPECT_TRUE(is_sovd_error(err.code));
    EXPECT_EQ(get_sovd_error(err.code).sovd_error, "sovd-server-failure");
    EXPECT_EQ(get_sovd_error(err.code).message_text, "down");
    EXPECT_FALSE(err.payload.has_value());
}

TEST(SovdErrorTest, ErrorFromNrc)
{
    const auto err = Error::from_nrc(uds::NegativeResponseCode::RequestOutOfRange);
    EXPECT_TRUE(is_uds_error(err.code));
    EXPECT_EQ(get_uds_nrc(err.code), uds::NegativeResponseCode::RequestOutOfRange);
    EXPECT_FALSE(err.payload.has_value());
}

TEST(SovdErrorTest, ErrorWithPayloadBuilder)
{
    auto err = Error::from_nrc(uds::NegativeResponseCode::GeneralReject);
    EXPECT_FALSE(err.payload.has_value());
    err = std::move(err).with_payload(ReplyMessagePayload::from_string("detail"));
    ASSERT_TRUE(err.payload.has_value());
    EXPECT_EQ(err.payload->text_data, "detail");
}

TEST(SovdErrorTest, ErrorMutexPoisoned)
{
    const auto err = Error::mutex_poisoned();
    EXPECT_TRUE(is_sovd_error(err.code));
    EXPECT_EQ(get_sovd_error(err.code).sovd_error, "sovd-server-failure");
    EXPECT_EQ(get_sovd_error(err.code).message_text, "mutex acquisition failed unexpectedly");
}

// ── Result<T> and helpers ─────────────────────────────────────────────────

TEST(SovdErrorTest, ResultIsOkInt)
{
    const Result<int> r{42};
    EXPECT_TRUE(is_ok(r));
    EXPECT_FALSE(is_err(r));
    EXPECT_EQ(get_value(r), 42);
}

TEST(SovdErrorTest, ResultIsErr)
{
    const Result<int> r{score::unexpect, Error::from_nrc(uds::NegativeResponseCode::GeneralReject)};
    EXPECT_FALSE(is_ok(r));
    EXPECT_TRUE(is_err(r));
    EXPECT_TRUE(is_uds_error(get_error(r).code));
}

TEST(SovdErrorTest, ResultBlankOk)
{
    const ResultBlank r{};
    EXPECT_TRUE(is_ok(r));
}

TEST(SovdErrorTest, ResultBlankErr)
{
    const ResultBlank r{score::unexpect, Error::from_nrc(uds::NegativeResponseCode::ConditionsNotCorrect)};
    EXPECT_TRUE(is_err(r));
}

}  // namespace score::mw::diag
