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

/// @file read_data_by_identifier_mock.h
/// @brief GMock implementation of score::mw::diag::ReadDataByIdentifier.

#ifndef SCORE_MW_DIAG_UDS_READ_DATA_BY_IDENTIFIER_MOCK_H
#define SCORE_MW_DIAG_UDS_READ_DATA_BY_IDENTIFIER_MOCK_H

#include "score/mw/diag/uds/read_data_by_identifier.h"

#include <gmock/gmock.h>

namespace score::mw::diag::uds
{

/// Mock for score::mw::diag::uds::ReadDataByIdentifier (Service 0x22).
class ReadDataByIdentifierMock : public ReadDataByIdentifier
{
  public:
    MOCK_METHOD(ResultWithData, Read, (), (override));
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_READ_DATA_BY_IDENTIFIER_MOCK_H
