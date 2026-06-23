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
    ///
    /// @note The Rust equivalent (DataResourceAdapter::with_rdbi(self)) consumes the builder
    ///       and returns a new instance, preventing partial-builder reuse. In C++, this method
    ///       returns an lvalue reference to *this to enable method chaining on a named variable.
    ///       Only chain calls; do not store the returned reference beyond the full build expression.
    DataResourceAdapter& with_rdbi(std::unique_ptr<ReadDataByIdentifier> rdbi) & noexcept;

    /// Register a WriteDataByIdentifier implementation (builder pattern).
    ///
    /// @note Same consuming-vs-lvalue-reference note as with_rdbi() above.
    DataResourceAdapter& with_wdbi(std::unique_ptr<WriteDataByIdentifier> wdbi) & noexcept;

    /// Read the data resource value via the registered RDBI service.
    /// Only Binary reply encoding is supported (UDS returns raw bytes).
    [[nodiscard]] Result<ReadValueReply> read(ReadValueArgs input) override;

    /// Write a value to the data resource via the registered WDBI service.
    /// Only Binary input is supported (user_data must hold a ByteVector — i.e.
    /// std::holds_alternative<ByteVector>(user_data) must be true).
    [[nodiscard]] WriteValueResult write(WriteValueArgs input) override;

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
    explicit RoutineControlAdapter(std::unique_ptr<RoutineControl> routine_control) noexcept;

    [[nodiscard]] Result<ExecutionHandle> start(ExecuteArguments input) override;

    [[nodiscard]] Result<std::optional<DiagnosticReply>> stop(std::optional<ExecuteArguments> input) override;

    std::optional<std::uint8_t> completion_percentage() const noexcept override;

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
