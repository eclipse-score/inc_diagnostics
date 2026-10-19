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
#ifndef SCORE_OPENSOVD_CORE_BRIDGE_APPLICATION_H
#define SCORE_OPENSOVD_CORE_BRIDGE_APPLICATION_H

#include "score/opensovd-core/bridge/read_did.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ScoreDiagApplication ScoreDiagApplication;
typedef struct ScoreDiagSensorSource ScoreDiagSensorSource;

typedef enum ScoreDiagEncoding
{
    SCORE_DIAG_BYTES = 0,
    SCORE_DIAG_TEMPERATURE_CENTIDEGREES = 1,
    SCORE_DIAG_HEALTHY_BOOLEAN = 2
} ScoreDiagEncoding;

const char* score_diag_application_id(const ScoreDiagApplication* application);
const char* score_diag_application_name(const ScoreDiagApplication* application);
const char* score_diag_application_component(const ScoreDiagApplication* application);
size_t score_diag_application_resource_count(const ScoreDiagApplication* application);
const char* score_diag_application_resource_id(const ScoreDiagApplication* application, size_t index);
const char* score_diag_application_resource_name(const ScoreDiagApplication* application, size_t index);
uint8_t score_diag_application_resource_encoding(const ScoreDiagApplication* application, size_t index);
ScoreDiagReader* score_diag_application_resource_reader(const ScoreDiagApplication* application, size_t index);
void score_diag_application_release(ScoreDiagApplication* application);

ScoreDiagSensorSource* score_diag_sensor_source_create(void);
void score_diag_sensor_source_publish(ScoreDiagSensorSource* source, int16_t temperature, uint8_t available);
ScoreDiagApplication* score_diag_sensor_source_register(const ScoreDiagSensorSource* source);
void score_diag_sensor_source_release(ScoreDiagSensorSource* source);

#ifdef __cplusplus
}

#include <string>
#include <vector>

namespace score::opensovd::bridge
{
struct ReadResourceRegistration
{
    std::string id;
    std::string name;
    ScoreDiagEncoding encoding;
    std::shared_ptr<score::mw::diag::uds::ReadDataByIdentifier> handler;
};

ScoreDiagApplication* RegisterApplication(std::string id,
                                          std::string name,
                                          std::string component,
                                          std::vector<ReadResourceRegistration> resources);
}  // namespace score::opensovd::bridge
#endif

#endif
