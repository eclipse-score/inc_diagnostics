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
/// @brief UDS RoutineControl service interface (See ISO 14229-1:2020, Service 0x31).
///
/// Provides two levels of abstraction:
///   - `RoutineControl`       — full interface with `MetaData` and cancellation support.
///   - `SimpleRoutineControl` — simplified adapter for non-blocking, context-free routines.

#ifndef SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_H
#define SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/meta_data.h"

#include <score/stop_token.hpp>

#include <cstdint>
#include <optional>
#include <future>

namespace score::mw::diag::uds
{

/// UDS RoutineControl service (See ISO 14229-1:2020, Service 0x31).
///
/// Implement `Start()`, `Stop()`, and `RequestResults()` for every routine.
///
/// Full interface — receives request `MetaData` (session, security level, etc.) and
/// a `stop_token` for cooperative cancellation of long-running operations.
class RoutineControl
{
  public:
    /// Start the routine (sub-function 0x01).
    /// @param input       Non-owning view of the raw input bytes accompanying the start request.
    /// @param meta_data   Context provided by the diagnostic runtime for this request.
    /// @param stop_token  Token that becomes stopped if the runtime cancels the request.
    /// @return std::future<Result<ByteVector>> wrapping serialized routineStatusRecord bytes on success
    ///         (empty if the routine produces no start reply data); NegativeResponseCode on failure.
    [[nodiscard]] virtual std::future<Result<ByteVector>> Start(ByteView input,
                                                                const MetaData& meta_data,
                                                                score::cpp::stop_token stop_token) = 0;

    /// Stop the routine (sub-function 0x02).
    /// @param input       Non-owning view of the raw input bytes accompanying the stop request.
    /// @param meta_data   Context provided by the diagnostic runtime for this request.
    /// @param stop_token  Token that becomes stopped if the runtime cancels the request.
    /// @return std::future<Result<ByteVector>> wrapping serialized routineStatusRecord bytes on success
    ///         (empty if the routine produces no stop reply data); NegativeResponseCode on failure.
    [[nodiscard]] virtual std::future<Result<ByteVector>> Stop(ByteView input,
                                                               const MetaData& meta_data,
                                                               score::cpp::stop_token stop_token) = 0;

    /// Request the routine results (sub-function 0x03).
    /// @param input       Non-owning view of the raw input bytes accompanying the request.
    /// @param meta_data   Context provided by the diagnostic runtime for this request.
    /// @param stop_token  Token that becomes stopped if the runtime cancels the request.
    /// @return std::future<Result<ByteVector>> wrapping serialized routineStatusRecord bytes on success
    ///         (empty if no result data); NegativeResponseCode on failure.
    [[nodiscard]] virtual std::future<Result<ByteVector>> RequestResults(ByteView input,
                                                                         const MetaData& meta_data,
                                                                         score::cpp::stop_token stop_token) = 0;

    /// Optionally provide the current routine completion percentage.
    /// @return A value in [0, 100] representing the completion percentage,
    ///         or `std::nullopt` if the routine does not support progress reporting.
    ///
    /// @note Required once an SOVD-capable diagnostic stack is available
    ///       that wraps legacy uds::RoutineControl implementations for backward compatibility.
    [[nodiscard]] virtual std::optional<std::uint8_t> CompletionPercentage() const noexcept
    {
        return std::nullopt;
    }

    virtual ~RoutineControl() noexcept = default;
};

/// Simplified adapter for `RoutineControl` (must be non-blocking!)
///
/// Implement the simple variants — the adapter bridges them to the full
/// `RoutineControl` interface by ignoring `meta_data` and `stop_token`.
class SimpleRoutineControl : public RoutineControl
{
  public:
    /// Start the routine in a fast and non-blocking manner.
    /// @param input  Non-owning view of the raw input bytes accompanying the start request.
    /// @return Result<ByteVector> wrapping serialized routineStatusRecord bytes on success;
    ///         NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<ByteVector> Start(ByteView input) = 0;

    /// Stop the routine in a fast and non-blocking manner.
    /// @param input  Non-owning view of the raw input bytes accompanying the stop request.
    /// @return Result<ByteVector> wrapping serialized routineStatusRecord bytes on success;
    ///         NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<ByteVector> Stop(ByteView input) = 0;

    /// Request the routine results in a fast and non-blocking manner.
    /// @param input  Non-owning view of the raw input bytes accompanying the request.
    /// @return Result<ByteVector> wrapping serialized routineStatusRecord bytes on success;
    ///         NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<ByteVector> RequestResults(ByteView input) = 0;

    virtual ~SimpleRoutineControl() noexcept = default;

  private:
    std::future<Result<ByteVector>> Start(ByteView input, const MetaData& /*meta_data*/, score::cpp::stop_token /*stop_token*/) final
    {
        std::promise<Result<ByteVector>> promise;
        promise.set_value(Start(input));
        return promise.get_future();
    }

    std::future<Result<ByteVector>> Stop(ByteView input, const MetaData& /*meta_data*/, score::cpp::stop_token /*stop_token*/) final
    {
        std::promise<Result<ByteVector>> promise;
        promise.set_value(Stop(input));
        return promise.get_future();
    }

    std::future<Result<ByteVector>> RequestResults(ByteView input,
                                      const MetaData& /*meta_data*/,
                                      score::cpp::stop_token /*stop_token*/) final
    {
        std::promise<Result<ByteVector>> promise;
        promise.set_value(RequestResults(input));
        return promise.get_future();
    }
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_H
