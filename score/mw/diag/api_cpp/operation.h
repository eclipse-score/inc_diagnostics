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

/// @file operation.h
/// @brief SOVD operation API: ExecuteArguments, ExecutionStatus, ExecutionStatusDetails,
///        ExecutionEventKind, ExecutionEvent, ExecutionHandle, OperationMetadata
///        and the Operation interface.

#ifndef SCORE_MW_DIAG_OPERATION_H
#define SCORE_MW_DIAG_OPERATION_H

#include "score/mw/diag/result.h"
#include "score/mw/diag/sovd_error.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace score
{
namespace mw
{
namespace diag
{

/************************************/
/* General operation types          */
/************************************/

/// Alias for an operation's input/output user parameters.
using UserParameters = RequestMessagePayload;

/// cf. ISO 17978-3:2025 Section 7.14.6, Table 181
struct ExecuteArguments
{
    ReplyMessageEncoding              reply_encoding{ReplyMessageEncoding::binary()};
    std::optional<UserParameters>     user_parameters;
    std::optional<KeyValueAttributes> additional_attrs;
    std::optional<std::string>        proximity_response;
};

/// cf. ISO 17978-3:2025 Section 7.14.9, Table 194
using CustomCapability = std::string;

/// cf. ISO 17978-3:2025 Section 7.14.7, Table 186
using ExecutionId = std::string;

/************************************/
/* ExecutionStatus                  */
/************************************/

/// cf. ISO 17978-3:2025 Section 7.14.6, Table 185
enum class ExecutionStatus : std::uint8_t
{
    /// Raised when an unknown or unhandled SOVD capability is received.
    ///
    /// @note The name of the unhandled capability MUST be set in
    ///       ExecutionStatusDetails::last_executed_capability before reporting this status.
    ///       SimpleOperationAdapter does this automatically for HandleCustomCapability events.
    ///       Custom Operation implementations that report this status directly are
    ///       responsible for setting last_executed_capability themselves.
    UnsupportedCapability,
    Unknown,
    Scheduled,
    Running,
    Interrupted,
    Completed,
    Stopped,
    Failed,
};

/************************************/
/* ExecutionStatusDetails           */
/************************************/

/// cf. ISO 17978-3:2025 Section 7.14.7, Table 189
struct ExecutionStatusDetails
{
    /// Name of the last-executed or unhandled SOVD capability.
    ///
    /// @note MUST be populated whenever reporting ExecutionStatus::UnsupportedCapability.
    ///       Defaults to "n/a" if not set. SimpleOperationAdapter sets this automatically
    ///       from HandleCustomCapability events.
    std::string                         last_executed_capability{"n/a"};
    std::optional<std::uint8_t>         completion_percentage;
    std::optional<DiagnosticReply>      event_result;
    std::optional<std::vector<Error>>   exec_errors;

    /// Builder: set the completion percentage.
    ExecutionStatusDetails with_completion_percentage(std::uint8_t pct) && noexcept
    {
        completion_percentage = pct;
        return std::move(*this);
    }

    /// Builder: attach the reply data.
    ExecutionStatusDetails with_reply_data(DiagnosticReply reply) && noexcept
    {
        event_result = std::move(reply);
        return std::move(*this);
    }

    /// Builder: attach execution errors.
    ExecutionStatusDetails with_exec_errors(std::vector<Error> errors) && noexcept
    {
        exec_errors = std::move(errors);
        return std::move(*this);
    }
};

/************************************/
/* ExecutionEventKind               */
/************************************/

/// Kind type for events delivered to an operation's execution control loop.
/// Maps to the SOVD capability model (cf. ISO 17978-3:2025 Section 7.14.9).
enum class ExecutionEventKind : std::uint8_t
{
    HandleCustomCapability,  ///< HandleCustomCapability(CustomCapability).
                             ///< Use ExecutionEvent::for_custom_capability(name) factory to construct.
    ReportStatus,
    ControlGone,
    Interrupt,
    Resume,
    Reset,
    Stop,
    Error,   ///< Carries an error payload in ExecutionEvent::error_payload; accumulated into
             ///<   ExecutionStatusDetails::exec_errors on the final ControlGone event.
};

/// Returns the SOVD wire-format string for an ExecutionEventKind.
inline std::string_view to_string(ExecutionEventKind kind) noexcept
{
    switch (kind)
    {
        case ExecutionEventKind::HandleCustomCapability: return "";  ///< caller uses capability_name
        case ExecutionEventKind::ReportStatus:           return "status";
        case ExecutionEventKind::ControlGone:            return "unknown";
        case ExecutionEventKind::Interrupt:              return "freeze";
        case ExecutionEventKind::Resume:                 return "execute";
        case ExecutionEventKind::Reset:                  return "reset";
        case ExecutionEventKind::Stop:                   return "stop";
        case ExecutionEventKind::Error:                  return "error";
    }
    return "";
}

/************************************/
/* StatusReporter                   */
/************************************/

/// Callback wrapper used to report execution status back to the runtime.
struct StatusReporter
{
    /// The callback. May be empty (no-op) if no reporter was attached.
    std::function<void(ExecutionStatus, ExecutionStatusDetails)> callback;

    /// Invoke the reporter with the given status and details (if callback is set).
    void put(ExecutionStatus status, ExecutionStatusDetails details)
    {
        if (callback)
        {
            callback(status, std::move(details));
        }
    }
};

/************************************/
/* ExecutionEvent                   */
/************************************/

/// Events delivered to an operation's execution control loop.
/// Maps to the SOVD capability model (cf. ISO 17978-3:2025 Section 7.14.5, Table 178).
struct ExecutionEvent
{
    ExecutionEventKind           kind{ExecutionEventKind::Resume};
    std::optional<std::string>   capability_name;  ///< non-empty when kind == HandleCustomCapability
    std::optional<ExecuteArguments> args;
    StatusReporter               status_reporter;
    std::optional<Error>         error_payload;    ///< non-empty when kind == Error

    /// Factory: construct with a kind.
    static ExecutionEvent from_kind(ExecutionEventKind k) noexcept
    {
        return ExecutionEvent{k, std::nullopt, std::nullopt, {}};
    }

    /// Factory: construct a HandleCustomCapability event with the capability name guaranteed set.
    static ExecutionEvent for_custom_capability(std::string capability) noexcept
    {
        return ExecutionEvent{ExecutionEventKind::HandleCustomCapability,
                              std::move(capability), std::nullopt, {}};
    }

    /// Factory: construct an Error event carrying the given error payload.
    static ExecutionEvent for_error(Error err) noexcept
    {
        ExecutionEvent ev{ExecutionEventKind::Error};
        ev.error_payload = std::move(err);
        return ev;
    }

    /// Builder: attach execute arguments.
    ExecutionEvent with_args(ExecuteArguments a) && noexcept
    {
        args = std::move(a);
        return std::move(*this);
    }

    /// Builder: attach a status reporter callback.
    ExecutionEvent with_status_reporter(
        std::function<void(ExecutionStatus, ExecutionStatusDetails)> f) && noexcept
    {
        status_reporter.callback = std::move(f);
        return std::move(*this);
    }
};

/************************************/
/* ExecutionControl interface       */
/************************************/

/// Interface for receiving execution control events from the runtime.
/// cf. ISO 17978-3:2025 Sections 7.14.7 / 7.14.9
class ExecutionControl
{
  public:
    /// Returns the unique execution id for this execution instance.
    virtual const ExecutionId& exec_id() const noexcept = 0;

    /// Synchronously receive the next execution control event.
    /// Blocks until an event is available (runtime-defined behaviour for async contexts).
    virtual ExecutionEvent next_exec_event() = 0;

    ExecutionControl() = default;
    ExecutionControl(const ExecutionControl&) = delete;
    ExecutionControl(ExecutionControl&&) noexcept = delete;
    ExecutionControl& operator=(const ExecutionControl&) & = delete;
    ExecutionControl& operator=(ExecutionControl&&) & noexcept = delete;
    virtual ~ExecutionControl() noexcept = default;
};

/************************************/
/* ExecutionHandle                  */
/************************************/

/// The result type of an operation execution.
using ExecutionResult = Result<DiagnosticReply>;

/// Returned by Operation::execute().
/// Contains a callable that produces the ExecutionResult, and an optional initial
/// DiagnosticReply that shall be used as the response to the execute request.
struct ExecutionHandle
{
    /// Callable that produces the final execution result.
    /// The runtime invokes this after returning the initial reply to the client.
    std::function<ExecutionResult()> future;

    /// Optional initial reply to be sent back as the direct response to the execute request.
    std::optional<DiagnosticReply> reply;

    /// Builder: attach an initial reply.
    ExecutionHandle with_reply(DiagnosticReply r) && noexcept
    {
        reply = std::move(r);
        return std::move(*this);
    }
};

/************************************/
/* OperationMetadata                */
/************************************/

/// cf. ISO 17978-3:2025 Section 7.14.5, Table 176
struct OperationMetadata
{
    /// cf. ISO 17978-3:2025 Table 169: If true, execution requires proof of co-location.
    bool proximity_proof_required{false};

    /// cf. ISO 17978-3:2025 Table 169: If true, execution shall get performed synchronously.
    bool synchronous_execution{false};

    /// If true, executions shall not get performed at the same time in parallel.
    bool exclusive_execution{false};

    /// cf. ISO 17978-3:2025 Table 176: Required modes to execute the operation.
    /// Key is the mode-id, value lists the valid mode values.
    std::optional<InsertionOrderedMap<std::string, std::vector<std::string>>> supported_modes;
};

/************************************/
/* Operation interface              */
/************************************/

/// Interface representing a single SOVD operation that can be executed on an Entity.
/// cf. ISO 17978-3:2025 Section 7.14
class Operation
{
  public:
    /// Execute the operation with the given input arguments and execution control handle.
    ///
    /// @param input    Arguments for this execution, including reply encoding and user parameters.
    /// @param control  Handle for receiving lifecycle events from the runtime.
    /// @return Ok(ExecutionHandle) on successful start, Err(Error) if execution cannot begin.
    ///
    /// @note This method is conceptually async: on success the returned ExecutionHandle::future 
    /// will eventually produce the final execution result, and the runtime may deliver control events to the provided ExecutionControl handle at any time.
    /// Implementations that require async behaviour shall store the deferred work in a callable and return it inside the ExecutionHandle.

    [[nodiscard]] virtual Result<ExecutionHandle> execute(ExecuteArguments        input,
                                                         ExecutionControl&       control) = 0;

    Operation() = default;
    Operation(const Operation&) = delete;
    Operation(Operation&&) noexcept = delete;
    Operation& operator=(const Operation&) & = delete;
    Operation& operator=(Operation&&) & noexcept = delete;
    virtual ~Operation() noexcept = default;
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_OPERATION_H
