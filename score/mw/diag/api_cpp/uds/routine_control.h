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

/// @file routine_control.h
/// @brief UDS RoutineControl service interface and StartRoutine result type
///        (See ISO 14229-1:2020, Service 0x31).

#ifndef SCORE_MW_DIAG_API_CPP_UDS_ROUTINE_CONTROL_H
#define SCORE_MW_DIAG_API_CPP_UDS_ROUTINE_CONTROL_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"

#include <cstdint>
#include <functional>
#include <optional>

namespace score::mw::diag::uds
{

/************************************/
/* StartRoutine                     */
/************************************/

/// Returned by RoutineControl::start().
/// Contains the synchronous reply to the start request (optional) and a
/// callable that will be invoked asynchronously to await the routine result.
///
/// @note The Rust API wraps the async part in a Pin<Box<dyn Future>>. In C++,
///       the async future is modelled as a std::function returning Result<optional<ByteVector>>,
///       which the diagnostic runtime invokes at its discretion.
struct StartRoutine
{
    /// Optional synchronous reply payload to be returned as the response to
    /// the RoutineControl Start request (sub-function 0x01).
    std::optional<ByteVector> reply;

    /// Callable that produces the final routine result.
    /// The runtime invokes this and delivers the result via the execution status mechanism.
    /// Returns: Ok(Some(bytes)) on success with result data,
    ///          Ok(None)        on success without result data,
    ///          Err(Error)      on failure.
    std::function<Result<std::optional<ByteVector>>()> result_provider;
};

/************************************/
/* RoutineControl                   */
/************************************/

/// UDS RoutineControl service (See ISO 14229-1:2020, Service 0x31).
///
/// NOTE: RequestResults (sub-function 0x03) is handled implicitly by the
///       diagnostic runtime via the execution status reporting mechanism —
///       implementors do not need to override it.
class RoutineControl
{
  public:
    /// Start the routine (sub-function 0x01).
    /// @param input  Optional input data accompanying the start request.
    /// @return Ok(StartRoutine) on success, Err(Error) on failure.
    [[nodiscard]] virtual Result<StartRoutine> Start(std::optional<ByteView> input) = 0;

    /// Stop the routine (sub-function 0x02).
    /// @param input  Optional input data accompanying the stop request.
    /// @return Ok(optional<ByteVector>) on success (byte vector is the stop reply data),
    ///         Err(Error) on failure.
    [[nodiscard]] virtual Result<std::optional<ByteVector>> Stop(std::optional<ByteView> input) = 0;

    /// Optionally provide the current routine completion percentage (0..100).
    /// Returns std::nullopt if completion percentage is not available.
    virtual std::optional<std::uint8_t> CompletionPercentage() const noexcept
    {
        return std::nullopt;
    }

    RoutineControl() = default;
    RoutineControl(const RoutineControl&) = delete;
    RoutineControl(RoutineControl&&) noexcept = delete;
    RoutineControl& operator=(const RoutineControl&) & = delete;
    RoutineControl& operator=(RoutineControl&&) & noexcept = delete;
    virtual ~RoutineControl() noexcept;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_API_CPP_UDS_ROUTINE_CONTROL_H
