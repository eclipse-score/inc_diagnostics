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
#include "score/mw/diag/uds/negative_response_code.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

namespace
{
using score::mw::diag::ByteVector;
using score::mw::diag::Future;
using score::mw::diag::uds::MetaData;
using score::mw::diag::uds::NegativeResponseCode;
using score::mw::diag::uds::ReadDataByIdentifier;
using score::mw::diag::uds::Result;

struct HandlerState
{
    std::shared_ptr<ReadDataByIdentifier> handler;
    std::mutex dispatch_mutex;
    std::atomic<std::size_t> active_requests{0U};
};

struct RequestState
{
    ~RequestState()
    {
        if (owner)
        {
            score::cpp::ignore = owner->active_requests.fetch_sub(1U);
        }
    }

    std::shared_ptr<HandlerState> owner;
    score::cpp::stop_source stop_source;
    void* context;
    ScoreDiagReadCompletion completion;
};

class DemoReader final : public ReadDataByIdentifier
{
  public:
    DemoReader(std::uint32_t delay_ms, std::uint8_t nrc) : delay_ms_{delay_ms}, nrc_{nrc} {}

    Future<Result<ByteVector>> Read(const MetaData& meta_data, score::cpp::stop_token stop_token) override
    {
        if (meta_data.session != score::mw::diag::uds::DiagnosticSession::Default ||
            meta_data.security_level.has_value())
        {
            return score::mw::diag::WrapAsFuture(
                Result<ByteVector>{score::MakeUnexpected(NegativeResponseCode::SecurityAccessDenied)});
        }
        score::mw::diag::Promise<Result<ByteVector>> promise;
        auto future = promise.GetInterruptibleFuture().value();
        const auto delay_ms = delay_ms_;
        const auto nrc = nrc_;
        std::thread([promise = std::move(promise), delay_ms, nrc, stop_token]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds{delay_ms});
            if (stop_token.stop_requested())
            {
                return;
            }
            if (nrc != 0U)
            {
                score::cpp::ignore =
                    promise.SetValue(Result<ByteVector>{score::MakeUnexpected(static_cast<NegativeResponseCode>(nrc))});
            }
            else
            {
                score::cpp::ignore = promise.SetValue(Result<ByteVector>{
                    ByteVector{std::byte{0x31}, std::byte{0x2e}, std::byte{0x30}, std::byte{0x2e}, std::byte{0x30}}});
            }
        }).detach();
        return future;
    }

  private:
    std::uint32_t delay_ms_;
    std::uint8_t nrc_;
};

void CompleteRead(const std::shared_ptr<RequestState>& state)
{
    try
    {
        auto future = [&state]() {
            std::lock_guard<std::mutex> guard{state->owner->dispatch_mutex};
            return state->owner->handler->Read(MetaData{}, state->stop_source.get_token());
        }();
        auto completion = future.Get(state->stop_source.get_token());
        if (state->stop_source.stop_requested())
        {
            state->completion(state->context, SCORE_DIAG_READ_CANCELLED, 0U, nullptr, 0U);
        }
        else if (!completion.has_value())
        {
            state->completion(state->context, SCORE_DIAG_READ_FAILED, 0U, nullptr, 0U);
        }
        else if (!completion.value().has_value())
        {
            const auto nrc = score::mw::diag::uds::ToNegativeResponseCode(*completion.value().error());
            if (nrc.has_value())
            {
                state->completion(
                    state->context, SCORE_DIAG_READ_NRC, static_cast<std::uint8_t>(nrc.value()), nullptr, 0U);
            }
            else
            {
                state->completion(state->context, SCORE_DIAG_READ_FAILED, 0U, nullptr, 0U);
            }
        }
        else
        {
            const auto& bytes = completion.value().value();
            state->completion(state->context,
                              SCORE_DIAG_READ_OK,
                              0U,
                              reinterpret_cast<const std::uint8_t*>(bytes.data()),
                              bytes.size());
        }
    }
    catch (...)
    {
        state->completion(state->context, SCORE_DIAG_READ_FAILED, 0U, nullptr, 0U);
    }
}
}  // namespace

struct ScoreDiagReader
{
    std::shared_ptr<HandlerState> state;
};

struct ScoreDiagReadRequest
{
    std::shared_ptr<RequestState> state;
};

namespace score::opensovd::bridge
{
ScoreDiagReader* RegisterReadDataByIdentifier(std::shared_ptr<ReadDataByIdentifier> handler)
{
    if (!handler)
    {
        return nullptr;
    }
    auto state = std::make_shared<HandlerState>();
    state->handler = std::move(handler);
    return new ScoreDiagReader{std::move(state)};
}
}  // namespace score::opensovd::bridge

extern "C" ScoreDiagReader* score_diag_demo_reader_create(std::uint32_t delay_ms, std::uint8_t nrc)
{
    try
    {
        return score::opensovd::bridge::RegisterReadDataByIdentifier(std::make_shared<DemoReader>(delay_ms, nrc));
    }
    catch (...)
    {
        return nullptr;
    }
}

extern "C" ScoreDiagReader* score_diag_reader_clone(const ScoreDiagReader* reader)
{
    try
    {
        return reader == nullptr ? nullptr : new ScoreDiagReader{reader->state};
    }
    catch (...)
    {
        return nullptr;
    }
}

extern "C" void score_diag_reader_release(ScoreDiagReader* reader)
{
    delete reader;
}

extern "C" ScoreDiagReadRequest* score_diag_read_start(ScoreDiagReader* reader,
                                                       void* context,
                                                       ScoreDiagReadCompletion completion)
{
    std::unique_ptr<ScoreDiagReadRequest> request;
    try
    {
        auto state = std::make_shared<RequestState>();
        if (reader->state->active_requests.fetch_add(1U) >= 8U)
        {
            score::cpp::ignore = reader->state->active_requests.fetch_sub(1U);
            completion(context, SCORE_DIAG_READ_BUSY, 0U, nullptr, 0U);
            return nullptr;
        }
        state->owner = reader->state;
        state->context = context;
        state->completion = completion;
        request = std::make_unique<ScoreDiagReadRequest>(ScoreDiagReadRequest{state});
        std::thread([state]() {
            CompleteRead(state);
        }).detach();
        return request.release();
    }
    catch (...)
    {
        completion(context, SCORE_DIAG_READ_FAILED, 0U, nullptr, 0U);
        return nullptr;
    }
}

extern "C" void score_diag_read_cancel(ScoreDiagReadRequest* request)
{
    score::cpp::ignore = request->state->stop_source.request_stop();
}

extern "C" void score_diag_read_release(ScoreDiagReadRequest* request)
{
    delete request;
}
