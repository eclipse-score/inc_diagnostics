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

/// @file operation_test.cpp
/// @brief Unit tests for score/mw/diag/operation.h
///        Covers: ExecuteArguments, ExecutionStatus, ExecutionStatusDetails,
///                StatusReporter, ExecutionEventKind, ExecutionEvent,
///                ExecutionHandle, OperationMetadata, ExecutionControlMock,
///                OperationMock.

#include "score/mw/diag/operation_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

namespace score
{
namespace mw
{
namespace diag
{

// ── ExecuteArguments ──────────────────────────────────────────────────────

TEST(OperationTest, ExecuteArgumentsDefaultBinaryEncoding)
{
    const ExecuteArguments args{};
    EXPECT_EQ(args.reply_encoding.kind(), ReplyMessageEncoding::Kind::Binary);
    EXPECT_FALSE(args.user_parameters.has_value());
    EXPECT_FALSE(args.additional_attrs.has_value());
    EXPECT_FALSE(args.proximity_response.has_value());
}

TEST(OperationTest, ExecuteArgumentsWithUserParameters)
{
    ExecuteArguments args{};
    args.user_parameters = RequestMessagePayload::from_bytes({0xABU, 0xCDU});
    ASSERT_TRUE(args.user_parameters.has_value());
    EXPECT_EQ(args.user_parameters->binary_data.size(), 2U);
}

TEST(OperationTest, ExecuteArgumentsWithProximityResponse)
{
    ExecuteArguments args{};
    args.proximity_response = std::string{"proximity-token-xyz"};
    ASSERT_TRUE(args.proximity_response.has_value());
    EXPECT_EQ(args.proximity_response.value(), "proximity-token-xyz");
}

TEST(OperationTest, ExecuteArgumentsWithUtf8Encoding)
{
    ExecuteArguments args{};
    args.reply_encoding = ReplyMessageEncoding::utf8();
    EXPECT_EQ(args.reply_encoding.kind(), ReplyMessageEncoding::Kind::Utf8);
}

// ── ExecutionStatus ───────────────────────────────────────────────────────

TEST(OperationTest, ExecutionStatusDistinctValues)
{
    EXPECT_NE(ExecutionStatus::Running,              ExecutionStatus::Completed);
    EXPECT_NE(ExecutionStatus::Running,              ExecutionStatus::Failed);
    EXPECT_NE(ExecutionStatus::Running,              ExecutionStatus::Stopped);
    EXPECT_NE(ExecutionStatus::Scheduled,            ExecutionStatus::Interrupted);
    EXPECT_NE(ExecutionStatus::UnsupportedCapability,ExecutionStatus::Unknown);
}

TEST(OperationTest, ExecutionStatusUsableInSwitch)
{
    auto describe = [](ExecutionStatus s) -> const char*
    {
        switch (s)
        {
            case ExecutionStatus::UnsupportedCapability: return "unsupported";
            case ExecutionStatus::Unknown:               return "unknown";
            case ExecutionStatus::Scheduled:             return "scheduled";
            case ExecutionStatus::Running:               return "running";
            case ExecutionStatus::Interrupted:           return "interrupted";
            case ExecutionStatus::Completed:             return "completed";
            case ExecutionStatus::Stopped:               return "stopped";
            case ExecutionStatus::Failed:                return "failed";
        }
        return "?";
    };
    EXPECT_STREQ(describe(ExecutionStatus::Running),   "running");
    EXPECT_STREQ(describe(ExecutionStatus::Completed), "completed");
    EXPECT_STREQ(describe(ExecutionStatus::Failed),    "failed");
    EXPECT_STREQ(describe(ExecutionStatus::Scheduled), "scheduled");
}

// ── ExecutionStatusDetails ────────────────────────────────────────────────

TEST(OperationTest, ExecutionStatusDetailsDefaultConstruction)
{
    const ExecutionStatusDetails details{};
    EXPECT_EQ(details.last_executed_capability, "n/a");
    EXPECT_FALSE(details.completion_percentage.has_value());
    EXPECT_FALSE(details.event_result.has_value());
    EXPECT_FALSE(details.exec_errors.has_value());
}

TEST(OperationTest, ExecutionStatusDetailsWithCompletionPercentage)
{
    auto details = ExecutionStatusDetails{}.with_completion_percentage(75U);
    ASSERT_TRUE(details.completion_percentage.has_value());
    EXPECT_EQ(details.completion_percentage.value(), 75U);
}

TEST(OperationTest, ExecutionStatusDetailsWithReplyData)
{
    DiagnosticReply reply{};
    reply.message_payload = ReplyMessagePayload::from_byte_vector({0x01U, 0x02U});
    auto details = ExecutionStatusDetails{}.with_reply_data(reply);
    ASSERT_TRUE(details.event_result.has_value());
    ASSERT_TRUE(details.event_result->message_payload.has_value());
    EXPECT_EQ(details.event_result->message_payload->kind, ReplyMessagePayload::Kind::Binary);
}

TEST(OperationTest, ExecutionStatusDetailsWithExecErrors)
{
    std::vector<Error> errors;
    errors.push_back(Error::from_error(
        sovd::GenericError::from_code(sovd::ErrorCode::NotResponding, "timed out")));
    auto details = ExecutionStatusDetails{}.with_exec_errors(std::move(errors));
    ASSERT_TRUE(details.exec_errors.has_value());
    EXPECT_EQ(details.exec_errors->size(), 1U);
    EXPECT_EQ(get_sovd_error(details.exec_errors->at(0).code).sovd_error, "not-responding");
}

TEST(OperationTest, ExecutionStatusDetailsCustomCapability)
{
    ExecutionStatusDetails details{};
    details.last_executed_capability = "myCapability";
    EXPECT_EQ(details.last_executed_capability, "myCapability");
}

// ── StatusReporter ────────────────────────────────────────────────────────

TEST(OperationTest, StatusReporterInvokesPutCallback)
{
    bool called = false;
    ExecutionStatus captured_status = ExecutionStatus::Unknown;

    StatusReporter reporter{};
    reporter.callback = [&](ExecutionStatus s, ExecutionStatusDetails /*d*/) {
        called         = true;
        captured_status = s;
    };

    reporter.put(ExecutionStatus::Running, ExecutionStatusDetails{});
    EXPECT_TRUE(called);
    EXPECT_EQ(captured_status, ExecutionStatus::Running);
}

TEST(OperationTest, StatusReporterWithNullCallbackIsNoOp)
{
    StatusReporter reporter{};
    // Default callback is empty — put() must not crash.
    EXPECT_NO_THROW(reporter.put(ExecutionStatus::Completed, ExecutionStatusDetails{}));
}

TEST(OperationTest, StatusReporterCanBeCalledMultipleTimes)
{
    int call_count = 0;
    StatusReporter reporter{};
    reporter.callback = [&](ExecutionStatus /*s*/, ExecutionStatusDetails /*d*/)
    {
        ++call_count;
    };

    reporter.put(ExecutionStatus::Running,   ExecutionStatusDetails{});
    reporter.put(ExecutionStatus::Completed, ExecutionStatusDetails{});
    reporter.put(ExecutionStatus::Failed,    ExecutionStatusDetails{});
    EXPECT_EQ(call_count, 3);
}

// ── ExecutionEventKind to_string (free function) ──────────────────────────

TEST(OperationTest, ExecutionEventKindToStringAllVariants)
{
    // Verify free-function to_string returns expected wire strings.
    EXPECT_EQ(to_string(ExecutionEventKind::ReportStatus),           "status");
    EXPECT_EQ(to_string(ExecutionEventKind::ControlGone),            "unknown");
    EXPECT_EQ(to_string(ExecutionEventKind::Interrupt),              "freeze");
    EXPECT_EQ(to_string(ExecutionEventKind::Resume),                 "execute");
    EXPECT_EQ(to_string(ExecutionEventKind::Reset),                  "reset");
    EXPECT_EQ(to_string(ExecutionEventKind::Stop),                   "stop");
    // HandleCustomCapability is represented by capability_name, not a wire string.
    EXPECT_EQ(to_string(ExecutionEventKind::HandleCustomCapability), "");
}

// ── ExecutionEvent builders ───────────────────────────────────────────────

TEST(OperationTest, ExecutionEventFromKindResume)
{
    const auto event = ExecutionEvent::from_kind(ExecutionEventKind::Resume);
    EXPECT_EQ(event.kind, ExecutionEventKind::Resume);
    EXPECT_FALSE(event.capability_name.has_value());
    EXPECT_FALSE(event.args.has_value());
}

TEST(OperationTest, ExecutionEventFromKindStop)
{
    const auto event = ExecutionEvent::from_kind(ExecutionEventKind::Stop);
    EXPECT_EQ(event.kind, ExecutionEventKind::Stop);
}

TEST(OperationTest, ExecutionEventFromKindInterrupt)
{
    const auto event = ExecutionEvent::from_kind(ExecutionEventKind::Interrupt);
    EXPECT_EQ(event.kind, ExecutionEventKind::Interrupt);
}

TEST(OperationTest, ExecutionEventFromKindHandleCustomCapability)
{
    auto event = ExecutionEvent::from_kind(ExecutionEventKind::HandleCustomCapability);
    event.capability_name = "myFeature";
    EXPECT_EQ(event.kind, ExecutionEventKind::HandleCustomCapability);
    ASSERT_TRUE(event.capability_name.has_value());
    EXPECT_EQ(event.capability_name.value(), "myFeature");
}

TEST(OperationTest, ExecutionEventForCustomCapabilityFactorySetsNameAndKind)
{
    const auto event = ExecutionEvent::for_custom_capability("myFeature");
    EXPECT_EQ(event.kind, ExecutionEventKind::HandleCustomCapability);
    ASSERT_TRUE(event.capability_name.has_value());
    EXPECT_EQ(event.capability_name.value(), "myFeature");
    EXPECT_FALSE(event.args.has_value());
}

TEST(OperationTest, ExecutionEventWithArgs)
{
    ExecuteArguments args{};
    args.user_parameters = RequestMessagePayload::from_bytes({0x01U});
    const auto event =
        ExecutionEvent::from_kind(ExecutionEventKind::Resume).with_args(std::move(args));
    ASSERT_TRUE(event.args.has_value());
    ASSERT_TRUE(event.args->user_parameters.has_value());
    EXPECT_EQ(event.args->user_parameters->binary_data.size(), 1U);
}

TEST(OperationTest, ExecutionEventWithStatusReporter)
{
    bool reporter_called = false;
    const auto event =
        ExecutionEvent::from_kind(ExecutionEventKind::ReportStatus)
            .with_status_reporter([&](ExecutionStatus /*s*/, ExecutionStatusDetails /*d*/)
            {
                reporter_called = true;
            });
    ASSERT_TRUE(static_cast<bool>(event.status_reporter.callback));
    // Invoke the callback to verify it wires through.
    const_cast<StatusReporter&>(event.status_reporter)
        .put(ExecutionStatus::Completed, ExecutionStatusDetails{});
    EXPECT_TRUE(reporter_called);
}

// ── ExecutionHandle ───────────────────────────────────────────────────────

TEST(OperationTest, ExecutionHandleWithReply)
{
    DiagnosticReply reply{};
    reply.message_payload = ReplyMessagePayload::from_string("done");

    const auto handle = ExecutionHandle{
        []() -> ExecutionResult { return DiagnosticReply{}; }
    }.with_reply(std::move(reply));

    ASSERT_TRUE(handle.reply.has_value());
    ASSERT_TRUE(handle.reply->message_payload.has_value());
    EXPECT_EQ(handle.reply->message_payload->text_data, "done");
}

TEST(OperationTest, ExecutionHandleFutureIsInvokable)
{
    bool future_called = false;
    const ExecutionHandle handle{
        [&]() -> ExecutionResult {
            future_called = true;
            return DiagnosticReply{};
        }
    };

    const auto result = handle.future();
    EXPECT_TRUE(future_called);
    EXPECT_TRUE(is_ok(result));
}

TEST(OperationTest, ExecutionHandleDefaultNoReply)
{
    const ExecutionHandle handle{
        []() -> ExecutionResult { return DiagnosticReply{}; }
    };
    EXPECT_FALSE(handle.reply.has_value());
}

// ── OperationMetadata ─────────────────────────────────────────────────────

TEST(OperationTest, OperationMetadataDefaults)
{
    const OperationMetadata meta{};
    EXPECT_FALSE(meta.proximity_proof_required);
    EXPECT_FALSE(meta.synchronous_execution);
    EXPECT_FALSE(meta.exclusive_execution);
    EXPECT_FALSE(meta.supported_modes.has_value());
}

TEST(OperationTest, OperationMetadataProximityRequired)
{
    OperationMetadata meta{};
    meta.proximity_proof_required = true;
    EXPECT_TRUE(meta.proximity_proof_required);
}

TEST(OperationTest, OperationMetadataExclusiveSync)
{
    OperationMetadata meta{};
    meta.synchronous_execution = true;
    meta.exclusive_execution   = true;
    EXPECT_TRUE(meta.synchronous_execution);
    EXPECT_TRUE(meta.exclusive_execution);
}

TEST(OperationTest, OperationMetadataSupportedModes)
{
    OperationMetadata meta{};
    InsertionOrderedMap<std::string, std::vector<std::string>> modes;
    modes.emplace("session", std::vector<std::string>{"default", "programming"});
    meta.supported_modes = std::move(modes);
    ASSERT_TRUE(meta.supported_modes.has_value());
    EXPECT_EQ(meta.supported_modes->at("session").size(), 2U);
}

// ── ExecutionControlMock ──────────────────────────────────────────────────

TEST(OperationTest, ExecutionControlMockExecId)
{
    ExecutionControlMock mock{};
    const ExecutionId expected_id = "exec-42";

    EXPECT_CALL(mock, exec_id())
        .WillOnce(ReturnRef(expected_id));

    EXPECT_EQ(mock.exec_id(), "exec-42");
}

TEST(OperationTest, ExecutionControlMockNextExecEvent)
{
    ExecutionControlMock mock{};

    EXPECT_CALL(mock, next_exec_event())
        .WillOnce(Return(ExecutionEvent::from_kind(ExecutionEventKind::Resume)))
        .WillOnce(Return(ExecutionEvent::from_kind(ExecutionEventKind::Stop)))
        .WillOnce(Return(ExecutionEvent::from_kind(ExecutionEventKind::ControlGone)));

    EXPECT_EQ(mock.next_exec_event().kind, ExecutionEventKind::Resume);
    EXPECT_EQ(mock.next_exec_event().kind, ExecutionEventKind::Stop);
    EXPECT_EQ(mock.next_exec_event().kind, ExecutionEventKind::ControlGone);
}

// ── OperationMock ─────────────────────────────────────────────────────────

TEST(OperationTest, OperationMockExecuteReturnsOk)
{
    OperationMock mock{};
    ExecutionControlMock ctrl{};

    const ExecutionHandle expected_handle{
        []() -> ExecutionResult { return DiagnosticReply{}; }
    };

    EXPECT_CALL(mock, execute(_, _))
        .WillOnce(Return(OperationMock::ExecResult{expected_handle}));

    const auto result = mock.execute(ExecuteArguments{}, ctrl);
    EXPECT_TRUE(is_ok(result));
}

TEST(OperationTest, OperationMockExecuteReturnsError)
{
    OperationMock mock{};
    ExecutionControlMock ctrl{};

    EXPECT_CALL(mock, execute(_, _))
        .WillOnce(Return(OperationMock::ExecResult{
            Error::from_error(
                sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "operation not ready"))}));

    const auto result = mock.execute(ExecuteArguments{}, ctrl);
    ASSERT_TRUE(is_err(result));
    EXPECT_TRUE(is_sovd_error(get_error(result).code));
    EXPECT_EQ(get_sovd_error(get_error(result).code).sovd_error, "precondition-not-fulfilled");
}

TEST(OperationTest, OperationMockExecuteMultipleCalls)
{
    OperationMock mock{};
    ExecutionControlMock ctrl{};

    const ExecutionHandle handle{[]() -> ExecutionResult { return DiagnosticReply{}; }};

    EXPECT_CALL(mock, execute(_, _))
        .Times(2)
        .WillRepeatedly(Return(OperationMock::ExecResult{handle}));

    EXPECT_TRUE(is_ok(mock.execute(ExecuteArguments{}, ctrl)));
    EXPECT_TRUE(is_ok(mock.execute(ExecuteArguments{}, ctrl)));
}

}  // namespace diag
}  // namespace mw
}  // namespace score
