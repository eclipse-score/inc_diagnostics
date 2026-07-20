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

/// @file generic_service.h
/// @brief Generic raw UDS service handler interface for vendor-specific services
///        (See ISO 14229-1:2020, vendor-specific service identifiers).
///
/// Provides two levels of abstraction:
///   - `GenericService`       — full interface with `MetaData` and cancellation support.
///   - `SimpleGenericService` — simplified adapter for non-blocking, context-free service handling.

#ifndef SCORE_MW_DIAG_UDS_GENERIC_SERVICE_H
#define SCORE_MW_DIAG_UDS_GENERIC_SERVICE_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/meta_data.h"

#include <score/stop_token.hpp>

#include <future>

namespace score::mw::diag::uds
{

/// Generic raw UDS service handler for custom or proprietary services not covered by
/// ReadDataByIdentifier, WriteDataByIdentifier or RoutineControl.
/// See ISO 14229-1:2020 — vendor-specific service identifiers.
///
/// Full interface — receives request `MetaData` (session, security level, etc.) and
/// a `stop_token` for cooperative cancellation of long-running operations.
class GenericService
{
  public:
    /// Handle a raw UDS message.
    /// @param input       Raw request payload bytes (service identifier + data).
    /// @param meta_data   Context provided by the diagnostic runtime for this request.
    /// @param stop_token  Token that becomes stopped if the runtime cancels the request.
    /// @return std::future<Result<ByteVector>> on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual std::future<Result<ByteVector>> HandleMessage(ByteView input,
                                                                        const MetaData& meta_data,
                                                                        score::cpp::stop_token stop_token) = 0;

    virtual ~GenericService() noexcept = default;
};

/// Simplified adapter for `GenericService` (must be non-blocking!)
///
/// Implementors override HandleMessage() to process the raw byte payload
/// and return the response bytes. The diagnostic runtime is responsible for
/// sending a rejection response when no suitable handler is registered.
class SimpleGenericService : public GenericService
{
  public:
    /// Handle a raw UDS message in a fast and non-blocking manner.
    /// @param input  Raw request payload bytes (service identifier + data).
    /// @return Result<ByteVector> on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<ByteVector> HandleMessage(ByteView input) = 0;

    virtual ~SimpleGenericService() noexcept = default;

  private:
    std::future<Result<ByteVector>> HandleMessage(ByteView input,
                                     const MetaData& /*meta_data*/,
                                     score::cpp::stop_token /*stop_token*/) final
    {
        std::promise<Result<ByteVector>> promise;
        promise.set_value(HandleMessage(input));
        return promise.get_future();
    }
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_GENERIC_SERVICE_H
