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

/// @file meta_data.h
/// @brief MetaData — contextual information associated with an incoming UDS diagnostic request.
///
/// Typed members are used for all currently-known fields. If the field set ever needs to grow
/// opaquely (e.g. vendor-specific keys), a GetValue(MetaInfoKey) -> std::optional<...> accessor
/// pattern (as used in ara::diag / AUTOSAR AP) can be introduced later without changing existing
/// handlers.

#ifndef SCORE_MW_DIAG_UDS_META_DATA_H
#define SCORE_MW_DIAG_UDS_META_DATA_H

#include <cstdint>
#include <optional>

namespace score::mw::diag::uds
{

/// UDS diagnostic session (ISO 14229-1:2020, Service 0x10 sub-functions).
enum class DiagnosticSession : std::uint8_t
{
    Default = 0x01,
    Programming = 0x02,
    Extended = 0x03,
    SafetySystem = 0x04,
};

/// Request addressing mode on the diagnostic link.
enum class AddressingMode : std::uint8_t
{
    Physical,
    Functional,
};

/// Contextual metadata provided by the diagnostic runtime to each service handler call.
///
/// Handlers may gate access based on the active session or security level, for example:
/// @code
/// if (!meta_data.security_level.has_value()) {
///     return score::MakeUnexpected(NegativeResponseCode::SecurityAccessDenied);
/// }
/// @endcode
struct MetaData
{
    /// Active diagnostic session at the time of the request.
    /// Gate session-restricted DIDs/routines on this.
    DiagnosticSession session{DiagnosticSession::Default};

    /// Currently unlocked SecurityAccess (0x27) level; std::nullopt when locked / not yet
    /// requested. Gate security-restricted read/write on this (return SecurityAccessDenied
    /// otherwise).
    /// The value is the requestSeed sub-function byte (always odd, e.g. 0x01, 0x03 ...).
    /// Per ISO 14229-1:2020 Section 10.4: "the security levels numbering is arbitrary and
    /// does not imply any relationship between the levels".
    std::optional<std::uint8_t> security_level{};

    /// Physical vs functional addressing of this request.
    AddressingMode addressing{AddressingMode::Physical};

    /// Diagnostic addresses of the tester (source), when known.
    std::optional<std::uint16_t> source_address{};

    /// Diagnostic address of this ECU (target), when known.
    std::optional<std::uint16_t> target_address{};
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_META_DATA_H
