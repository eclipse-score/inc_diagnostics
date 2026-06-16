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
///       The ExecutionControl reference passed to execute() MUST remain valid until
///       the returned ExecutionHandle's future has been invoked.

#ifndef SCORE_MW_DIAG_SIMPLE_OPERATION_H
#define SCORE_MW_DIAG_SIMPLE_OPERATION_H

#include "score/mw/diag/operation.h"

#include <memory>
#include <optional>

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
    virtual Result<ExecutionHandle> start(ExecuteArguments input) = 0;

    /// Stop the operation, optionally with input arguments.
    /// @return Ok(Some(DiagnosticReply)) if there is a stop reply,
    ///         Ok(None) if the operation stopped without a reply,
    ///         Err(Error) on failure.
    virtual Result<std::optional<DiagnosticReply>> stop(
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
///   - ControlGone: terminates the event loop and invokes the start result's future.
///   - Stop:        calls SimpleOperation::stop(), reports Stopped status, returns error.
///   - ReportStatus: calls completion_percentage() and reports Running status.
///   - Other kinds: reported as UnsupportedCapability status.
class SimpleOperationAdapter final : public Operation
{
  public:
    /// Constructs the adapter taking ownership of the wrapped SimpleOperation.
    explicit SimpleOperationAdapter(std::unique_ptr<SimpleOperation> op) noexcept
        : op_{std::move(op)}
    {}

    /// Execute the operation.
    ///
    /// @note The provided @p control reference must remain valid until the returned
    ///       ExecutionHandle::future has been invoked (C++ lifetime requirement equivalent
    ///       to Rust's Box<dyn ExecutionControl> ownership transfer).
    Result<ExecutionHandle> execute(ExecuteArguments input, ExecutionControl& control) override;

    SimpleOperationAdapter(const SimpleOperationAdapter&)           = delete;
    SimpleOperationAdapter(SimpleOperationAdapter&&) noexcept       = delete;
    SimpleOperationAdapter& operator=(const SimpleOperationAdapter&)  & = delete;
    SimpleOperationAdapter& operator=(SimpleOperationAdapter&&) &    noexcept = delete;
    ~SimpleOperationAdapter() noexcept override                     = default;

  private:
    std::unique_ptr<SimpleOperation> op_;
    std::optional<ExecutionId>       active_exec_id_;
};

/************************************/
/* SimpleOperationAdapter impl      */
/************************************/

inline Result<ExecutionHandle> SimpleOperationAdapter::execute(
    ExecuteArguments input, ExecutionControl& control)
{
    // Exclusive execution: reject if already running
    if (active_exec_id_.has_value())
    {
        return Result<ExecutionHandle>{score::unexpect,
                Error::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "operation is already executing"))};
    }
    active_exec_id_ = control.exec_id();

    // Delegate start to the wrapped SimpleOperation
    auto handle_result = op_->start(std::move(input));
    if (!handle_result.has_value())
    {
        active_exec_id_.reset();
        return handle_result;
    }

    ExecutionHandle exec_handle = std::move(*handle_result);

    // Wrap the op's future with a synchronous event-processing loop.
    // The lambda captures: control by reference (must outlive this future),
    // op_ by raw pointer (owned by this adapter which outlives the future),
    // active_exec_id_ by pointer (ditto).
    SimpleOperation*             op_raw  = op_.get();
    std::optional<ExecutionId>*  exec_id = &active_exec_id_;
    std::function<ExecutionResult()> op_future = std::move(exec_handle.future);

    exec_handle.future = [op_raw, exec_id, &control,
                          fut = std::move(op_future)]() mutable -> ExecutionResult
    {
        while (true)
        {
            ExecutionEvent event = control.next_exec_event();

            switch (event.kind)
            {
                case ExecutionEventKind::ControlGone:
                {
                    exec_id->reset();
                    return fut();
                }

                case ExecutionEventKind::Stop:
                {
                    auto stop_result = op_raw->stop(std::move(event.args));
                    exec_id->reset();
                    if (!stop_result.has_value())
                    {
                        return ExecutionResult{score::unexpect, stop_result.error()};
                    }
                    DiagnosticReply reply{};
                    if (stop_result->has_value())
                    {
                        reply = std::move(**stop_result);
                    }
                    event.status_reporter.put(ExecutionStatus::Stopped,
                        ExecutionStatusDetails{}.with_reply_data(std::move(reply)));
                    return ExecutionResult{score::unexpect,
                            Error::from_error(sovd::GenericError::from_code(
                                sovd::ErrorCode::ErrorResponse,
                                "operation was stopped"))};
                }

                case ExecutionEventKind::ReportStatus:
                {
                    auto details = ExecutionStatusDetails{};
                    const auto pct = op_raw->completion_percentage();
                    if (pct.has_value())
                    {
                        details = std::move(details).with_completion_percentage(*pct);
                    }
                    event.status_reporter.put(ExecutionStatus::Running, std::move(details));
                    break;
                }

                default:
                {
                    event.status_reporter.put(ExecutionStatus::UnsupportedCapability,
                                              ExecutionStatusDetails{});
                    break;
                }
            }
        }
    };

    return exec_handle;
}

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_SIMPLE_OPERATION_H
