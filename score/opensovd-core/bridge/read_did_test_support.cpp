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
#include "score/opensovd-core/bridge/read_did_test_support.h"

#include <memory>
#include <mutex>
#include <vector>

namespace
{
using score::mw::diag::ByteVector;
using score::mw::diag::Future;
using score::mw::diag::Promise;
using score::mw::diag::uds::MetaData;
using score::mw::diag::uds::ReadDataByIdentifier;
using score::mw::diag::uds::Result;

/// Reader whose reads stay pending until the test completes them, so tests synchronize on state instead of time.
class ControlledReader final : public ReadDataByIdentifier
{
  public:
    ControlledReader() : ReadDataByIdentifier{} {}

    Future<Result<ByteVector>> Read(const MetaData&, score::cpp::stop_token token) override
    {
        std::lock_guard<std::mutex> guard{mutex_};
        Promise<Result<ByteVector>> promise;
        auto future = promise.GetInterruptibleFuture().value();
        promises_.push_back(std::move(promise));
        tokens_.push_back(token);
        return future;
    }

    std::size_t Started()
    {
        std::lock_guard<std::mutex> guard{mutex_};
        return tokens_.size();
    }

    std::size_t Stopped()
    {
        std::lock_guard<std::mutex> guard{mutex_};
        std::size_t stopped{0U};
        for (const auto& token : tokens_)
        {
            if (token.stop_requested())
            {
                ++stopped;
            }
        }
        return stopped;
    }

    void Complete()
    {
        std::vector<Promise<Result<ByteVector>>> promises;
        {
            std::lock_guard<std::mutex> guard{mutex_};
            promises = std::move(promises_);
        }
        for (auto& promise : promises)
        {
            score::cpp::ignore = promise.SetValue(Result<ByteVector>{ByteVector{std::byte{0x6f}, std::byte{0x6b}}});
        }
    }

  private:
    std::mutex mutex_;
    std::vector<Promise<Result<ByteVector>>> promises_;
    std::vector<score::cpp::stop_token> tokens_;
};
}  // namespace

struct ScoreDiagTestControl
{
    std::shared_ptr<ControlledReader> reader;
};

extern "C" ScoreDiagTestControl* score_diag_test_control_create(void)
{
    return new ScoreDiagTestControl{std::make_shared<ControlledReader>()};
}

extern "C" ScoreDiagReader* score_diag_test_control_register(const ScoreDiagTestControl* control)
{
    return score::opensovd::bridge::RegisterReadDataByIdentifier(control->reader);
}

extern "C" size_t score_diag_test_control_started(const ScoreDiagTestControl* control)
{
    return control->reader->Started();
}

extern "C" size_t score_diag_test_control_stopped(const ScoreDiagTestControl* control)
{
    return control->reader->Stopped();
}

extern "C" void score_diag_test_control_complete(ScoreDiagTestControl* control)
{
    control->reader->Complete();
}

extern "C" void score_diag_test_control_release(ScoreDiagTestControl* control)
{
    delete control;
}
