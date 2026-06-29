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

/// @file negative_response_code.h
/// @brief UDS Negative Response Code (See ISO 14229-1:2020, Table A.1) and
///        VehicleManufacturerSpecificCNC wrapper.

#ifndef SCORE_MW_DIAG_API_CPP_UDS_NEGATIVE_RESPONSE_CODE_H
#define SCORE_MW_DIAG_API_CPP_UDS_NEGATIVE_RESPONSE_CODE_H

#include <cassert>
#include <cstdint>

namespace score::mw::diag::uds
{

/// See ISO 14229-1:2020, Table A.1
enum class NegativeResponseCode : std::uint8_t
{
    GeneralReject = 0x10,
    ServiceNotSupported = 0x11,
    SubFunctionNotSupported = 0x12,
    IncorrectMessageLengthOrInvalidFormat = 0x13,
    ResponseTooLong = 0x14,
    BusyRepeatRequest = 0x21,
    ConditionsNotCorrect = 0x22,
    NoResponseFromSubnetComponent = 0x23,
    RequestSequenceError = 0x24,
    NoResponseFromSubNetComponent = 0x25,
    FailurePreventsExecutionOfRequestedAction = 0x26,
    RequestOutOfRange = 0x31,
    SecurityAccessDenied = 0x33,
    AuthenticationRequired = 0x34,
    InvalidKey = 0x35,
    ExceededNumberOfAttempts = 0x36,
    RequiredTimeDelayNotExpired = 0x37,
    SecureDataTransmissionRequired = 0x38,
    SecureDataTransmissionNotAllowed = 0x39,
    SecureDataVerificationFailed = 0x3A,
    CertificateVerificationFailedInvalidTimePeriod = 0x50,
    CertificateVerificationFailedInvalidSignature = 0x51,
    CertificateVerificationFailedInvalidChainOfTrust = 0x52,
    CertificateVerificationFailedInvalidType = 0x53,
    CertificateVerificationFailedInvalidFormat = 0x54,
    CertificateVerificationFailedInvalidContent = 0x55,
    CertificateVerificationFailedInvalidScope = 0x56,
    CertificateVerificationFailedInvalidCertificate = 0x57,
    OwnershipVerificationFailed = 0x58,
    ChallengeCalculationFailed = 0x59,
    SettingAccessRightsFailed = 0x5A,
    SessionKeyCreationOrDerivationFailed = 0x5B,
    ConfigurationDataUsageFailed = 0x5C,
    DeAuthenticationFailed = 0x5D,
    UploadDownloadNotAccepted = 0x70,
    TransferDataSuspended = 0x71,
    GeneralProgrammingFailure = 0x72,
    WrongBlockSequenceCounter = 0x73,
    RequestCorrectlyReceivedResponsePending = 0x78,
    SubFunctionNotSupportedInActiveSession = 0x7E,
    ServiceNotSupportedInActiveSession = 0x7F,
    RpmTooHigh = 0x81,
    RpmTooLow = 0x82,
    EngineIsRunning = 0x83,
    EngineIsNotRunning = 0x84,
    EngineRunTimeTooLow = 0x85,
    TemperatureTooHigh = 0x86,
    TemperatureTooLow = 0x87,
    VehicleSpeedTooHigh = 0x88,
    VehicleSpeedTooLow = 0x89,
    ThrottleOrPedalTooHigh = 0x8A,
    ThrottleOrPedalTooLow = 0x8B,
    TransmissionRangeNotInNeutral = 0x8C,
    TransmissionRangeNotInGear = 0x8D,
    BrakeSwitchOrSwitchesNotClosed = 0x8F,
    ShifterLeverNotInPark = 0x90,
    TorqueConverterClutchLocked = 0x91,
    VoltageTooHigh = 0x92,
    VoltageTooLow = 0x93,
    ResourceTemporarilyNotAvailable = 0x94,
    NoProcessingNoResponse = 0xFF,
};

/// See ISO 14229-1:2020, Table A.1 (vehicleManufacturerSpecificConditionsNotCorrect).
/// Wraps an NRC byte in the manufacturer-specific range 0xF0–0xFE.
class VehicleManufacturerSpecificCNC
{
  public:
    /// Inclusive lower bound of the manufacturer-specific NRC range (ISO 14229-1:2020).
    static constexpr std::uint8_t kRangeMin{0xF0U};
    /// Inclusive upper bound of the manufacturer-specific NRC range (ISO 14229-1:2020).
    static constexpr std::uint8_t kRangeMax{0xFEU};

    /// Factory: construct from a validated byte value.
    /// Precondition: value must be in [kRangeMin, kRangeMax].
    static VehicleManufacturerSpecificCNC from(std::uint8_t value) noexcept
    {
        assert(value >= kRangeMin && value <= kRangeMax && "VehicleManufacturerSpecificCNC out of range");
        return VehicleManufacturerSpecificCNC{value};
    }

    std::uint8_t value() const noexcept
    {
        return value_;
    }

    bool operator==(const VehicleManufacturerSpecificCNC& other) const noexcept
    {
        return value_ == other.value_;
    }

    bool operator!=(const VehicleManufacturerSpecificCNC& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    explicit VehicleManufacturerSpecificCNC(std::uint8_t v) noexcept : value_{v} {}

    std::uint8_t value_{kRangeMin};
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_API_CPP_UDS_NEGATIVE_RESPONSE_CODE_H
