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

#include "score/mw/diag/uds/negative_response_code.h"

namespace score::mw::diag::uds
{

namespace
{

[[nodiscard]] constexpr std::optional<std::string_view> GetMessageForNegativeResponseCode(
    const NegativeResponseCode response_code) noexcept
{
    switch (response_code)
    {
        case NegativeResponseCode::GeneralReject:
            return "General reject";
        case NegativeResponseCode::ServiceNotSupported:
            return "Service not supported";
        case NegativeResponseCode::SubFunctionNotSupported:
            return "Sub-function not supported";
        case NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat:
            return "Incorrect message length or invalid format";
        case NegativeResponseCode::ResponseTooLong:
            return "Response too long";
        case NegativeResponseCode::BusyRepeatRequest:
            return "Busy repeat request";
        case NegativeResponseCode::ConditionsNotCorrect:
            return "Conditions not correct";
        case NegativeResponseCode::RequestSequenceError:
            return "Request sequence error";
        case NegativeResponseCode::NoResponseFromSubnetComponent:
            return "No response from subnet component";
        case NegativeResponseCode::FailurePreventsExecutionOfRequestedAction:
            return "Failure prevents execution of requested action";
        case NegativeResponseCode::RequestOutOfRange:
            return "Request out of range";
        case NegativeResponseCode::SecurityAccessDenied:
            return "Security access denied";
        case NegativeResponseCode::AuthenticationRequired:
            return "Authentication required";
        case NegativeResponseCode::InvalidKey:
            return "Invalid key";
        case NegativeResponseCode::ExceededNumberOfAttempts:
            return "Exceeded number of attempts";
        case NegativeResponseCode::RequiredTimeDelayNotExpired:
            return "Required time delay not expired";
        case NegativeResponseCode::SecureDataTransmissionRequired:
            return "Secure data transmission required";
        case NegativeResponseCode::SecureDataTransmissionNotAllowed:
            return "Secure data transmission not allowed";
        case NegativeResponseCode::SecureDataVerificationFailed:
            return "Secure data verification failed";
        case NegativeResponseCode::CertificateVerificationFailedInvalidTimePeriod:
            return "Certificate verification failed - invalid time period";
        case NegativeResponseCode::CertificateVerificationFailedInvalidSignature:
            return "Certificate verification failed - invalid signature";
        case NegativeResponseCode::CertificateVerificationFailedInvalidChainOfTrust:
            return "Certificate verification failed - invalid chain of trust";
        case NegativeResponseCode::CertificateVerificationFailedInvalidType:
            return "Certificate verification failed - invalid type";
        case NegativeResponseCode::CertificateVerificationFailedInvalidFormat:
            return "Certificate verification failed - invalid format";
        case NegativeResponseCode::CertificateVerificationFailedInvalidContent:
            return "Certificate verification failed - invalid content";
        case NegativeResponseCode::CertificateVerificationFailedInvalidScope:
            return "Certificate verification failed - invalid scope";
        case NegativeResponseCode::CertificateVerificationFailedInvalidCertificate:
            return "Certificate verification failed - invalid certificate";
        case NegativeResponseCode::OwnershipVerificationFailed:
            return "Ownership verification failed";
        case NegativeResponseCode::ChallengeCalculationFailed:
            return "Challenge calculation failed";
        case NegativeResponseCode::SettingAccessRightsFailed:
            return "Setting access rights failed";
        case NegativeResponseCode::SessionKeyCreationOrDerivationFailed:
            return "Session key creation or derivation failed";
        case NegativeResponseCode::ConfigurationDataUsageFailed:
            return "Configuration data usage failed";
        case NegativeResponseCode::DeAuthenticationFailed:
            return "De-authentication failed";
        case NegativeResponseCode::UploadDownloadNotAccepted:
            return "Upload/download not accepted";
        case NegativeResponseCode::TransferDataSuspended:
            return "Transfer data suspended";
        case NegativeResponseCode::GeneralProgrammingFailure:
            return "General programming failure";
        case NegativeResponseCode::WrongBlockSequenceCounter:
            return "Wrong block sequence counter";
        case NegativeResponseCode::RequestCorrectlyReceivedResponsePending:
            return "Request correctly received - response pending";
        case NegativeResponseCode::SubFunctionNotSupportedInActiveSession:
            return "Sub-function not supported in active session";
        case NegativeResponseCode::ServiceNotSupportedInActiveSession:
            return "Service not supported in active session";
        case NegativeResponseCode::RpmTooHigh:
            return "RPM too high";
        case NegativeResponseCode::RpmTooLow:
            return "RPM too low";
        case NegativeResponseCode::EngineIsRunning:
            return "Engine is running";
        case NegativeResponseCode::EngineIsNotRunning:
            return "Engine is not running";
        case NegativeResponseCode::EngineRunTimeTooLow:
            return "Engine run time too low";
        case NegativeResponseCode::TemperatureTooHigh:
            return "Temperature too high";
        case NegativeResponseCode::TemperatureTooLow:
            return "Temperature too low";
        case NegativeResponseCode::VehicleSpeedTooHigh:
            return "Vehicle speed too high";
        case NegativeResponseCode::VehicleSpeedTooLow:
            return "Vehicle speed too low";
        case NegativeResponseCode::ThrottleOrPedalTooHigh:
            return "Throttle/pedal position too high";
        case NegativeResponseCode::ThrottleOrPedalTooLow:
            return "Throttle/pedal position too low";
        case NegativeResponseCode::TransmissionRangeNotInNeutral:
            return "Transmission range not in neutral";
        case NegativeResponseCode::TransmissionRangeNotInGear:
            return "Transmission range not in gear";
        case NegativeResponseCode::BrakeSwitchOrSwitchesNotClosed:
            return "Brake switch/switches not closed";
        case NegativeResponseCode::ShifterLeverNotInPark:
            return "Shifter lever not in park";
        case NegativeResponseCode::TorqueConverterClutchLocked:
            return "Torque converter clutch locked";
        case NegativeResponseCode::VoltageTooHigh:
            return "Voltage too high";
        case NegativeResponseCode::VoltageTooLow:
            return "Voltage too low";
        case NegativeResponseCode::ResourceTemporarilyNotAvailable:
            return "Resource temporarily not available";
        case NegativeResponseCode::NoProcessingNoResponse:
            return "No processing / no response";
        default:
            return std::nullopt;
    }
}

// Custom ErrorDomain for UDS Diagnostic errors
class DiagUdsErrorDomain final : public score::result::ErrorDomain
{
  public:
    std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override
    {
        const auto response_code = ToNegativeResponseCode(code);
        if (!response_code.has_value())
        {
            return "Undefined ErrorCode!";
        }

        return GetMessageForNegativeResponseCode(response_code.value()).value();
    }
};

const score::result::ErrorDomain& GetDiagUdsErrorDomain() noexcept
{
    static constexpr DiagUdsErrorDomain uds_diag_error_domain{};
    return uds_diag_error_domain;
}

}  // namespace

[[nodiscard]] std::optional<NegativeResponseCode> ToNegativeResponseCode(const score::result::ErrorCode& code) noexcept
{
    // Bounds check to avoid truncation bugs with out-of-range integer values
    if (code < 0 || code > 0xFF)
    {
        return std::nullopt;
    }

    const auto raw_val = static_cast<std::uint8_t>(code);
    const auto response_code = static_cast<NegativeResponseCode>(raw_val);
    return GetMessageForNegativeResponseCode(response_code).has_value()
               ? std::optional<NegativeResponseCode>{response_code}
               : std::nullopt;
}

score::result::Error MakeError(const NegativeResponseCode code, const std::string_view user_message) noexcept
{
    return score::result::Error{static_cast<score::result::ErrorCode>(code), GetDiagUdsErrorDomain(), user_message};
}

}  // namespace score::mw::diag::uds
