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
#ifndef SCORE_OPENSOVD_CORE_BRIDGE_READ_DID_TEST_SUPPORT_H
#define SCORE_OPENSOVD_CORE_BRIDGE_READ_DID_TEST_SUPPORT_H

#include "score/opensovd-core/bridge/read_did.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Test-only reader whose reads stay pending until score_diag_test_control_complete. Completed reads return "ok". */
typedef struct ScoreDiagTestControl ScoreDiagTestControl;

ScoreDiagTestControl* score_diag_test_control_create(void);
ScoreDiagReader* score_diag_test_control_register(const ScoreDiagTestControl* control);
size_t score_diag_test_control_started(const ScoreDiagTestControl* control);
size_t score_diag_test_control_stopped(const ScoreDiagTestControl* control);
void score_diag_test_control_complete(ScoreDiagTestControl* control);
void score_diag_test_control_release(ScoreDiagTestControl* control);

#ifdef __cplusplus
}
#endif

#endif
