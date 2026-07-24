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
/// @brief Unit tests for score/mw/diag/uds/negative_response_code.h
///        Covers: NegativeResponseCode enum values, VehicleManufacturerSpecificCNC.

#include "score/mw/diag/uds/negative_response_code.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds
{

// ── NegativeResponseCode ──────────────────────────────────────────────────

TEST(UdsResponseCodeTest, NrcGeneralRejectValue)
{
    EXPECT_EQ(static_cast<std::uint8_t>(NegativeResponseCode::GeneralReject), 0x10U);
}

TEST(UdsResponseCodeTest, NrcFullIso14229Coverage)
{
    // Spot-check key values from all sections of ISO 14229-1:2020 Table A.1
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

// ── VehicleManufacturerSpecificCNC ────────────────────────────────────────

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCValue)
{
    const auto cnc = VehicleManufacturerSpecificCNC::FromValue<VehicleManufacturerSpecificCNC::kRangeMin>();
    EXPECT_EQ(cnc.Value(), VehicleManufacturerSpecificCNC::kRangeMin);
}

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCMaxValue)
{
    const auto cnc = VehicleManufacturerSpecificCNC::FromValue<VehicleManufacturerSpecificCNC::kRangeMax>();
    EXPECT_EQ(cnc.Value(), VehicleManufacturerSpecificCNC::kRangeMax);
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

// ── RangedNrc runtime FromValue() ──────────────────────────────────────────

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

// ── RangedNrc implicit conversion ──────────────────────────────────────────

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCImplicitlyConvertsToNrc)
{
    const VehicleManufacturerSpecificCNC cnc = VehicleManufacturerSpecificCNC::FromValue<0xF0U>();
    const NegativeResponseCode nrc = cnc;  // implicit conversion
    EXPECT_EQ(static_cast<std::uint8_t>(nrc), 0xF0U);
}

}  // namespace score::mw::diag::uds
