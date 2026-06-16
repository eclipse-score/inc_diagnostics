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

#ifndef SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_H
#define SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_H

#include "score/move_only_function.hpp"
#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"

#include <cstdint>
#include <optional>

namespace score::mw::diag::uds
{

/// Result type for RoutineControl::Stop() and the async result_provider.
using StopResult = Result<std::optional<ByteVector>>;

/************************************/
/* StartRoutine                     */
/************************************/

/// Returned by RoutineControl::start().
/// Contains the synchronous reply to the start request (optional) and a
/// callable that will be invoked asynchronously to await the routine result.
///
/// @note The async result is modelled as a score::cpp::move_only_function returning StopResult,
///       which the diagnostic runtime invokes at its discretion.
struct StartRoutine
{
    /// Optional synchronous reply payload to be returned as the response to
    /// the RoutineControl Start request (sub-function 0x01).
    std::optional<ByteVector> reply;

    /// Callable that produces the final routine result.
    /// The runtime invokes this and delivers the result via the execution status mechanism.
    /// @return StopResult: Some(bytes) on success with data, std::nullopt on success without data,
    ///         NegativeResponseCode on failure.
    score::cpp::move_only_function<StopResult()> result_provider;
};

/// Result type for RoutineControl::Start().
using StartResult = Result<StartRoutine>;

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
    /// @return StartResult on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual StartResult Start(std::optional<ByteView> input) = 0;

    /// Stop the routine (sub-function 0x02).
    /// @param input  Optional input data accompanying the stop request.
    /// @return StopResult on success (byte vector is the stop reply data),
    ///         NegativeResponseCode on failure.
    [[nodiscard]] virtual StopResult Stop(std::optional<ByteView> input) = 0;

    /// Optionally provide the current routine completion percentage (0..100).
    /// Returns std::nullopt if completion percentage is not available.
    [[nodiscard]] virtual std::optional<std::uint8_t> CompletionPercentage() const noexcept
    {
        return std::nullopt;
    }

    RoutineControl() = default;
    virtual ~RoutineControl() noexcept = default;
    RoutineControl(const RoutineControl&) = delete;
    RoutineControl(RoutineControl&&) noexcept = delete;
    RoutineControl& operator=(const RoutineControl&) & = delete;
    RoutineControl& operator=(RoutineControl&&) & noexcept = delete;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_H
