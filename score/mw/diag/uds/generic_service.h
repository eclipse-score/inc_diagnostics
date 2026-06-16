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

#ifndef SCORE_MW_DIAG_UDS_GENERIC_SERVICE_H
#define SCORE_MW_DIAG_UDS_GENERIC_SERVICE_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/negative_response_code.h"
#include "score/result/result.h"  // score::unexpect

namespace score::mw::diag::uds
{

/// Generic raw UDS service handler for custom or proprietary services not covered by
/// ReadDataByIdentifier, WriteDataByIdentifier or RoutineControl.
/// See ISO 14229-1:2020 — vendor-specific service identifiers.
///
/// The default implementation rejects all messages with
/// NegativeResponseCode::SubFunctionNotSupported, indicating that the
/// specific sub-function is not handled. Implementors override HandleMessage()
/// to process the raw byte payload and return the response bytes.
class GenericService
{
  public:
    /// Handle a raw UDS message.
    /// @param input  Raw request payload bytes (service identifier + data).
    /// @return ResultWithData on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual ResultWithData HandleMessage(ByteView /*input*/)
    {
        return ResultWithData{score::unexpect, NegativeResponseCode::SubFunctionNotSupported};
    }

    GenericService() = default;
    virtual ~GenericService() noexcept = default;
    GenericService(const GenericService&) = delete;
    GenericService(GenericService&&) noexcept = delete;
    GenericService& operator=(const GenericService&) & = delete;
    GenericService& operator=(GenericService&&) & noexcept = delete;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_GENERIC_SERVICE_H
