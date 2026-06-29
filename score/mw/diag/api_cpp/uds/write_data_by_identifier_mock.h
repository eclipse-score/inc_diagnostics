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

/// @file write_data_by_identifier_mock.h
/// @brief GMock implementation of score::mw::diag::WriteDataByIdentifier.

#ifndef SCORE_MW_DIAG_API_CPP_UDS_WRITE_DATA_BY_IDENTIFIER_MOCK_H
#define SCORE_MW_DIAG_API_CPP_UDS_WRITE_DATA_BY_IDENTIFIER_MOCK_H

#include "score/mw/diag/write_data_by_identifier.h"

#include <gmock/gmock.h>

namespace score::mw::diag::uds
{

/// Mock for score::mw::diag::uds::WriteDataByIdentifier (Service 0x2E).
class WriteDataByIdentifierMock : public WriteDataByIdentifier
{
  public:
    MOCK_METHOD(ResultBlank, Write, (ByteView input), (override));
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_API_CPP_UDS_WRITE_DATA_BY_IDENTIFIER_MOCK_H
