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

/// @file generic_data_identifier.h
/// @brief Combined UDS DataIdentifier supporting both ReadDataByIdentifier (Service 0x22)
///        and WriteDataByIdentifier (Service 0x2E) through a single implementation class.

#ifndef SCORE_MW_DIAG_API_CPP_UDS_GENERIC_DATA_IDENTIFIER_H
#define SCORE_MW_DIAG_API_CPP_UDS_GENERIC_DATA_IDENTIFIER_H

#include "score/mw/diag/read_data_by_identifier.h"
#include "score/mw/diag/write_data_by_identifier.h"

namespace score::mw::diag::uds
{

/// Combined UDS DataIdentifier — supports both ReadDataByIdentifier (Service 0x22)
/// and WriteDataByIdentifier (Service 0x2E) through a single implementation class.
///
/// Use this when a DID must handle both read and write requests.
/// For read-only or write-only DIDs, implement ReadDataByIdentifier or
/// WriteDataByIdentifier directly.
// NOLINTBEGIN(fuchsia-multiple-inheritance)
class GenericDataIdentifier : public ReadDataByIdentifier, public WriteDataByIdentifier
{
  public:
    GenericDataIdentifier() = default;
    GenericDataIdentifier(const GenericDataIdentifier&) = delete;
    GenericDataIdentifier(GenericDataIdentifier&&) noexcept = delete;
    GenericDataIdentifier& operator=(const GenericDataIdentifier&) & = delete;
    GenericDataIdentifier& operator=(GenericDataIdentifier&&) & noexcept = delete;
    virtual ~GenericDataIdentifier() noexcept;
};
// NOLINTEND(fuchsia-multiple-inheritance)

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_API_CPP_UDS_GENERIC_DATA_IDENTIFIER_H
