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

/// @file uds_adapters.h
/// @brief Adapter classes bridging UDS interfaces to the SOVD data/operation model.
///
/// DataResourceAdapter bridges ReadDataByIdentifier and WriteDataByIdentifier to
/// the DataResource interface, enabling UDS DIDs to be exposed as SOVD data resources.
///
/// RoutineControlAdapter bridges RoutineControl to the SimpleOperation interface,
/// enabling UDS routines to be exposed as SOVD operations.

#ifndef SCORE_MW_DIAG_UDS_ADAPTERS_H
#define SCORE_MW_DIAG_UDS_ADAPTERS_H

#include "score/mw/diag/data_resource.h"
#include "score/mw/diag/simple_operation.h"
#include "score/mw/diag/uds.h"

#include <memory>
#include <optional>

namespace score
{
namespace mw
{
namespace diag
{

/************************************/
/* DataResourceAdapter              */
/************************************/

/// Bridges UDS ReadDataByIdentifier (0x22) and/or WriteDataByIdentifier (0x2E)
/// to the DataResource interface.
///
/// - read()  is served by the registered RDBI instance (binary encoding only).
/// - write() is served by the registered WDBI instance (binary input only).
class DataResourceAdapter final : public DataResource
{
  public:
    DataResourceAdapter() noexcept = default;

    /// Register a ReadDataByIdentifier implementation (builder pattern).
    DataResourceAdapter& with_rdbi(std::unique_ptr<ReadDataByIdentifier> rdbi) & noexcept
    {
        rdbi_ = std::move(rdbi);
        return *this;
    }

    /// Register a WriteDataByIdentifier implementation (builder pattern).
    DataResourceAdapter& with_wdbi(std::unique_ptr<WriteDataByIdentifier> wdbi) & noexcept
    {
        wdbi_ = std::move(wdbi);
        return *this;
    }

    /// Read the data resource value via the registered RDBI service.
    /// Only Binary reply encoding is supported (UDS returns raw bytes).
    Result<ReadValueReply> read(ReadValueArgs input) override
    {
        if (!rdbi_)
        {
            return Result<ReadValueReply>{score::unexpect,
                    Error::from_error(sovd::GenericError::from_code(
                        sovd::ErrorCode::PreconditionNotFulfilled,
                        "no ReadDataByIdentifier service registered for this data resource"))};
        }
        if (!input.reply_encoding.is_binary())
        {
            return Result<ReadValueReply>{score::unexpect,
                    Error::from_error(sovd::GenericError::from_code(
                        sovd::ErrorCode::PreconditionNotFulfilled,
                        "this data resource only supports binary encoding for its reply data"))};
        }
        auto read_result = rdbi_->read();
        if (!read_result.has_value())
        {
            return Result<ReadValueReply>{score::unexpect, read_result.error()};
        }
        return ReadValueReply{
            ReplyMessagePayload::from_byte_vector(std::move(*read_result)),
            std::nullopt};
    }

    /// Write a value to the data resource via the registered WDBI service.
    /// Only Binary input is supported (user_data must be RequestMessagePayload::Binary).
    WriteValueResult write(WriteValueArgs input) override
    {
        if (!wdbi_)
        {
            return sovd::DataError::from_error(
                sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "no WriteDataByIdentifier service registered for this data resource"));
        }
        if (!input.user_data.has_value() ||
            input.user_data->kind != RequestMessagePayload::Kind::Binary)
        {
            return sovd::DataError::from_error(
                sovd::GenericError::from_code(
                    sovd::ErrorCode::IncompleteRequest,
                    "this data resource requires binary encoding for its input data"));
        }
        const ByteView view{input.user_data->binary_data};
        auto write_result = wdbi_->write(view);
        if (!write_result.has_value())
        {
        const Error& err = write_result.error();
            // cf. Rust: match e.code { SOVD(err) => Some(err), UDS(nrc) => mapped GenericError }
            if (is_sovd_error(err.code))
            {
                return sovd::DataError{std::string{}, get_sovd_error(err.code)};
            }
            return sovd::DataError::from_error(
                sovd::GenericError::from_code(
                    sovd::ErrorCode::ErrorResponse,
                    "write operation failed with a UDS negative response code"));
        }
        return std::monostate{};
    }

    DataResourceAdapter(const DataResourceAdapter&)           = delete;
    DataResourceAdapter(DataResourceAdapter&&) noexcept       = delete;
    DataResourceAdapter& operator=(const DataResourceAdapter&)  & = delete;
    DataResourceAdapter& operator=(DataResourceAdapter&&) &    noexcept = delete;
    ~DataResourceAdapter() noexcept override                   = default;

  private:
    std::unique_ptr<ReadDataByIdentifier>  rdbi_;
    std::unique_ptr<WriteDataByIdentifier> wdbi_;
};

/************************************/
/* RoutineControlAdapter            */
/************************************/

/// Bridges UDS RoutineControl (Service 0x31) to the SimpleOperation interface.
///
/// - start() maps ExecuteArguments (binary user_parameters) to RoutineControl::start().
///   The returned ExecutionHandle::future invokes StartRoutine::result_provider.
/// - stop()  maps optional ExecuteArguments to RoutineControl::stop().
/// - completion_percentage() delegates to RoutineControl::completion_percentage().
class RoutineControlAdapter final : public SimpleOperation
{
  public:
    /// Constructs the adapter taking ownership of the RoutineControl implementation.
    explicit RoutineControlAdapter(std::unique_ptr<RoutineControl> routine_control) noexcept
        : routine_control_{std::move(routine_control)}
    {}

    Result<ExecutionHandle> start(ExecuteArguments input) override
    {
        // Extract binary payload from user_parameters (only Binary encoding supported)
        std::optional<ByteVector> byte_input;
        if (input.user_parameters.has_value())
        {
            if (input.user_parameters->kind != RequestMessagePayload::Kind::Binary)
            {
                return Result<ExecutionHandle>{score::unexpect,
                        Error::from_error(sovd::GenericError::from_code(
                            sovd::ErrorCode::PreconditionNotFulfilled,
                            "UDS RoutineControl only supports binary encoding for its input"))};
            }
            byte_input = std::move(input.user_parameters->binary_data);
        }

        const std::optional<ByteView> view =
            byte_input.has_value()
                ? std::optional<ByteView>{ByteView{*byte_input}}
                : std::nullopt;

        auto start_result = routine_control_->start(view);
        if (!start_result.has_value())
        {
            return Result<ExecutionHandle>{score::unexpect, start_result.error()};
        }

        StartRoutine& sr = *start_result;

        // Build optional initial DiagnosticReply from the StartRoutine byte reply
        std::optional<DiagnosticReply> initial_reply;
        if (sr.reply.has_value())
        {
            initial_reply = DiagnosticReply{
                ReplyMessagePayload::from_byte_vector(std::move(*sr.reply)),
                std::nullopt};
        }

        // Wrap the StartRoutine::result_provider in an ExecutionResult callable
        auto result_provider = std::move(sr.result_provider);
        ExecutionHandle handle{
            [rp = std::move(result_provider)]() mutable -> ExecutionResult
            {
                if (!rp)
                {
                    return DiagnosticReply{};
                }
                auto routine_result = rp();
                if (!routine_result.has_value())
                {
                    return ExecutionResult{score::unexpect, routine_result.error()};
                }
                DiagnosticReply diag_reply{};
                if (routine_result->has_value())
                {
                    diag_reply.message_payload =
                        ReplyMessagePayload::from_byte_vector(std::move(**routine_result));
                }
                return diag_reply;
            }};
        handle.reply = std::move(initial_reply);
        return handle;
    }

    Result<std::optional<DiagnosticReply>> stop(std::optional<ExecuteArguments> input) override
    {
        std::optional<ByteVector> byte_input;
        if (input.has_value() && input->user_parameters.has_value())
        {
            if (input->user_parameters->kind != RequestMessagePayload::Kind::Binary)
            {
                return Result<std::optional<DiagnosticReply>>{score::unexpect,
                        Error::from_error(sovd::GenericError::from_code(
                            sovd::ErrorCode::PreconditionNotFulfilled,
                            "UDS RoutineControl only supports binary encoding for its input"))};
            }
            byte_input = std::move(input->user_parameters->binary_data);
        }

        const std::optional<ByteView> view =
            byte_input.has_value()
                ? std::optional<ByteView>{ByteView{*byte_input}}
                : std::nullopt;

        auto stop_result = routine_control_->stop(view);
        if (!stop_result.has_value())
        {
            return Result<std::optional<DiagnosticReply>>{score::unexpect, stop_result.error()};
        }
        if (!stop_result->has_value())
        {
            return std::optional<DiagnosticReply>{std::nullopt};
        }
        DiagnosticReply reply{};
        reply.message_payload =
            ReplyMessagePayload::from_byte_vector(std::move(**stop_result));
        return std::optional<DiagnosticReply>{std::move(reply)};
    }

    std::optional<std::uint8_t> completion_percentage() const noexcept override
    {
        return routine_control_->completion_percentage();
    }

    RoutineControlAdapter(const RoutineControlAdapter&)           = delete;
    RoutineControlAdapter(RoutineControlAdapter&&) noexcept       = delete;
    RoutineControlAdapter& operator=(const RoutineControlAdapter&)  & = delete;
    RoutineControlAdapter& operator=(RoutineControlAdapter&&) &    noexcept = delete;
    ~RoutineControlAdapter() noexcept override                    = default;

  private:
    std::unique_ptr<RoutineControl> routine_control_;
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_UDS_ADAPTERS_H
