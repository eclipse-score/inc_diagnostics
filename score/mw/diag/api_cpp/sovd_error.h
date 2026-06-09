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

/// @file sovd_error.h
/// @brief SOVD error types: ErrorCode, GenericError, DataError.

#ifndef SCORE_MW_DIAG_SOVD_ERROR_H
#define SCORE_MW_DIAG_SOVD_ERROR_H

#include "score/mw/diag/result.h"

#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace score
{
namespace mw
{
namespace diag
{
namespace sovd
{

/************************************/
/* SOVD ErrorCode                   */
/************************************/

/// cf. ISO 17978-3:2025 Section 5.8.4, Table 18
enum class ErrorCode : std::uint8_t
{
    ErrorResponse,
    IncompleteRequest,
    InsufficientAccessRights,
    InvalidResponseContent,
    InvalidSignature,
    LockBroken,
    NotResponding,
    PreconditionNotFulfilled,
    SovdServerFailure,
    SovdServerMisconfigured,
    UpdateAutomatedNotSupported,
    UpdateExecutionInProgress,
    UpdatePreparationInProgress,
    UpdateProcessInProgress,
    VendorSpecific,
};

/// Returns the SOVD wire-format string for an ErrorCode.
inline std::string_view to_string(ErrorCode code) noexcept
{
    switch (code)
    {
        case ErrorCode::ErrorResponse:              return "error-response";
        case ErrorCode::IncompleteRequest:          return "incomplete-request";
        case ErrorCode::InsufficientAccessRights:   return "insufficient-access-rights";
        case ErrorCode::InvalidResponseContent:     return "invalid-response-content";
        case ErrorCode::InvalidSignature:           return "invalid-signature";
        case ErrorCode::LockBroken:                 return "lock-broken";
        case ErrorCode::NotResponding:              return "not-responding";
        case ErrorCode::PreconditionNotFulfilled:   return "precondition-not-fulfilled";
        case ErrorCode::SovdServerFailure:          return "sovd-server-failure";
        case ErrorCode::SovdServerMisconfigured:    return "sovd-server-misconfigured";
        case ErrorCode::UpdateAutomatedNotSupported:    return "update-automated-not-supported";
        case ErrorCode::UpdateExecutionInProgress:      return "update-execution-in-progress";
        case ErrorCode::UpdatePreparationInProgress:    return "update-preparation-in-progress";
        case ErrorCode::UpdateProcessInProgress:        return "update-process-in-progress";
        case ErrorCode::VendorSpecific:             return "vendor-specific";
    }
    return "unknown";
}

/************************************/
/* GenericError                     */
/************************************/

/// cf. ISO 17978-3:2025 Section 5.8.3, Table 16
struct GenericError
{
    std::string                       sovd_error;       ///< SOVD error code string (from ErrorCode or vendor)
    std::string                       message_text;     ///< human-readable message
    std::optional<std::string>        vendor_error;     ///< vendor-specific error identifier
    std::optional<std::string>        translation_id;   ///< optional i18n translation key
    std::optional<KeyValueAttributes> additional_attrs; ///< optional key-value metadata

    /// Factory: construct from a standard ErrorCode.
    static GenericError from_code(ErrorCode code, std::string message) noexcept
    {
        return GenericError{std::string{to_string(code)}, std::move(message),
                            std::nullopt, std::nullopt, std::nullopt};
    }

    /// Factory: construct a vendor-specific error.
    static GenericError from_vendor_error(std::string vendor_err, std::string message) noexcept
    {
        return GenericError{std::string{to_string(ErrorCode::VendorSpecific)}, std::move(message),
                            std::move(vendor_err), std::nullopt, std::nullopt};
    }

    /// Factory: construct from ErrorCode with a translation id.
    static GenericError from_code_with_translation(ErrorCode code,
                                                    std::string message,
                                                    std::string tid) noexcept
    {
        return GenericError{std::string{to_string(code)}, std::move(message),
                            std::nullopt, std::move(tid), std::nullopt};
    }

    /// Builder: attach a translation id.
    GenericError with_translation_id(std::string tid) && noexcept
    {
        translation_id = std::move(tid);
        return std::move(*this);
    }

    /// Builder: attach additional attributes.
    GenericError with_additional_attrs(KeyValueAttributes attrs) && noexcept
    {
        additional_attrs = std::move(attrs);
        return std::move(*this);
    }

    /// Mutating: insert a single additional attribute, creating the map if needed.
    void add_additional_attr(std::string key, std::string value)
    {
        if (!additional_attrs.has_value())
        {
            additional_attrs.emplace();
        }
        additional_attrs->emplace(std::move(key), std::move(value));
    }

    /// Mutating: replace all additional attributes.
    void set_additional_attrs(KeyValueAttributes attrs)
    {
        additional_attrs = std::move(attrs);
    }
};

/// Type alias matching Rust: pub type Error = GenericError
using Error = GenericError;

/************************************/
/* DataError                        */
/************************************/

/// cf. ISO 17978-3:2025 Section 5.8.3, Table 17
///
/// @note Per ISO 17978-3:2025 Section 5.8.3 Table 17, `path` shall contain
///       a JSON Pointer describing which element of the response is erroneous.
struct DataError
{
    std::string                path;   ///< JSON Pointer to erroneous element (may be empty)
    std::optional<GenericError> error; ///< associated generic error

    /// Factory: construct with a JSON Pointer path.
    static DataError from_path(std::string path) noexcept
    {
        return DataError{std::move(path), std::nullopt};
    }

    /// Factory: construct from a GenericError only (path left empty).
    static DataError from_error(GenericError err) noexcept
    {
        return DataError{{}, std::move(err)};
    }

    /// Factory: construct with both path and error.
    static DataError with_path_and_error(std::string path, GenericError err) noexcept
    {
        return DataError{std::move(path), std::move(err)};
    }

    /// Builder: attach or replace the associated GenericError.
    DataError with_error(GenericError err) && noexcept
    {
        error = std::move(err);
        return std::move(*this);
    }

    /// Builder: update the JSON Pointer path.
    DataError with_path(std::string p) && noexcept
    {
        path = std::move(p);
        return std::move(*this);
    }
};

}  // namespace sovd

/************************************/
/* ErrorCode                        */
/************************************/

/// Diagnostic error code encompassing protocol-specific error variants.
/// std::variant is the C++17 discriminated union that eliminates invalid states:
///   - std::variant<sovd::GenericError>        → SOVD error (always has a value)
///   - std::variant<uds::NegativeResponseCode> → UDS error (always has a value)
using ErrorCode = std::variant<sovd::GenericError, uds::NegativeResponseCode>;

/// Returns true when the error code is a SOVD error.
[[nodiscard]] inline bool is_sovd_error(const ErrorCode& code) noexcept
{
    return std::holds_alternative<sovd::GenericError>(code);
}

/// Returns true when the error code is a UDS error.
[[nodiscard]] inline bool is_uds_error(const ErrorCode& code) noexcept
{
    return std::holds_alternative<uds::NegativeResponseCode>(code);
}

/// Returns the SOVD GenericError. Precondition: is_sovd_error(code).
[[nodiscard]] inline const sovd::GenericError& get_sovd_error(const ErrorCode& code)
{
    return std::get<sovd::GenericError>(code);
}

/// Returns the UDS NegativeResponseCode. Precondition: is_uds_error(code).
[[nodiscard]] inline uds::NegativeResponseCode get_uds_nrc(const ErrorCode& code)
{
    return std::get<uds::NegativeResponseCode>(code);
}

/************************************/
/* Error                            */
/************************************/

/// Error type for any diagnostic action.
struct Error
{
    ErrorCode                          code;
    std::optional<ReplyMessagePayload> payload;

    /// Factory: construct from a SOVD GenericError.
    static Error from_error(sovd::GenericError error) noexcept
    {
        return Error{std::move(error), std::nullopt};
    }

    /// Factory: construct from a UDS NegativeResponseCode.
    static Error from_nrc(uds::NegativeResponseCode nrc) noexcept
    {
        return Error{nrc, std::nullopt};
    }

    /// Builder: attach a reply payload to this error.
    Error with_payload(ReplyMessagePayload p) && noexcept
    {
        payload = std::move(p);
        return std::move(*this);
    }

    /// Factory: mutex-poisoned sentinel.
    static Error mutex_poisoned() noexcept
    {
        return from_error(
            sovd::GenericError::from_code(
                sovd::ErrorCode::SovdServerFailure,
                "mutex acquisition failed unexpectedly"));
    }
};

/************************************/
/* Result<T> and helpers            */
/************************************/

/// Result type for any diagnostic action.
template <typename T>
using Result = std::variant<T, Error>;

/// Void-like result for write/blank operations.
using ResultBlank = Result<std::monostate>;

template <typename T>
bool is_ok(const Result<T>& r) noexcept { return std::holds_alternative<T>(r); }

template <typename T>
bool is_err(const Result<T>& r) noexcept { return std::holds_alternative<Error>(r); }

template <typename T>
const T& get_value(const Result<T>& r) { return std::get<T>(r); }

template <typename T>
const Error& get_error(const Result<T>& r) { return std::get<Error>(r); }

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_SOVD_ERROR_H
