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
#include <optional>
#include <thread>
#include <utility>

static_assert(sizeof(ScoreDiagReadStatus) == 1U, "Rust declares ReadStatus as repr(u8)");
static_assert(SCORE_DIAG_READ_OK == 0 && SCORE_DIAG_READ_NRC == 1 && SCORE_DIAG_READ_CANCELLED == 2 &&
                  SCORE_DIAG_READ_FAILED == 3 && SCORE_DIAG_READ_BUSY == 4,
              "status values must match ReadStatus in read_did.rs");

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

// Owns one admission slot already reserved with fetch_add. It is released exactly once, before the
// completion callback runs, so completed requests do not keep their slot until the caller releases
// the request handle.
class Admission
{
  public:
    explicit Admission(std::shared_ptr<HandlerState> owner) : owner_{std::move(owner)} {}
    Admission(Admission&& other) noexcept : owner_{std::move(other.owner_)} {}
    Admission(const Admission&) = delete;
    Admission& operator=(const Admission&) = delete;
    Admission& operator=(Admission&&) = delete;
    ~Admission()
    {
        Release();
    }

    void Release()
    {
        if (owner_)
        {
            score::cpp::ignore = owner_->active_requests.fetch_sub(1U);
            owner_.reset();
        }
    }

  private:
    std::shared_ptr<HandlerState> owner_;
};

struct RequestState
{
    std::shared_ptr<HandlerState> owner;
    score::cpp::stop_source stop_source;
    void* context;
    ScoreDiagReadCompletion completion;
};

enum class BridgeErrc : score::result::ErrorCode
{
    kInvalidArgument = 1,
    kBusy = 2,
};

class BridgeErrorDomain final : public score::result::ErrorDomain
{
  public:
    std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override
    {
        switch (static_cast<BridgeErrc>(code))
        {
            case BridgeErrc::kInvalidArgument:
                return "invalid argument";
            case BridgeErrc::kBusy:
                return "reader busy";
        }
        return "undefined bridge error";
    }
};

const score::result::ErrorDomain& GetBridgeErrorDomain() noexcept
{
    static constexpr BridgeErrorDomain bridge_error_domain{};
    return bridge_error_domain;
}

score::result::Error MakeBridgeError(const BridgeErrc code) noexcept
{
    return score::result::Error{static_cast<score::result::ErrorCode>(code), GetBridgeErrorDomain(), ""};
}

ScoreDiagStatus ToStatus(const score::result::Error& error) noexcept
{
    switch (static_cast<BridgeErrc>(*error))
    {
        case BridgeErrc::kInvalidArgument:
            return SCORE_DIAG_INVALID_ARGUMENT;
        case BridgeErrc::kBusy:
            return SCORE_DIAG_BUSY;
    }
    return SCORE_DIAG_FAILED;
}

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

void Complete(RequestState& state,
              Admission& admission,
              std::uint8_t status,
              std::uint8_t nrc,
              const std::uint8_t* data,
              std::size_t size)
{
    admission.Release();
    state.completion(state.context, status, nrc, data, size);
}

void CompleteRead(const std::shared_ptr<RequestState>& state, Admission& admission)
{
    try
    {
        std::optional<Future<Result<ByteVector>>> future;
        {
            std::lock_guard<std::mutex> guard{state->owner->dispatch_mutex};
            if (!state->stop_source.stop_requested())
            {
                future.emplace(state->owner->handler->Read(MetaData{}, state->stop_source.get_token()));
            }
        }
        if (!future.has_value())
        {
            Complete(*state, admission, SCORE_DIAG_READ_CANCELLED, 0U, nullptr, 0U);
            return;
        }
        auto completion = future->Get(state->stop_source.get_token());
        if (state->stop_source.stop_requested())
        {
            Complete(*state, admission, SCORE_DIAG_READ_CANCELLED, 0U, nullptr, 0U);
        }
        else if (!completion.has_value())
        {
            Complete(*state, admission, SCORE_DIAG_READ_FAILED, 0U, nullptr, 0U);
        }
        else if (!completion.value().has_value())
        {
            const auto nrc = score::mw::diag::uds::ToNegativeResponseCode(completion.value().error());
            if (nrc.has_value())
            {
                Complete(*state, admission, SCORE_DIAG_READ_NRC, static_cast<std::uint8_t>(nrc.value()), nullptr, 0U);
            }
            else
            {
                Complete(*state, admission, SCORE_DIAG_READ_FAILED, 0U, nullptr, 0U);
            }
        }
        else
        {
            const auto& bytes = completion.value().value();
            Complete(*state,
                     admission,
                     SCORE_DIAG_READ_OK,
                     0U,
                     reinterpret_cast<const std::uint8_t*>(bytes.data()),
                     bytes.size());
        }
    }
    catch (...)
    {
        // Detached worker: nothing above can catch. Handler code runs here, so a throw must become a failed read.
        Complete(*state, admission, SCORE_DIAG_READ_FAILED, 0U, nullptr, 0U);
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

namespace
{
score::Result<ScoreDiagReadRequest*> StartRead(ScoreDiagReader& reader,
                                               void* context,
                                               ScoreDiagReadCompletion completion)
{
    if (reader.state->active_requests.fetch_add(1U) >= 8U)
    {
        score::cpp::ignore = reader.state->active_requests.fetch_sub(1U);
        return score::MakeUnexpected<ScoreDiagReadRequest*>(MakeBridgeError(BridgeErrc::kBusy));
    }
    Admission admission{reader.state};
    auto state = std::make_shared<RequestState>();
    state->owner = reader.state;
    state->context = context;
    state->completion = completion;
    auto request = std::make_unique<ScoreDiagReadRequest>(ScoreDiagReadRequest{state});
    std::thread([state, admission = std::move(admission)]() mutable {
        CompleteRead(state, admission);
    }).detach();
    return request.release();
}
}  // namespace

namespace score::opensovd::bridge
{
score::Result<ScoreDiagReader*> RegisterReadDataByIdentifier(std::shared_ptr<ReadDataByIdentifier> handler)
{
    if (!handler)
    {
        return score::MakeUnexpected<ScoreDiagReader*>(MakeBridgeError(BridgeErrc::kInvalidArgument));
    }
    auto state = std::make_shared<HandlerState>();
    state->handler = std::move(handler);
    return new ScoreDiagReader{std::move(state)};
}
}  // namespace score::opensovd::bridge

// The C entry points are the boundary where exceptions from allocation or thread creation end: they
// cannot cross the C ABI, so each one turns them into SCORE_DIAG_FAILED.

extern "C" ScoreDiagStatus score_diag_demo_reader_create(std::uint32_t delay_ms,
                                                         std::uint8_t nrc,
                                                         ScoreDiagReader** out)
{
    if (out == nullptr)
    {
        return SCORE_DIAG_INVALID_ARGUMENT;
    }
    *out = nullptr;
    try
    {
        auto reader =
            score::opensovd::bridge::RegisterReadDataByIdentifier(std::make_shared<DemoReader>(delay_ms, nrc));
        if (!reader.has_value())
        {
            return ToStatus(reader.error());
        }
        *out = reader.value();
        return SCORE_DIAG_OK;
    }
    catch (...)
    {
        return SCORE_DIAG_FAILED;
    }
}

extern "C" ScoreDiagStatus score_diag_reader_clone(const ScoreDiagReader* reader, ScoreDiagReader** out)
{
    if (reader == nullptr || out == nullptr)
    {
        return SCORE_DIAG_INVALID_ARGUMENT;
    }
    *out = nullptr;
    try
    {
        *out = new ScoreDiagReader{reader->state};
        return SCORE_DIAG_OK;
    }
    catch (...)
    {
        return SCORE_DIAG_FAILED;
    }
}

extern "C" void score_diag_reader_release(ScoreDiagReader* reader)
{
    delete reader;
}

extern "C" ScoreDiagStatus score_diag_read_start(ScoreDiagReader* reader,
                                                 void* context,
                                                 ScoreDiagReadCompletion completion,
                                                 ScoreDiagReadRequest** out)
{
    if (reader == nullptr || completion == nullptr || out == nullptr)
    {
        return SCORE_DIAG_INVALID_ARGUMENT;
    }
    *out = nullptr;
    try
    {
        auto request = StartRead(*reader, context, completion);
        if (!request.has_value())
        {
            return ToStatus(request.error());
        }
        *out = request.value();
        return SCORE_DIAG_OK;
    }
    catch (...)
    {
        return SCORE_DIAG_FAILED;
    }
}

extern "C" void score_diag_read_cancel(ScoreDiagReadRequest* request)
{
    if (request == nullptr)
    {
        return;
    }
    score::cpp::ignore = request->state->stop_source.request_stop();
}

extern "C" void score_diag_read_release(ScoreDiagReadRequest* request)
{
    delete request;
}
