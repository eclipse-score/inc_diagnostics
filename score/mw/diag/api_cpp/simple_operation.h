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

/// @file simple_operation.h
/// @brief SimpleOperation interface and SimpleOperationAdapter.
///
/// SimpleOperation is a simplified subset of Operation for operations that only
/// need start/stop/completion_percentage, without handling the full execution event loop.
///
/// SimpleOperationAdapter wraps a SimpleOperation and implements the full Operation
/// interface, bridging the simplified interface to the runtime's execution control model.
///
/// @note C++/Rust adaptation: Rust uses async streams (tokio::select!) for concurrent
///       event processing and future execution. In C++17, the event loop runs
///       synchronously inside ExecutionHandle::future.
///       Both the ExecutionControl reference AND the SimpleOperationAdapter instance
///       MUST remain valid until the returned ExecutionHandle::future has been invoked.

#ifndef SCORE_MW_DIAG_SIMPLE_OPERATION_H
#define SCORE_MW_DIAG_SIMPLE_OPERATION_H

#include "score/mw/diag/operation.h"

#include <memory>
#include <optional>
#include <vector>
#include <vector>

namespace score
{
namespace mw
{
namespace diag
{

/************************************/
/* SimpleOperation interface        */
/************************************/

/// Simplified operation interface for operations that follow a start/stop lifecycle.
///
/// Implementations need only provide start(), stop() and optionally completion_percentage().
/// The SimpleOperationAdapter bridges this to the full Operation interface automatically.
class SimpleOperation
{
  public:
    /// Start the operation with the given input arguments.
    /// @return Ok(ExecutionHandle) on success, Err(Error) on failure.
    [[nodiscard]] virtual Result<ExecutionHandle> start(ExecuteArguments input) = 0;

    /// Stop the operation, optionally with input arguments.
    /// @return Ok(Some(DiagnosticReply)) if there is a stop reply,
    ///         Ok(None) if the operation stopped without a reply,
    ///         Err(Error) on failure.
    [[nodiscard]] virtual Result<std::optional<DiagnosticReply>> stop(
        std::optional<ExecuteArguments> input) = 0;

    /// Optionally provide the current completion percentage (0..100).
    /// Returns std::nullopt if the completion percentage is not available.
    virtual std::optional<std::uint8_t> completion_percentage() const noexcept
    {
        return std::nullopt;
    }

    SimpleOperation()                                  = default;
    SimpleOperation(const SimpleOperation&)            = delete;
    SimpleOperation(SimpleOperation&&) noexcept        = delete;
    SimpleOperation& operator=(const SimpleOperation&)  & = delete;
    SimpleOperation& operator=(SimpleOperation&&) &     noexcept = delete;
    virtual ~SimpleOperation() noexcept                = default;
};

/************************************/
/* SimpleOperationAdapter           */
/************************************/

/// Adapts a SimpleOperation to the full Operation interface.
///
/// Enforces exclusive execution: a second call to execute() while a prior execution
/// is still running returns Err(Error::from_error(sovd::GenericError::from_code(
///   sovd::ErrorCode::PreconditionNotFulfilled, "operation is already executing"))).
///
/// Event handling inside ExecutionHandle::future (synchronous C++ adaptation of Rust's
/// tokio::select! event loop):
///   - ControlGone:  terminates the event loop and invokes the start result's future.
///                   Any accumulated Error events are reported via exec_errors in the
///                   ControlGone event's StatusReporter before returning.
///   - Stop:         calls SimpleOperation::stop(), reports Stopped status, returns error.
///   - ReportStatus: calls completion_percentage() and reports Running status.
///   - Error:        error payload is accumulated; attached to final exec_errors on ControlGone.
///   - Other kinds:  reported as UnsupportedCapability status.
///
/// @warning Not thread-safe. Concurrent calls to execute() from multiple threads cause data
///          races on the internal active_exec_id_ state. In the Rust equivalent, this type
///          uses Arc<Mutex<...>> for interior mutability. If thread safety is required,
///          guard all calls to execute() with an external mutex.
class SimpleOperationAdapter final : public Operation
{
  public:
    /// Constructs the adapter taking ownership of the wrapped SimpleOperation.
    explicit SimpleOperationAdapter(std::unique_ptr<SimpleOperation> op) noexcept
        : wrapped_operation_{std::move(op)}
    {}

    /// Execute the operation.
    ///
    /// @note Both the @p control reference AND this adapter instance MUST remain valid
    ///       until the returned ExecutionHandle::future has been invoked.
    [[nodiscard]] Result<ExecutionHandle> execute(ExecuteArguments input, ExecutionControl& control) override
    {
        if (active_exec_id_.has_value())
        {
            return Result<ExecutionHandle>{
                score::unexpect,
                Error::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "operation is already executing"))};
        }

        auto start_result = wrapped_operation_->start(std::move(input));
        if (!start_result.has_value())
        {
            return Result<ExecutionHandle>{score::unexpect, start_result.error()};
        }

        active_exec_id_ = control.exec_id();
        ExecutionHandle outer_handle{};
        ExecutionHandle inner_handle = std::move(start_result.value());

        outer_handle.future = [this, inner_handle = std::move(inner_handle),
                                &control]() mutable -> ExecutionResult
        {
            std::vector<Error> accumulated_errors;

            while (true)
            {
                ExecutionEvent event = control.next_exec_event();

                if (event.kind == ExecutionEventKind::ControlGone)
                {
                    if (!accumulated_errors.empty())
                    {
                        event.status_reporter.put(
                            ExecutionStatus::Failed,
                            ExecutionStatusDetails{}
                                .with_exec_errors(std::move(accumulated_errors)));
                    }
                    active_exec_id_ = std::nullopt;
                    return inner_handle.future();
                }

                if (event.kind == ExecutionEventKind::Stop)
                {
                    auto stop_result = wrapped_operation_->stop(std::move(event.args));
                    event.status_reporter.put(ExecutionStatus::Stopped, ExecutionStatusDetails{});
                    active_exec_id_ = std::nullopt;
                    (void)stop_result;
                    return Result<DiagnosticReply>{
                        score::unexpect,
                        Error::from_error(sovd::GenericError::from_code(
                            sovd::ErrorCode::ErrorResponse, "operation stopped"))};
                }

                if (event.kind == ExecutionEventKind::ReportStatus)
                {
                    auto pct = wrapped_operation_->completion_percentage();
                    ExecutionStatusDetails details{};
                    if (pct.has_value())
                    {
                        details = std::move(details).with_completion_percentage(*pct);
                    }
                    event.status_reporter.put(ExecutionStatus::Running, std::move(details));
                    continue;
                }

                if (event.kind == ExecutionEventKind::Error)
                {
                    if (event.error_payload.has_value())
                    {
                        accumulated_errors.push_back(std::move(*event.error_payload));
                    }
                    continue;
                }

                // All other kinds → UnsupportedCapability
                ExecutionStatusDetails unsupported{};
                unsupported.last_executed_capability =
                    event.capability_name.value_or(std::string{to_string(event.kind)});
                event.status_reporter.put(ExecutionStatus::UnsupportedCapability,
                                          std::move(unsupported));
            }
        };

        return outer_handle;
    }

    SimpleOperationAdapter(const SimpleOperationAdapter&)           = delete;
    SimpleOperationAdapter(SimpleOperationAdapter&&) noexcept       = delete;
    SimpleOperationAdapter& operator=(const SimpleOperationAdapter&)  & = delete;
    SimpleOperationAdapter& operator=(SimpleOperationAdapter&&) &    noexcept = delete;
    ~SimpleOperationAdapter() noexcept override                     = default;

  private:
    std::unique_ptr<SimpleOperation> wrapped_operation_;
    std::optional<ExecutionId>       active_exec_id_;
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_SIMPLE_OPERATION_H
