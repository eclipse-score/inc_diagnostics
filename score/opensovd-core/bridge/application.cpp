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
#include "score/opensovd-core/bridge/application.h"
#include "score/mw/diag/uds/negative_response_code.h"

#include <mutex>
#include <set>
#include <utility>

namespace
{
using score::mw::diag::ByteVector;
using score::mw::diag::Future;
using score::mw::diag::uds::MetaData;
using score::mw::diag::uds::NegativeResponseCode;
using score::mw::diag::uds::ReadDataByIdentifier;
using score::mw::diag::uds::Result;

struct BoundResource
{
    std::string id;
    std::string name;
    ScoreDiagEncoding encoding;
    std::shared_ptr<ScoreDiagReader> reader;
};

struct SensorState
{
    std::mutex mutex;
    std::int16_t temperature{2150};
    bool available{true};
};

class SensorReader final : public ReadDataByIdentifier
{
  public:
    SensorReader(std::shared_ptr<SensorState> state, bool health) : state_{std::move(state)}, health_{health} {}

    Future<Result<ByteVector>> Read(const MetaData&, score::cpp::stop_token token) override
    {
        std::lock_guard<std::mutex> guard{state_->mutex};
        if (token.stop_requested())
        {
            return score::mw::diag::WrapAsFuture(
                Result<ByteVector>{score::MakeUnexpected(NegativeResponseCode::ConditionsNotCorrect)});
        }
        if (health_)
        {
            return score::mw::diag::WrapAsFuture(
                Result<ByteVector>{ByteVector{state_->available ? std::byte{1} : std::byte{0}}});
        }
        if (!state_->available)
        {
            return score::mw::diag::WrapAsFuture(
                Result<ByteVector>{score::MakeUnexpected(NegativeResponseCode::ConditionsNotCorrect)});
        }
        const auto value = static_cast<std::uint16_t>(state_->temperature);
        return score::mw::diag::WrapAsFuture(
            Result<ByteVector>{ByteVector{static_cast<std::byte>(value >> 8U), static_cast<std::byte>(value & 0xffU)}});
    }

  private:
    std::shared_ptr<SensorState> state_;
    bool health_;
};
}  // namespace

struct ScoreDiagApplication
{
    std::string id;
    std::string name;
    std::string component;
    std::vector<BoundResource> resources;
};

struct ScoreDiagSensorSource
{
    std::shared_ptr<SensorState> state;
};

namespace score::opensovd::bridge
{
ScoreDiagApplication* RegisterApplication(std::string id,
                                          std::string name,
                                          std::string component,
                                          std::vector<ReadResourceRegistration> resources)
{
    if (id.empty() || name.empty() || component.empty() || resources.empty())
    {
        return nullptr;
    }
    auto application = std::make_unique<ScoreDiagApplication>();
    application->id = std::move(id);
    application->name = std::move(name);
    application->component = std::move(component);
    std::set<std::string> identifiers;
    for (auto& resource : resources)
    {
        if (resource.id.empty() || resource.name.empty() || !resource.handler ||
            !identifiers.insert(resource.id).second || resource.encoding > SCORE_DIAG_HEALTHY_BOOLEAN ||
            resource.encoding < SCORE_DIAG_BYTES)
        {
            return nullptr;
        }
        std::shared_ptr<ScoreDiagReader> reader{RegisterReadDataByIdentifier(std::move(resource.handler)),
                                                score_diag_reader_release};
        if (!reader)
        {
            return nullptr;
        }
        application->resources.push_back(
            BoundResource{std::move(resource.id), std::move(resource.name), resource.encoding, std::move(reader)});
    }
    return application.release();
}
}  // namespace score::opensovd::bridge

extern "C" const char* score_diag_application_id(const ScoreDiagApplication* application)
{
    return application->id.c_str();
}

extern "C" const char* score_diag_application_name(const ScoreDiagApplication* application)
{
    return application->name.c_str();
}

extern "C" const char* score_diag_application_component(const ScoreDiagApplication* application)
{
    return application->component.c_str();
}

extern "C" std::size_t score_diag_application_resource_count(const ScoreDiagApplication* application)
{
    return application->resources.size();
}

extern "C" const char* score_diag_application_resource_id(const ScoreDiagApplication* application, std::size_t index)
{
    return index < application->resources.size() ? application->resources[index].id.c_str() : nullptr;
}

extern "C" const char* score_diag_application_resource_name(const ScoreDiagApplication* application, std::size_t index)
{
    return index < application->resources.size() ? application->resources[index].name.c_str() : nullptr;
}

extern "C" std::uint8_t score_diag_application_resource_encoding(const ScoreDiagApplication* application,
                                                                 std::size_t index)
{
    return index < application->resources.size() ? static_cast<std::uint8_t>(application->resources[index].encoding)
                                                 : 0xffU;
}

extern "C" ScoreDiagReader* score_diag_application_resource_reader(const ScoreDiagApplication* application,
                                                                   std::size_t index)
{
    return index < application->resources.size() ? score_diag_reader_clone(application->resources[index].reader.get())
                                                 : nullptr;
}

extern "C" void score_diag_application_release(ScoreDiagApplication* application)
{
    delete application;
}

extern "C" ScoreDiagSensorSource* score_diag_sensor_source_create()
{
    try
    {
        return new ScoreDiagSensorSource{std::make_shared<SensorState>()};
    }
    catch (...)
    {
        return nullptr;
    }
}

extern "C" void score_diag_sensor_source_publish(ScoreDiagSensorSource* source,
                                                 std::int16_t temperature,
                                                 std::uint8_t available)
{
    std::lock_guard<std::mutex> guard{source->state->mutex};
    source->state->temperature = temperature;
    source->state->available = available != 0U;
}

extern "C" ScoreDiagApplication* score_diag_sensor_source_register(const ScoreDiagSensorSource* source)
{
    try
    {
        return score::opensovd::bridge::RegisterApplication("score-sensor",
                                                            "SCORE Sensor (Simulator)",
                                                            "score-demo",
                                                            {{"temperature.celsius",
                                                              "Temperature (degC)",
                                                              SCORE_DIAG_TEMPERATURE_CENTIDEGREES,
                                                              std::make_shared<SensorReader>(source->state, false)},
                                                             {"sensor.healthy",
                                                              "Sensor Available",
                                                              SCORE_DIAG_HEALTHY_BOOLEAN,
                                                              std::make_shared<SensorReader>(source->state, true)}});
    }
    catch (...)
    {
        return nullptr;
    }
}

extern "C" void score_diag_sensor_source_release(ScoreDiagSensorSource* source)
{
    if (source != nullptr)
    {
        std::lock_guard<std::mutex> guard{source->state->mutex};
        source->state->available = false;
    }
    delete source;
}
