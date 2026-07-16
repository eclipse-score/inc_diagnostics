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

/// @file generic_service_mock.h
/// @brief GMock implementation of score::mw::diag::uds::GenericService.

#ifndef SCORE_MW_DIAG_UDS_GENERIC_SERVICE_MOCK_H
#define SCORE_MW_DIAG_UDS_GENERIC_SERVICE_MOCK_H

#include "score/mw/diag/uds/generic_service.h"

#include <gmock/gmock.h>

namespace score::mw::diag::uds::test
{

/// Mock for the full context-aware score::mw::diag::uds::GenericService.
class GenericServiceMock : public GenericService
{
  public:
    MOCK_METHOD(Result<ByteVector>,
                HandleMessage,
                (ByteView input, const MetaData& meta_data, score::cpp::stop_token stop_token),
                (override));
};

/// Mock for the context-free score::mw::diag::uds::SimpleGenericService.
class SimpleGenericServiceMock : public SimpleGenericService
{
  public:
    MOCK_METHOD(Result<ByteVector>, HandleMessage, (ByteView input), (override));
};

}  // namespace score::mw::diag::uds::test

#endif  // SCORE_MW_DIAG_UDS_GENERIC_SERVICE_MOCK_H
