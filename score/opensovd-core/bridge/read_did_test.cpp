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

struct Completion
{
    std::mutex mutex;
    std::condition_variable changed;
    std::size_t count{0U};
    std::uint8_t status{SCORE_DIAG_READ_FAILED};
    std::vector<std::uint8_t> bytes;

    bool Wait()
    {
        std::unique_lock<std::mutex> guard{mutex};
        return changed.wait_for(guard, std::chrono::seconds{2}, [this]() {
            return count != 0U;
        });
    }
};

void Complete(void* context, std::uint8_t status, std::uint8_t, const std::uint8_t* bytes, std::size_t size)
{
    auto& reply = *static_cast<Completion*>(context);
    std::lock_guard<std::mutex> guard{reply.mutex};
    reply.status = status;
    if (size != 0U)
    {
        reply.bytes.assign(bytes, bytes + size);
    }
    ++reply.count;
    reply.changed.notify_one();
}

ScoreDiagReader* Register(const std::shared_ptr<ReadDataByIdentifier>& handler)
{
    auto reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    EXPECT_TRUE(reader.has_value());
    return reader.has_value() ? reader.value() : nullptr;
}

ScoreDiagReadRequest* StartRead(ScoreDiagReader* reader, void* context, ScoreDiagReadCompletion completion)
{
    ScoreDiagReadRequest* request = nullptr;
    EXPECT_EQ(score_diag_read_start(reader, context, completion, &request), SCORE_DIAG_OK);
    return request;
}

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

class GatedReader final : public ReadDataByIdentifier
{
  public:
    Future<Result<ByteVector>> Read(const MetaData&, score::cpp::stop_token) override
    {
        std::unique_lock<std::mutex> guard{mutex};
        ++invocations;
        changed.notify_all();
        changed.wait(guard, [this]() {
            return open;
        });
        return promise.GetInterruptibleFuture().value();
    }

    bool WaitForInvocations(std::size_t count)
    {
        std::unique_lock<std::mutex> guard{mutex};
        return changed.wait_for(guard, std::chrono::seconds{2}, [this, count]() {
            return invocations >= count;
        });
    }

    void Open()
    {
        std::lock_guard<std::mutex> guard{mutex};
        open = true;
        changed.notify_all();
    }

    std::size_t Invocations()
    {
        std::lock_guard<std::mutex> guard{mutex};
        return invocations;
    }

    score::mw::diag::Promise<Result<ByteVector>> promise;

  private:
    std::mutex mutex;
    std::condition_variable changed;
    std::size_t invocations{0U};
    bool open{false};
};

TEST(ReadDidBridge, PendingReadSurvivesRegistrationRelease)
{
    RecordProperty("PartiallyVerifies", "comp_req__uds_data_bridge__read_handlers");
    RecordProperty("Description",
                   "Check that a read started on a registered ReadDataByIdentifier handler completes with its data "
                   "after the registration is released.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto handler = std::make_shared<DeferredReader>();
    auto* reader = Register(handler);
    Completion reply;
    auto* request = StartRead(reader, &reply, Complete);
    ASSERT_NE(request, nullptr);
    ASSERT_TRUE(handler->WaitForInvocation());
    score_diag_reader_release(reader);
    EXPECT_EQ(handler->received_metadata.session, score::mw::diag::uds::DiagnosticSession::Default);
    EXPECT_FALSE(handler->received_metadata.security_level.has_value());
    EXPECT_EQ(handler->received_metadata.addressing, score::mw::diag::uds::AddressingMode::Physical);
    EXPECT_FALSE(handler->received_metadata.source_address.has_value());
    EXPECT_FALSE(handler->received_metadata.target_address.has_value());
    EXPECT_TRUE(
        handler->promise.SetValue(Result<ByteVector>{ByteVector{std::byte{0x00}, std::byte{0xff}}}).has_value());
    EXPECT_TRUE(reply.Wait());
    EXPECT_EQ(reply.status, SCORE_DIAG_READ_OK);
    EXPECT_EQ(reply.count, 1U);
    EXPECT_EQ(reply.bytes, (std::vector<std::uint8_t>{0x00, 0xff}));
    score_diag_read_release(request);
}

TEST(ReadDidBridge, CancellationCompletesOnceAndStopsHandlerToken)
{
    RecordProperty("PartiallyVerifies", "comp_req__uds_data_bridge__read_timeout");
    RecordProperty(
        "Description",
        "Check that cancelling a pending read completes it once as cancelled and requests the handler to stop.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto handler = std::make_shared<DeferredReader>();
    auto* reader = Register(handler);
    Completion reply;
    auto* request = StartRead(reader, &reply, Complete);
    ASSERT_NE(request, nullptr);
    ASSERT_TRUE(handler->WaitForInvocation());
    score_diag_read_cancel(request);
    EXPECT_TRUE(reply.Wait());
    EXPECT_EQ(reply.status, SCORE_DIAG_READ_CANCELLED);
    EXPECT_TRUE(handler->received_token.stop_requested());
    EXPECT_TRUE(handler->promise.SetValue(Result<ByteVector>{ByteVector{std::byte{0x12}}}).has_value());
    EXPECT_EQ(reply.count, 1U);
    score_diag_read_release(request);
    score_diag_reader_release(reader);
}

TEST(ReadDidBridge, ReadCancelledWhileQueuedIsNotDispatched)
{
    RecordProperty("PartiallyVerifies", "comp_req__uds_data_bridge__read_timeout");
    RecordProperty("Description",
                   "Check that a read cancelled while it waits for the dispatch of another read is never passed to "
                   "the handler.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto handler = std::make_shared<GatedReader>();
    auto* reader = Register(handler);
    Completion first_reply;
    Completion queued_reply;
    auto* first = StartRead(reader, &first_reply, Complete);
    ASSERT_NE(first, nullptr);
    ASSERT_TRUE(handler->WaitForInvocations(1U));
    auto* queued = StartRead(reader, &queued_reply, Complete);
    ASSERT_NE(queued, nullptr);
    score_diag_read_cancel(queued);
    handler->Open();
    EXPECT_TRUE(queued_reply.Wait());
    EXPECT_EQ(queued_reply.status, SCORE_DIAG_READ_CANCELLED);
    EXPECT_TRUE(handler->promise.SetValue(Result<ByteVector>{ByteVector{std::byte{0x12}}}).has_value());
    EXPECT_TRUE(first_reply.Wait());
    EXPECT_EQ(first_reply.status, SCORE_DIAG_READ_OK);
    EXPECT_EQ(handler->Invocations(), 1U);
    score_diag_read_release(queued);
    score_diag_read_release(first);
    score_diag_reader_release(reader);
}

TEST(ReadDidBridge, CompletedReadsDoNotHoldAdmissionUntilReleased)
{
    RecordProperty("PartiallyVerifies", "comp_req__uds_data_bridge__read_admission");
    RecordProperty("Description",
                   "Check that completed reads keep no admission slot, so that a ninth read starts after eight "
                   "completed reads whose handles are still held.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    ScoreDiagReader* reader = nullptr;
    ASSERT_EQ(score_diag_demo_reader_create(0U, 0U, &reader), SCORE_DIAG_OK);
    ASSERT_NE(reader, nullptr);
    std::vector<Completion> replies(9U);
    std::vector<ScoreDiagReadRequest*> requests;
    for (std::size_t index = 0U; index < 8U; ++index)
    {
        requests.push_back(StartRead(reader, &replies[index], Complete));
        ASSERT_NE(requests.back(), nullptr);
        EXPECT_TRUE(replies[index].Wait());
        EXPECT_EQ(replies[index].status, SCORE_DIAG_READ_OK);
    }
    requests.push_back(StartRead(reader, &replies[8U], Complete));
    EXPECT_NE(requests.back(), nullptr);
    EXPECT_TRUE(replies[8U].Wait());
    EXPECT_EQ(replies[8U].status, SCORE_DIAG_READ_OK);
    for (auto* request : requests)
    {
        score_diag_read_release(request);
    }
    score_diag_reader_release(reader);
}
}  // namespace
