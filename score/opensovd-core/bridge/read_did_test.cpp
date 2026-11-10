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

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace
{
using score::mw::diag::ByteVector;
using score::mw::diag::Future;
using score::mw::diag::uds::MetaData;
using score::mw::diag::uds::ReadDataByIdentifier;
using score::mw::diag::uds::Result;
using score::opensovd::bridge::ReaderPtr;
using score::opensovd::bridge::ReadRequestPtr;

struct Completion
{
    struct Reply
    {
        std::size_t count;
        std::uint8_t status;
        std::uint8_t nrc;
        std::vector<std::uint8_t> bytes;
    };

    std::mutex mutex;
    std::condition_variable changed;
    std::size_t count{0U};
    std::uint8_t status{SCORE_DIAG_READ_FAILED};
    std::uint8_t nrc{0U};
    std::vector<std::uint8_t> bytes;

    bool Wait()
    {
        std::unique_lock<std::mutex> guard{mutex};
        return changed.wait_for(guard, std::chrono::seconds{2}, [this]() {
            return count != 0U;
        });
    }

    Reply Snapshot()
    {
        std::lock_guard<std::mutex> guard{mutex};
        return Reply{count, status, nrc, bytes};
    }
};

void Complete(void* context, std::uint8_t status, std::uint8_t nrc, const std::uint8_t* bytes, std::size_t size)
{
    const std::unique_ptr<std::shared_ptr<Completion>> retained{static_cast<std::shared_ptr<Completion>*>(context)};
    auto& reply = **retained;
    std::lock_guard<std::mutex> guard{reply.mutex};
    reply.status = status;
    reply.nrc = nrc;
    if (size != 0U)
    {
        reply.bytes.assign(bytes, bytes + size);
    }
    ++reply.count;
    reply.changed.notify_one();
}

struct PendingRead
{
    explicit PendingRead(ScoreDiagReader* reader)
    {
        auto context = std::make_unique<std::shared_ptr<Completion>>(completion);
        request = score::opensovd::bridge::StartRead(reader, context.release(), &Complete);
    }

    ~PendingRead()
    {
        if (request)
        {
            score_diag_read_cancel(request.get());
        }
    }

    std::shared_ptr<Completion> completion{std::make_shared<Completion>()};
    ReadRequestPtr request;
};

class DeferredReader final : public ReadDataByIdentifier
{
  public:
    Future<Result<ByteVector>> Read(const MetaData& metadata, score::cpp::stop_token stop_token) override
    {
        std::lock_guard<std::mutex> guard{mutex};
        received_metadata = metadata;
        received_token = stop_token;
        invoked = true;
        changed.notify_one();
        return promise.GetInterruptibleFuture().value();
    }

    bool WaitForInvocation()
    {
        std::unique_lock<std::mutex> guard{mutex};
        return changed.wait_for(guard, std::chrono::seconds{2}, [this]() {
            return invoked;
        });
    }

    score::mw::diag::Promise<Result<ByteVector>> promise;
    MetaData received_metadata;
    score::cpp::stop_token received_token;

  private:
    std::mutex mutex;
    std::condition_variable changed;
    bool invoked{false};
};

TEST(ReadDidBridge, PendingReadSurvivesRegistrationRelease)
{
    auto handler = std::make_shared<DeferredReader>();
    ReaderPtr reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    PendingRead read{reader.get()};
    ASSERT_NE(read.request, nullptr);
    ASSERT_TRUE(handler->WaitForInvocation());
    reader.reset();
    EXPECT_EQ(handler->received_metadata.session, score::mw::diag::uds::DiagnosticSession::Default);
    EXPECT_FALSE(handler->received_metadata.security_level.has_value());
    EXPECT_EQ(handler->received_metadata.addressing, score::mw::diag::uds::AddressingMode::Physical);
    EXPECT_FALSE(handler->received_metadata.source_address.has_value());
    EXPECT_FALSE(handler->received_metadata.target_address.has_value());
    EXPECT_TRUE(
        handler->promise
            .SetValue(Result<ByteVector>{ByteVector{static_cast<std::byte>(0x00U), static_cast<std::byte>(0xffU)}})
            .has_value());
    ASSERT_TRUE(read.completion->Wait());
    const auto reply = read.completion->Snapshot();
    EXPECT_EQ(reply.status, SCORE_DIAG_READ_OK);
    EXPECT_EQ(reply.count, 1U);
    EXPECT_EQ(reply.bytes, (std::vector<std::uint8_t>{0x00, 0xff}));
}

TEST(ReadDidBridge, CancellationCompletesOnceAndStopsHandlerToken)
{
    auto handler = std::make_shared<DeferredReader>();
    ReaderPtr reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    PendingRead read{reader.get()};
    ASSERT_NE(read.request, nullptr);
    ASSERT_TRUE(handler->WaitForInvocation());
    score_diag_read_cancel(read.request.get());
    ASSERT_TRUE(read.completion->Wait());
    const auto reply = read.completion->Snapshot();
    EXPECT_EQ(reply.status, SCORE_DIAG_READ_CANCELLED);
    EXPECT_TRUE(handler->received_token.stop_requested());
    EXPECT_TRUE(handler->promise.SetValue(Result<ByteVector>{ByteVector{static_cast<std::byte>(0x12U)}}).has_value());
    EXPECT_EQ(reply.count, 1U);
}

class PendingReader final : public ReadDataByIdentifier
{
  public:
    Future<Result<ByteVector>> Read(const MetaData&, score::cpp::stop_token) override
    {
        std::lock_guard<std::mutex> guard{mutex_};
        auto promise = std::make_shared<score::mw::diag::Promise<Result<ByteVector>>>();
        auto future = promise->GetInterruptibleFuture().value();
        promises_.push_back(std::move(promise));
        changed_.notify_one();
        return future;
    }

    bool WaitForInvocations(std::size_t count)
    {
        std::unique_lock<std::mutex> guard{mutex_};
        return changed_.wait_for(guard, std::chrono::seconds{2}, [&]() {
            return promises_.size() == count;
        });
    }

  private:
    std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<std::shared_ptr<score::mw::diag::Promise<Result<ByteVector>>>> promises_;
};

TEST(ReadDidBridge, DuplicateRegistrationsShareAdmissionLimit)
{
    auto handler = std::make_shared<PendingReader>();
    auto first = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    auto second = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    std::vector<std::unique_ptr<PendingRead>> reads;
    for (std::size_t index = 0U; index < 8U; ++index)
    {
        reads.push_back(std::make_unique<PendingRead>(first.get()));
        ASSERT_NE(reads.back()->request, nullptr);
    }
    ASSERT_TRUE(handler->WaitForInvocations(8U));
    PendingRead rejected{second.get()};
    EXPECT_EQ(rejected.request, nullptr);
    ASSERT_TRUE(rejected.completion->Wait());
    EXPECT_EQ(rejected.completion->Snapshot().status, SCORE_DIAG_READ_BUSY);
}

TEST(ReadDidBridge, CallbackStateSurvivesRequestOwnerRelease)
{
    auto handler = std::make_shared<DeferredReader>();
    auto reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    std::shared_ptr<Completion> completion;
    {
        PendingRead read{reader.get()};
        completion = read.completion;
        ASSERT_TRUE(handler->WaitForInvocation());
    }
    ASSERT_TRUE(completion->Wait());
    EXPECT_EQ(completion->Snapshot().status, SCORE_DIAG_READ_CANCELLED);
}

class ErrorReader final : public ReadDataByIdentifier
{
  public:
    explicit ErrorReader(score::result::Error error) : ReadDataByIdentifier{}, error_{error} {}

    Future<Result<ByteVector>> Read(const MetaData&, score::cpp::stop_token) override
    {
        return score::mw::diag::WrapAsFuture(Result<ByteVector>{score::Unexpected{error_}});
    }

  private:
    score::result::Error error_;
};

TEST(ReadDidBridge, ManufacturerSpecificNrcIsPreserved)
{
    const auto nrc = score::mw::diag::uds::VehicleManufacturerSpecificCNC::FromValue<0xF0U>();
    auto handler = std::make_shared<ErrorReader>(score::mw::diag::uds::MakeError(nrc));
    auto reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    PendingRead read{reader.get()};
    ASSERT_TRUE(read.completion->Wait());
    const auto reply = read.completion->Snapshot();
    EXPECT_EQ(reply.status, SCORE_DIAG_READ_NRC);
    EXPECT_EQ(reply.nrc, 0xF0U);
}

TEST(ReadDidBridge, UnrelatedErrorIsNotReportedAsNrc)
{
    class ForeignDomain final : public score::result::ErrorDomain
    {
      public:
        std::string_view MessageFor(const score::result::ErrorCode&) const noexcept override
        {
            return "Foreign error";
        }
    };
    static const ForeignDomain domain;
    auto handler = std::make_shared<ErrorReader>(score::result::Error{0x33, domain, ""});
    auto reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    PendingRead read{reader.get()};
    ASSERT_TRUE(read.completion->Wait());
    EXPECT_EQ(read.completion->Snapshot().status, SCORE_DIAG_READ_FAILED);
}

TEST(ReadDidBridge, CompletedHandlesDoNotRetainAdmissionPermits)
{
    ReaderPtr reader{score_diag_demo_reader_create(0U, 0U)};
    ASSERT_NE(reader, nullptr);
    std::vector<std::unique_ptr<PendingRead>> reads;
    for (std::size_t index = 0U; index < 9U; ++index)
    {
        reads.push_back(std::make_unique<PendingRead>(reader.get()));
        ASSERT_NE(reads.back()->request, nullptr);
        ASSERT_TRUE(reads.back()->completion->Wait());
        EXPECT_EQ(reads.back()->completion->Snapshot().status, SCORE_DIAG_READ_OK);
    }
}

class BlockingReader final : public ReadDataByIdentifier
{
  public:
    Future<Result<ByteVector>> Read(const MetaData&, score::cpp::stop_token) override
    {
        std::unique_lock<std::mutex> guard{mutex};
        ++invocations;
        changed.notify_all();
        changed.wait_for(guard, std::chrono::seconds{2}, [&]() {
            return released;
        });
        return score::mw::diag::WrapAsFuture(Result<ByteVector>{ByteVector{}});
    }

    std::mutex mutex;
    std::condition_variable changed;
    std::size_t invocations{0U};
    bool released{false};
};

TEST(ReadDidBridge, CancelledQueuedRequestDoesNotInvokeHandler)
{
    auto handler = std::make_shared<BlockingReader>();
    auto reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    auto second_reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    PendingRead first{reader.get()};
    {
        std::unique_lock<std::mutex> guard{handler->mutex};
        ASSERT_TRUE(handler->changed.wait_for(guard, std::chrono::seconds{2}, [&]() {
            return handler->invocations == 1U;
        }));
    }
    PendingRead second{second_reader.get()};
    ASSERT_NE(second.request, nullptr);
    score_diag_read_cancel(second.request.get());
    {
        std::lock_guard<std::mutex> guard{handler->mutex};
        handler->released = true;
        handler->changed.notify_all();
    }
    ASSERT_TRUE(first.completion->Wait());
    ASSERT_TRUE(second.completion->Wait());
    EXPECT_EQ(second.completion->Snapshot().status, SCORE_DIAG_READ_CANCELLED);
    std::lock_guard<std::mutex> guard{handler->mutex};
    EXPECT_EQ(handler->invocations, 1U);
}
}  // namespace
