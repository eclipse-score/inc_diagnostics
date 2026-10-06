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
#ifndef SCORE_OPENSOVD_CORE_BRIDGE_READ_DID_H
#define SCORE_OPENSOVD_CORE_BRIDGE_READ_DID_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ScoreDiagReader ScoreDiagReader;
typedef struct ScoreDiagReadRequest ScoreDiagReadRequest;

typedef enum ScoreDiagReadStatus
{
    SCORE_DIAG_READ_OK = 0,
    SCORE_DIAG_READ_NRC = 1,
    SCORE_DIAG_READ_CANCELLED = 2,
    SCORE_DIAG_READ_FAILED = 3,
    SCORE_DIAG_READ_BUSY = 4
} ScoreDiagReadStatus;

typedef void (*ScoreDiagReadCompletion)(void* context, uint8_t status, uint8_t nrc, const uint8_t* data, size_t size);

ScoreDiagReader* score_diag_demo_reader_create(uint32_t delay_ms, uint8_t nrc);
void score_diag_reader_release(ScoreDiagReader* reader);
ScoreDiagReadRequest* score_diag_read_start(ScoreDiagReader* reader, void* context, ScoreDiagReadCompletion completion);
void score_diag_read_cancel(ScoreDiagReadRequest* request);
void score_diag_read_release(ScoreDiagReadRequest* request);

#ifdef __cplusplus
}

#include "score/mw/diag/uds/read_data_by_identifier.h"
#include <memory>

namespace score::opensovd::bridge
{
ScoreDiagReader* RegisterReadDataByIdentifier(std::shared_ptr<score::mw::diag::uds::ReadDataByIdentifier> handler);
}
#endif

#endif
