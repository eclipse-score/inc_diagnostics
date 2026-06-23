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

/// @file simple_operation_test.cpp
/// @brief Unit tests for score/mw/diag/simple_operation.h
///        Covers: SimpleOperation interface via mock,
///                SimpleOperationAdapter (exclusive execution, ControlGone, Stop, ReportStatus).

#include "score/mw/diag/simple_operation_mock.h"

#include "score/mw/diag/operation_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

namespace score::mw::diag
{

// ── SimpleOperationMock interface ─────────────────────────────────────────

TEST(SimpleOperationTest, StartReturnsOkHandle)
{
    SimpleOperationMock mock{};
    const ExecutionHandle expected_handle{
        []() -> ExecutionResult { return DiagnosticReply{}; }};

    EXPECT_CALL(mock, start(_))
        .WillOnce(Return(SimpleOperationMock::StartResult{expected_handle}));

    auto result = mock.start(ExecuteArguments{});
    EXPECT_TRUE(is_ok(result));
}

TEST(SimpleOperationTest, StartReturnsError)
{
    SimpleOperationMock mock{};
    EXPECT_CALL(mock, start(_))
        .WillOnce(Return(SimpleOperationMock::StartResult{
            score::unexpect,
            Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::PreconditionNotFulfilled, "precondition-not-fulfilled"))}));

    auto result = mock.start(ExecuteArguments{});
    EXPECT_TRUE(is_err(result));
    EXPECT_TRUE(is_sovd_error(get_error(result).code));
    EXPECT_EQ(get_sovd_error(get_error(result).code).sovd_error, "precondition-not-fulfilled");
}

TEST(SimpleOperationTest, StopReturnsNoReply)
{
    SimpleOperationMock mock{};
    EXPECT_CALL(mock, stop(_))
        .WillOnce(Return(SimpleOperationMock::StopResult{std::optional<DiagnosticReply>{}}));

    auto result = mock.stop(std::nullopt);
    ASSERT_TRUE(is_ok(result));
    EXPECT_FALSE(get_value(result).has_value());
}

TEST(SimpleOperationTest, StopReturnsReply)
{
    SimpleOperationMock mock{};
    DiagnosticReply reply{};
    reply.message_payload = ReplyMessagePayload::from_string("done");
    EXPECT_CALL(mock, stop(_))
        .WillOnce(Return(SimpleOperationMock::StopResult{
            std::optional<DiagnosticReply>{std::move(reply)}}));

    auto result = mock.stop(std::nullopt);
    ASSERT_TRUE(is_ok(result));
    ASSERT_TRUE(get_value(result).has_value());
    EXPECT_EQ(get_value(result)->message_payload->text_data, "done");
}

TEST(SimpleOperationTest, CompletionPercentageMockReturnsNullopt)
{
    SimpleOperationMock mock{};
    EXPECT_CALL(mock, completion_percentage())
        .WillOnce(Return(std::optional<std::uint8_t>{}));

    EXPECT_FALSE(mock.completion_percentage().has_value());
}

// ── SimpleOperationAdapter: ControlGone terminates loop ───────────────────

TEST(SimpleOperationAdapterTest, ControlGoneRunsOpFuture)
{
    bool future_called = false;

    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([&](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{
                [&future_called]() -> ExecutionResult
                {
                    future_called = true;
                    return DiagnosticReply{};
                }};
        });

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-1";
    EXPECT_CALL(control, exec_id())
        .WillOnce(ReturnRef(exec_id));
    EXPECT_CALL(control, next_exec_event())
        .WillOnce(Return(ExecutionEvent::from_kind(ExecutionEventKind::ControlGone)));

    SimpleOperationAdapter adapter{std::move(op)};
    auto result = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(result));

    // Invoke the future — it processes events and eventually calls the op's future
    const auto exec_result = get_value(result).future();
    EXPECT_TRUE(is_ok(exec_result));
    EXPECT_TRUE(future_called);
}

// ── SimpleOperationAdapter: Stop event ───────────────────────────────────

TEST(SimpleOperationAdapterTest, StopEventCallsStopAndReturnsError)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });
    EXPECT_CALL(*op, stop(_))
        .WillOnce(Return(SimpleOperationMock::StopResult{
            std::optional<DiagnosticReply>{std::nullopt}}));

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-2";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));

    ExecutionStatus reported_status = ExecutionStatus::Unknown;
    auto stop_event =
        ExecutionEvent::from_kind(ExecutionEventKind::Stop)
            .with_status_reporter([&](ExecutionStatus s, ExecutionStatusDetails)
            {
                reported_status = s;
            });

    EXPECT_CALL(control, next_exec_event())
        .WillOnce(Return(std::move(stop_event)));

    SimpleOperationAdapter adapter{std::move(op)};
    auto result = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(result));

    const auto exec_result = get_value(result).future();
    EXPECT_TRUE(is_err(exec_result));
    EXPECT_EQ(reported_status, ExecutionStatus::Stopped);
    EXPECT_EQ(get_sovd_error(get_error(exec_result).code).sovd_error, "error-response");
}

// ── SimpleOperationAdapter: ReportStatus event ───────────────────────────

TEST(SimpleOperationAdapterTest, ReportStatusThenControlGone)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });
    EXPECT_CALL(*op, completion_percentage())
        .WillOnce(Return(std::optional<std::uint8_t>{42U}));

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-3";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));

    ExecutionStatus reported_status = ExecutionStatus::Unknown;
    std::uint8_t    reported_pct    = 0U;

    auto report_event =
        ExecutionEvent::from_kind(ExecutionEventKind::ReportStatus)
            .with_status_reporter([&](ExecutionStatus s, ExecutionStatusDetails d)
            {
                reported_status = s;
                if (d.completion_percentage.has_value())
                {
                    reported_pct = *d.completion_percentage;
                }
            });

    EXPECT_CALL(control, next_exec_event())
        .WillOnce(Return(std::move(report_event)))
        .WillOnce(Return(ExecutionEvent::from_kind(ExecutionEventKind::ControlGone)));

    SimpleOperationAdapter adapter{std::move(op)};
    auto result = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(result));

    const auto exec_result = get_value(result).future();
    EXPECT_TRUE(is_ok(exec_result));
    EXPECT_EQ(reported_status, ExecutionStatus::Running);
    EXPECT_EQ(reported_pct,    42U);
}

// ── SimpleOperationAdapter: exclusive execution ───────────────────────────

TEST(SimpleOperationAdapterTest, ExclusiveExecutionRejectsConcurrentCall)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-4";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));

    SimpleOperationAdapter adapter{std::move(op)};

    // First execute — succeeds (future not yet invoked, so adapter still "running")
    auto first = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(first));

    // Second execute while active_exec_id is set — must return error
    auto second = adapter.execute(ExecuteArguments{}, control);
    EXPECT_TRUE(is_err(second));
    EXPECT_EQ(get_sovd_error(get_error(second).code).sovd_error, "precondition-not-fulfilled");
}

// ── SimpleOperationAdapter: start error propagated ────────────────────────

TEST(SimpleOperationAdapterTest, StartErrorPropagated)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce(Return(SimpleOperationMock::StartResult{
            score::unexpect,
            Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::NotResponding, "not-responding"))}));

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-5";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));

    SimpleOperationAdapter adapter{std::move(op)};
    auto result = adapter.execute(ExecuteArguments{}, control);
    EXPECT_TRUE(is_err(result));
    EXPECT_EQ(get_sovd_error(result.error().code).sovd_error, "not-responding");
}

// ── SimpleOperationAdapter: stop with a reply payload ─────────────────────────────

TEST(SimpleOperationAdapterTest, StopEventWithReplyAttachesPayloadToStatus)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });

    DiagnosticReply stop_reply{};
    stop_reply.message_payload = ReplyMessagePayload::from_string("stopped-ok");
    EXPECT_CALL(*op, stop(_))
        .WillOnce(Return(SimpleOperationMock::StopResult{
            std::optional<DiagnosticReply>{std::move(stop_reply)}}));

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-stop-reply";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));

    ExecutionStatus     reported_status = ExecutionStatus::Unknown;
    std::string         reported_text;
    auto stop_event =
        ExecutionEvent::from_kind(ExecutionEventKind::Stop)
            .with_status_reporter([&](ExecutionStatus s, ExecutionStatusDetails d)
            {
                reported_status = s;
                if (d.event_result.has_value() &&
                    d.event_result->message_payload.has_value())
                {
                    reported_text = d.event_result->message_payload->text_data;
                }
            });

    EXPECT_CALL(control, next_exec_event())
        .WillOnce(Return(std::move(stop_event)));

    SimpleOperationAdapter adapter{std::move(op)};
    auto result = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(result));

    const auto exec_result = get_value(result).future();
    EXPECT_TRUE(is_err(exec_result));
    EXPECT_EQ(reported_status, ExecutionStatus::Stopped);
    EXPECT_EQ(reported_text,   "stopped-ok");
}

// ── SimpleOperationAdapter: stop() returning Err ──────────────────────────────────

TEST(SimpleOperationAdapterTest, StopEventErrorPropagated)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });
    EXPECT_CALL(*op, stop(_))
        .WillOnce(Return(SimpleOperationMock::StopResult{
            score::unexpect,
            Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::NotResponding, "not-responding"))}));

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-stop-err";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));
    EXPECT_CALL(control, next_exec_event())
        .WillOnce(Return(ExecutionEvent::from_kind(ExecutionEventKind::Stop)));

    SimpleOperationAdapter adapter{std::move(op)};
    auto result = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(result));

    const auto exec_result = get_value(result).future();
    EXPECT_TRUE(is_err(exec_result));
    EXPECT_EQ(get_sovd_error(get_error(exec_result).code).sovd_error, "not-responding");
}

// ── SimpleOperationAdapter: HandleCustomCapability sets capability name in details ───

TEST(SimpleOperationAdapterTest, HandleCustomCapabilityReportsNameInDetails)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-cap";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));

    ExecutionStatus reported_status = ExecutionStatus::Unknown;
    std::string     reported_capability;
    auto cap_event =
        ExecutionEvent::for_custom_capability("my-capability")
            .with_status_reporter([&](ExecutionStatus s, ExecutionStatusDetails d)
            {
                reported_status     = s;
                reported_capability = d.last_executed_capability;
            });

    EXPECT_CALL(control, next_exec_event())
        .WillOnce(Return(std::move(cap_event)))
        .WillOnce(Return(ExecutionEvent::from_kind(ExecutionEventKind::ControlGone)));

    SimpleOperationAdapter adapter{std::move(op)};
    auto result = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(result));

    const auto exec_result = get_value(result).future();
    EXPECT_TRUE(is_ok(exec_result));
    EXPECT_EQ(reported_status,     ExecutionStatus::UnsupportedCapability);
    EXPECT_EQ(reported_capability, "my-capability");
}

// ── SimpleOperationAdapter: Error events accumulated into exec_errors ─────────────

TEST(SimpleOperationAdapterTest, ErrorEventsAccumulatedAndReportedOnControlGone)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .WillOnce([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });

    ExecutionControlMock control{};
    const ExecutionId exec_id = "exec-acc-errors";
    EXPECT_CALL(control, exec_id()).WillOnce(ReturnRef(exec_id));

    // Two Error events followed by ControlGone (with a status reporter to capture exec_errors).
    auto err1 = ExecutionEvent::for_error(
        Error::from_nrc(uds::NegativeResponseCode::GeneralReject));
    auto err2 = ExecutionEvent::for_error(
        Error::from_nrc(uds::NegativeResponseCode::ConditionsNotCorrect));

    std::vector<Error> captured_errors;
    auto done_event = ExecutionEvent::from_kind(ExecutionEventKind::ControlGone);
    done_event.status_reporter.callback =
        [&captured_errors](ExecutionStatus /*status*/, ExecutionStatusDetails details)
        {
            if (details.exec_errors.has_value())
            {
                captured_errors = *details.exec_errors;
            }
        };

    EXPECT_CALL(control, next_exec_event())
        .WillOnce(Return(std::move(err1)))
        .WillOnce(Return(std::move(err2)))
        .WillOnce(Return(std::move(done_event)));

    SimpleOperationAdapter adapter{std::move(op)};
    auto handle_result = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(handle_result));

    // Invoking the future triggers the event loop which processes the two Error events
    // and then ControlGone, reporting exec_errors via the status reporter.
    const auto exec_result = get_value(handle_result).future();
    EXPECT_TRUE(is_ok(exec_result));

    ASSERT_EQ(captured_errors.size(), 2U);
    EXPECT_TRUE(is_uds_error(captured_errors[0].code));
    EXPECT_EQ(get_uds_nrc(captured_errors[0].code), uds::NegativeResponseCode::GeneralReject);
    EXPECT_TRUE(is_uds_error(captured_errors[1].code));
    EXPECT_EQ(get_uds_nrc(captured_errors[1].code), uds::NegativeResponseCode::ConditionsNotCorrect);
}

// ── SimpleOperationAdapter: re-use after future completes ─────────────────────────

TEST(SimpleOperationAdapterTest, AdapterCanBeReusedAfterFutureCompletes)
{
    auto op = std::make_unique<SimpleOperationMock>();
    EXPECT_CALL(*op, start(_))
        .Times(2)
        .WillRepeatedly([](ExecuteArguments) -> SimpleOperationMock::StartResult
        {
            return ExecutionHandle{[]() -> ExecutionResult { return DiagnosticReply{}; }};
        });

    ExecutionControlMock control{};
    const ExecutionId exec_id_1 = "exec-reuse-1";
    const ExecutionId exec_id_2 = "exec-reuse-2";
    EXPECT_CALL(control, exec_id())
        .WillOnce(ReturnRef(exec_id_1))
        .WillOnce(ReturnRef(exec_id_2));
    EXPECT_CALL(control, next_exec_event())
        .WillRepeatedly(Return(ExecutionEvent::from_kind(ExecutionEventKind::ControlGone)));

    SimpleOperationAdapter adapter{std::move(op)};

    // First execution — future completes and resets active_exec_id_.
    auto first = adapter.execute(ExecuteArguments{}, control);
    ASSERT_TRUE(is_ok(first));
    EXPECT_TRUE(is_ok(get_value(first).future()));

    // Second execution — must succeed because active_exec_id_ was reset.
    auto second = adapter.execute(ExecuteArguments{}, control);
    EXPECT_TRUE(is_ok(second));
}

}  // namespace score::mw::diag
