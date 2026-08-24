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

/// @file meta_data_test.cpp
/// @brief Unit tests for MetaData, DiagnosticSession, and AddressingMode.

#include "score/mw/diag/uds/meta_data.h"
#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/negative_response_code.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds
{

// ── Default field values ───────────────────────────────────────────────────

TEST(MetaDataTest, DefaultInitializationHasExpectedValues)
{
    const MetaData meta_data{};

    EXPECT_EQ(meta_data.session, DiagnosticSession::Default);
    EXPECT_FALSE(meta_data.security_level.has_value());
    EXPECT_EQ(meta_data.addressing, AddressingMode::Physical);
    EXPECT_FALSE(meta_data.source_address.has_value());
    EXPECT_FALSE(meta_data.target_address.has_value());
}

// ── DiagnosticSession enum values (ISO 14229-1:2020, Service 0x10) ─────────

TEST(DiagnosticSessionTest, EnumValuesMatchIsoSpecification)
{
    EXPECT_EQ(static_cast<std::uint8_t>(DiagnosticSession::Default), 0x01U);
    EXPECT_EQ(static_cast<std::uint8_t>(DiagnosticSession::Programming), 0x02U);
    EXPECT_EQ(static_cast<std::uint8_t>(DiagnosticSession::Extended), 0x03U);
    EXPECT_EQ(static_cast<std::uint8_t>(DiagnosticSession::SafetySystem), 0x04U);
}

// ── Handler gating on session ──────────────────────────────────────────────

/// Simulates a handler that is only callable from an Extended or Programming session.
static Result<ByteVector> SessionRestrictedRead(const MetaData& meta_data)
{
    if (meta_data.session == DiagnosticSession::Default)
    {
        return score::MakeUnexpected(NegativeResponseCode::ConditionsNotCorrect);
    }
    return ByteVector{std::byte{0x01U}, std::byte{0x02U}};
}

TEST(MetaDataTest, HandlerRejectsDefaultSession)
{
    const MetaData meta{};  // Default session
    const auto result = SessionRestrictedRead(meta);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().Message(), "Conditions not correct");
}

// ── Handler gating on security level ──────────────────────────────────────

/// Simulates a handler that requires an unlocked security access level.
static Result<ByteVector> SecurityRestrictedRead(const MetaData& meta_data)
{
    if (!meta_data.security_level.has_value())
    {
        return score::MakeUnexpected(NegativeResponseCode::SecurityAccessDenied);
    }
    return ByteVector{std::byte{0xAAU}};
}

TEST(MetaDataTest, HandlerRejectsWhenSecurityLocked)
{
    const MetaData meta{};  // security_level is std::nullopt (locked)
    const auto result = SecurityRestrictedRead(meta);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().Message(), "Security access denied");
}

}  // namespace score::mw::diag::uds
