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
/// @brief UDS Negative Response Code (See ISO 14229-1:2020, Table A.1),
///        RangedNrc template and VehicleManufacturerSpecificCNC alias.

#ifndef SCORE_MW_DIAG_UDS_NEGATIVE_RESPONSE_CODE_H
#define SCORE_MW_DIAG_UDS_NEGATIVE_RESPONSE_CODE_H

#include <cstdint>
#include <optional>

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
    RequestSequenceError = 0x24,
    NoResponseFromSubnetComponent = 0x25,
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

/// Generic wrapper for an NRC byte constrained to the compile-time range [kMin, kMax].
/// Use the named aliases below rather than instantiating this template directly.
template <std::uint8_t kMin, std::uint8_t kMax>
class RangedNrc
{
    static_assert(kMin <= kMax, "RangedNrc: kMin must be <= kMax");

  public:
    /// Inclusive lower bound of this NRC range.
    static constexpr std::uint8_t kRangeMin{kMin};
    /// Inclusive upper bound of this NRC range.
    static constexpr std::uint8_t kRangeMax{kMax};

    /// kVal must be in [kRangeMin, kRangeMax] — enforced at compile time.
    template <std::uint8_t kVal>
    [[nodiscard]] static constexpr RangedNrc FromValue() noexcept
    {
        static_assert(kVal >= kRangeMin && kVal <= kRangeMax, "RangedNrc value out of range");
        return RangedNrc{kVal};
    }

    /// Runtime overload: returns std::nullopt if val is outside [kRangeMin, kRangeMax].
    [[nodiscard]] static constexpr std::optional<RangedNrc> FromValue(std::uint8_t val) noexcept
    {
        if (val < kRangeMin || val > kRangeMax)
        {
            return std::nullopt;
        }
        return RangedNrc{val};
    }

    /// Returns the raw byte value of this NRC.
    [[nodiscard]] constexpr std::uint8_t Value() const noexcept
    {
        return value_;
    }

    /// Implicit conversion to NegativeResponseCode.
    /// Safe because [kRangeMin, kRangeMax] is verified at compile time to lie within the enum's value range.
    // NOLINTNEXTLINE(google-explicit-constructor) implicit conversion to `NegativeResponseCode` is always safe
    constexpr operator NegativeResponseCode() const noexcept
    {
        // According to 7.2/6 of the C++ standard, permitted values for an enumeration are all possible values
        // in the range [emin, emax] whereby emin and emax denote the smallest respectively largest enumerator
        // value. That's why the `static_cast` as performed below is safe and also well-defined behavior.
        static_assert(kRangeMax <= static_cast<std::uint8_t>(NegativeResponseCode::NoProcessingNoResponse),
                      "RangedNrc upper bound exceeds NegativeResponseCode range");
        static_assert(kRangeMin >= static_cast<std::uint8_t>(NegativeResponseCode::GeneralReject),
                      "RangedNrc lower bound is below NegativeResponseCode range");
        return static_cast<NegativeResponseCode>(value_);
    }

  private:
    constexpr explicit RangedNrc(std::uint8_t val) noexcept : value_{val} {}

    std::uint8_t value_{};
};

/// See ISO 14229-1:2020, Table A.1 — vehicleManufacturerSpecificConditionsNotCorrect (0xF0–0xFE).
using VehicleManufacturerSpecificCNC = RangedNrc<0xF0U, 0xFEU>;

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_NEGATIVE_RESPONSE_CODE_H
