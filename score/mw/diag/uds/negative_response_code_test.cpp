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

/// @file negative_response_code_test.cpp
/// @brief Unit tests for score/mw/diag/uds/negative_response_code.h & .cpp

#include "score/mw/diag/uds/negative_response_code.h"
#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds
{

// ── NegativeResponseCode ──────────────────────────────────────────────────

TEST(UdsResponseCodeTest, NrcIso14229ValueMapping)
{
    // Spot-check key values from all sections of ISO 14229-1:2020 Table A.1
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::GeneralReject), 0x10U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::ServiceNotSupported), 0x11U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::SubFunctionNotSupported), 0x12U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::RequestOutOfRange), 0x31U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::AuthenticationRequired), 0x34U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::CertificateVerificationFailedInvalidTimePeriod), 0x50U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::OwnershipVerificationFailed), 0x58U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::DeAuthenticationFailed), 0x5DU);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::RequestCorrectlyReceivedResponsePending), 0x78U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::ServiceNotSupportedInActiveSession), 0x7FU);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::VehicleSpeedTooHigh), 0x88U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::ResourceTemporarilyNotAvailable), 0x94U);
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::NoProcessingNoResponse), 0xFFU);
}

// ── ToNegativeResponseCode Conversion Tests ─────────────────────────────────

TEST(UdsResponseCodeTest, ToNegativeResponseCodeValidCodes)
{
    const auto nrc_general_reject = ToNegativeResponseCode(0x10);
    ASSERT_TRUE(nrc_general_reject.has_value());
    EXPECT_EQ(nrc_general_reject.value(), NegativeResponseCode::GeneralReject);

    const auto nrc_security = ToNegativeResponseCode(0x33);
    ASSERT_TRUE(nrc_security.has_value());
    EXPECT_EQ(nrc_security.value(), NegativeResponseCode::SecurityAccessDenied);

    const auto nrc_no_response = ToNegativeResponseCode(0xFF);
    ASSERT_TRUE(nrc_no_response.has_value());
    EXPECT_EQ(nrc_no_response.value(), NegativeResponseCode::NoProcessingNoResponse);
}

TEST(UdsResponseCodeTest, ToNegativeResponseCodeInvalidCodesReturnsNullopt)
{
    // Out of bounds / reserved NRCs
    EXPECT_FALSE(ToNegativeResponseCode(0x00).has_value());
    EXPECT_FALSE(ToNegativeResponseCode(0x05).has_value());
    EXPECT_FALSE(ToNegativeResponseCode(0x1F).has_value());
    EXPECT_FALSE(ToNegativeResponseCode(0x100).has_value());
    EXPECT_FALSE(ToNegativeResponseCode(-1).has_value());
}

// ── Error Domain & MessageFor Tests ─────────────────────────────────────────

TEST(UdsErrorTest, MakeErrorCreatesValidErrorWithCorrectMessage)
{
    const auto err = MakeError(NegativeResponseCode::SecurityAccessDenied, "Additional contextual info");

    EXPECT_EQ(*err, static_cast<score::result::ErrorCode>(NegativeResponseCode::SecurityAccessDenied));
    EXPECT_EQ(err.Message(), "Security access denied");
    EXPECT_EQ(err.UserMessage(), "Additional contextual info");
}

TEST(UdsErrorTest, ErrorMessageForUnknownCodeReturnsUndefined)
{
    // Pass an illegal ErrorCode that isn't a valid NRC
    const score::result::Error err = MakeError(static_cast<NegativeResponseCode>(0x05));
    EXPECT_EQ(err.Message(), "Undefined ErrorCode!");
}

TEST(UdsErrorTest, MakeErrorWithDefaultEmptyUserMessageHasEmptyUserMessage)
{
    const auto err = MakeError(NegativeResponseCode::GeneralReject);
    EXPECT_EQ(err.UserMessage(), "");
}

// ── ToNegativeResponseCode — reserved/gap codes ─────────────────────────────

TEST(UdsResponseCodeTest, ToNegativeResponseCodeReservedGapCodesReturnNullopt)
{
    // Gaps in the ISO 14229-1:2020 Table A.1 (unassigned values between valid NRCs)
    EXPECT_FALSE(ToNegativeResponseCode(0x20).has_value());  // gap between 0x14 and 0x21
    EXPECT_FALSE(ToNegativeResponseCode(0x23).has_value());  // gap: 0x22 valid, 0x24 valid
    EXPECT_FALSE(ToNegativeResponseCode(0x30).has_value());  // gap between 0x26 and 0x31
    EXPECT_FALSE(ToNegativeResponseCode(0x32).has_value());  // gap between 0x31 and 0x33
    EXPECT_FALSE(ToNegativeResponseCode(0x74).has_value());  // gap between 0x73 and 0x78
    EXPECT_FALSE(ToNegativeResponseCode(0x80).has_value());  // gap between 0x7F and 0x81
    EXPECT_FALSE(ToNegativeResponseCode(0x8E).has_value());  // gap between 0x8D and 0x8F
    EXPECT_FALSE(ToNegativeResponseCode(0x95).has_value());  // gap between 0x94 and 0xFF
}

// ── VehicleManufacturerSpecificCNC & RangedNRC ─────────────────────────────

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCCompileTimeMinMax)
{
    const auto cnc_min = VehicleManufacturerSpecificCNC::FromValue<VehicleManufacturerSpecificCNC::kRangeMin>();
    EXPECT_EQ(cnc_min.Value(), VehicleManufacturerSpecificCNC::kRangeMin);

    const auto cnc_max = VehicleManufacturerSpecificCNC::FromValue<VehicleManufacturerSpecificCNC::kRangeMax>();
    EXPECT_EQ(cnc_max.Value(), VehicleManufacturerSpecificCNC::kRangeMax);
}

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCValueAccessor)
{
    static constexpr auto kCustomError = std::uint8_t{VehicleManufacturerSpecificCNC::kRangeMin + 1U};
    static constexpr auto kSomeError = std::uint8_t{VehicleManufacturerSpecificCNC::kRangeMin + 2U};

    const auto custom_error_a = VehicleManufacturerSpecificCNC::FromValue<kCustomError>();
    const auto custom_error_b = VehicleManufacturerSpecificCNC::FromValue<kCustomError>();
    const auto some_error = VehicleManufacturerSpecificCNC::FromValue<kSomeError>();

    EXPECT_EQ(custom_error_a.Value(), custom_error_b.Value());
    EXPECT_NE(custom_error_a.Value(), some_error.Value());
}

TEST(UdsResponseCodeTest, RuntimeFromInRangeReturnsValue)
{
    const auto result = VehicleManufacturerSpecificCNC::FromValue(VehicleManufacturerSpecificCNC::kRangeMin);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->Value(), VehicleManufacturerSpecificCNC::kRangeMin);
}

TEST(UdsResponseCodeTest, RuntimeFromBelowRangeReturnsNullopt)
{
    const auto result = VehicleManufacturerSpecificCNC::FromValue(
        static_cast<std::uint8_t>(VehicleManufacturerSpecificCNC::kRangeMin - 1U));
    EXPECT_FALSE(result.has_value());
}

TEST(UdsResponseCodeTest, RuntimeFromAboveRangeReturnsNullopt)
{
    const auto result = VehicleManufacturerSpecificCNC::FromValue(
        static_cast<std::uint8_t>(VehicleManufacturerSpecificCNC::kRangeMax + 1U));
    EXPECT_FALSE(result.has_value());
}

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCImplicitlyConvertsToNrc)
{
    const VehicleManufacturerSpecificCNC cnc = VehicleManufacturerSpecificCNC::FromValue<0xF0U>();
    const NegativeResponseCode nrc = cnc;  // implicit conversion
    EXPECT_EQ(static_cast<std::uint8_t>(nrc), 0xF0U);
}

// ── Result Integration Tests ──────────────────────────────────────────────

TEST(UdsResultTest, MakeUnexpectedWithNegativeResponseCode)
{
    Result<ByteVector> res = score::MakeUnexpected(NegativeResponseCode::SecurityAccessDenied);

    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.error().Message(), "Security access denied");
}

TEST(UdsResultTest, ResultVoidFailureWithNegativeResponseCode)
{
    auto execute_op = [](bool succeed) -> Result<void> {
        if (succeed)
        {
            return {};
        }
        return score::MakeUnexpected(NegativeResponseCode::ConditionsNotCorrect);
    };

    const auto success = execute_op(true);
    EXPECT_TRUE(success.has_value());

    const auto failure = execute_op(false);
    ASSERT_FALSE(failure.has_value());
    EXPECT_EQ(failure.error().Message(), "Conditions not correct");
}

}  // namespace score::mw::diag::uds
