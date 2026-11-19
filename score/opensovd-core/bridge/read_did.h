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

/* Fixed one-byte status shared with Rust's #[repr(u8)] ReadStatus; C enums have an int-sized underlying type. */
typedef uint8_t ScoreDiagReadStatus;
enum
{
    SCORE_DIAG_READ_OK = 0,
    SCORE_DIAG_READ_NRC = 1,
    SCORE_DIAG_READ_CANCELLED = 2,
    SCORE_DIAG_READ_FAILED = 3,
    SCORE_DIAG_READ_BUSY = 4
};

typedef void (
    *ScoreDiagReadCompletion)(void* context, ScoreDiagReadStatus status, uint8_t nrc, const uint8_t* data, size_t size);

/* Outcome of a call into the bridge. On anything but SCORE_DIAG_OK the out-parameter is left NULL. */
typedef uint8_t ScoreDiagStatus;
enum
{
    SCORE_DIAG_OK = 0,
    SCORE_DIAG_INVALID_ARGUMENT = 1,
    SCORE_DIAG_BUSY = 2,
    SCORE_DIAG_FAILED = 3
};

ScoreDiagStatus score_diag_demo_reader_create(uint32_t delay_ms, uint8_t nrc, ScoreDiagReader** out);
ScoreDiagStatus score_diag_reader_clone(const ScoreDiagReader* reader, ScoreDiagReader** out);
void score_diag_reader_release(ScoreDiagReader* reader);
/* On a status other than SCORE_DIAG_OK, completion is not called. */
ScoreDiagStatus score_diag_read_start(ScoreDiagReader* reader,
                                      void* context,
                                      ScoreDiagReadCompletion completion,
                                      ScoreDiagReadRequest** out);
void score_diag_read_cancel(ScoreDiagReadRequest* request);
void score_diag_read_release(ScoreDiagReadRequest* request);

#ifdef __cplusplus
}

#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/result/result.h"
#include <memory>

namespace score::opensovd::bridge
{
score::Result<ScoreDiagReader*> RegisterReadDataByIdentifier(
    std::shared_ptr<score::mw::diag::uds::ReadDataByIdentifier> handler);
}
#endif

#endif
