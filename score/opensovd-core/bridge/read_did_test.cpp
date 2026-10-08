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
#include "score/opensovd-core/bridge/application.h"

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
    auto* reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    Completion reply;
    auto* request = score_diag_read_start(reader, &reply, Complete);
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
    auto handler = std::make_shared<DeferredReader>();
    auto* reader = score::opensovd::bridge::RegisterReadDataByIdentifier(handler);
    Completion reply;
    auto* request = score_diag_read_start(reader, &reply, Complete);
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

TEST(ApplicationRegistration, CatalogOwnsDescriptorsAndClonedReadersOutliveIt)
{
    auto handler = std::make_shared<DeferredReader>();
    auto* application = score::opensovd::bridge::RegisterApplication(
        "my-app", "My Application", "my-component", {{"my-data", "My Data", SCORE_DIAG_BYTES, handler}});
    ASSERT_NE(application, nullptr);
    EXPECT_STREQ(score_diag_application_id(application), "my-app");
    EXPECT_STREQ(score_diag_application_name(application), "My Application");
    EXPECT_STREQ(score_diag_application_component(application), "my-component");
    EXPECT_EQ(score_diag_application_resource_count(application), 1U);
    EXPECT_STREQ(score_diag_application_resource_id(application, 0U), "my-data");
    EXPECT_STREQ(score_diag_application_resource_name(application, 0U), "My Data");
    EXPECT_EQ(score_diag_application_resource_encoding(application, 0U), SCORE_DIAG_BYTES);
    auto* reader = score_diag_application_resource_reader(application, 0U);
    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(score_diag_application_resource_reader(application, 1U), nullptr);
    score_diag_application_release(application);
    Completion reply;
    auto* request = score_diag_read_start(reader, &reply, Complete);
    ASSERT_NE(request, nullptr);
    ASSERT_TRUE(handler->WaitForInvocation());
    EXPECT_TRUE(handler->promise.SetValue(Result<ByteVector>{ByteVector{std::byte{0x42}}}).has_value());
    EXPECT_TRUE(reply.Wait());
    EXPECT_EQ(reply.status, SCORE_DIAG_READ_OK);
    EXPECT_EQ(reply.bytes, (std::vector<std::uint8_t>{0x42}));
    score_diag_read_release(request);
    score_diag_reader_release(reader);
}

TEST(ApplicationRegistration, RejectsMissingHandlersAndDuplicateResources)
{
    using score::opensovd::bridge::RegisterApplication;
    auto handler = std::make_shared<DeferredReader>();
    EXPECT_EQ(RegisterApplication("", "App", "Component", {{"data", "Data", SCORE_DIAG_BYTES, handler}}), nullptr);
    EXPECT_EQ(RegisterApplication("app", "App", "Component", {}), nullptr);
    EXPECT_EQ(RegisterApplication("app", "App", "Component", {{"data", "Data", SCORE_DIAG_BYTES, nullptr}}), nullptr);
    EXPECT_EQ(RegisterApplication(
                  "app",
                  "App",
                  "Component",
                  {{"data", "Data", SCORE_DIAG_BYTES, handler}, {"data", "Other", SCORE_DIAG_BYTES, handler}}),
              nullptr);
}
}  // namespace
