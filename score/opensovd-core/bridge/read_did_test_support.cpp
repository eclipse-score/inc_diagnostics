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
#include "score/opensovd-core/bridge/read_did.h"

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

    bool Stopped()
    {
        std::lock_guard<std::mutex> guard{mutex_};
        for (const auto& token : tokens_)
        {
            if (token.stop_requested())
            {
                return true;
            }
        }
        return false;
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
            score::cpp::ignore = promise.SetValue(Result<ByteVector>{ByteVector{}});
        }
    }

  private:
    std::mutex mutex_;
    std::vector<Promise<Result<ByteVector>>> promises_;
    std::vector<score::cpp::stop_token> tokens_;
};

using Control = std::shared_ptr<ControlledReader>;
}  // namespace

extern "C" void* score_diag_test_control_create()
{
    return new Control{std::make_shared<ControlledReader>()};
}

extern "C" ScoreDiagReader* score_diag_test_control_register(void* control)
{
    return score::opensovd::bridge::RegisterReadDataByIdentifier(*static_cast<Control*>(control)).release();
}

extern "C" void score_diag_test_control_release(void* control)
{
    delete static_cast<Control*>(control);
}

extern "C" std::size_t score_diag_test_control_started(void* control)
{
    return (*static_cast<Control*>(control))->Started();
}

extern "C" bool score_diag_test_control_stopped(void* control)
{
    return (*static_cast<Control*>(control))->Stopped();
}

extern "C" void score_diag_test_control_complete(void* control)
{
    (*static_cast<Control*>(control))->Complete();
}
