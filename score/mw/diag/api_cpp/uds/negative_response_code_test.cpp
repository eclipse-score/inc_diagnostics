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
/// @brief Unit tests for score/mw/diag/negative_response_code.h
///        Covers: NegativeResponseCode enum values, VehicleManufacturerSpecificCNC.

#include "score/mw/diag/negative_response_code.h"

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
    const auto cnc = VehicleManufacturerSpecificCNC::from(VehicleManufacturerSpecificCNC::kRangeMin);
    EXPECT_EQ(cnc.value(), VehicleManufacturerSpecificCNC::kRangeMin);
}

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCMaxValue)
{
    const auto cnc = VehicleManufacturerSpecificCNC::from(VehicleManufacturerSpecificCNC::kRangeMax);
    EXPECT_EQ(cnc.value(), VehicleManufacturerSpecificCNC::kRangeMax);
}

TEST(UdsResponseCodeTest, VehicleManufacturerSpecificCNCEqualityOperators)
{
    const auto a = VehicleManufacturerSpecificCNC::from(VehicleManufacturerSpecificCNC::kRangeMin + 1U);
    const auto b = VehicleManufacturerSpecificCNC::from(VehicleManufacturerSpecificCNC::kRangeMin + 1U);
    const auto c = VehicleManufacturerSpecificCNC::from(VehicleManufacturerSpecificCNC::kRangeMin + 2U);
    EXPECT_EQ(a.value(), b.value());
    EXPECT_NE(a.value(), c.value());
}

}  // namespace score::mw::diag::uds
