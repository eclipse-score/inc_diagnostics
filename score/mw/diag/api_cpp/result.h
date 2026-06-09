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

/// @file result.h
/// @brief Core diagnostic types: byte containers, payload encodings, error codes and Result.

#ifndef SCORE_MW_DIAG_RESULT_H
#define SCORE_MW_DIAG_RESULT_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace score
{
namespace mw
{
namespace diag
{

/************************************/
/* Byte container types             */
/************************************/

/// Owning byte buffer
using ByteVector = std::vector<std::uint8_t>;

/// Non-owning read-only view over a byte buffer
struct ByteView
{
    const std::uint8_t* ptr_{nullptr};
    std::size_t         len_{0U};

    ByteView() noexcept = default;
    constexpr ByteView(const std::uint8_t* p, std::size_t n) noexcept : ptr_{p}, len_{n} {}
    explicit ByteView(const ByteVector& v) noexcept : ptr_{v.data()}, len_{v.size()} {}

    const std::uint8_t* data()  const noexcept { return ptr_; }
    std::size_t         size()  const noexcept { return len_; }
    bool                empty() const noexcept { return len_ == 0U; }
    const std::uint8_t& operator[](std::size_t i) const noexcept { return ptr_[i]; }
    const std::uint8_t* begin() const noexcept { return ptr_; }
    const std::uint8_t* end()   const noexcept { return ptr_ + len_; }
};

/************************************/
/* Key-value attribute map          */
/************************************/

/// Insertion-order-preserving key/value map.
template <typename Key, typename Value>
class InsertionOrderedMap
{
  public:
    using value_type     = std::pair<Key, Value>;
    using iterator       = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    /// Insert with value; if key already exists the value is updated in-place.
    /// Returns {iterator-to-element, true} on new insert, {iterator, false} on update.
    std::pair<iterator, bool> emplace(Key key, Value value)
    {
        iterator it = find(key);
        if (it != end())
        {
            it->second = std::move(value);
            return {it, false};
        }
        entries_.emplace_back(std::move(key), std::move(value));
        return {entries_.end() - 1, true};
    }

    /// Access or default-insert by key (same semantics as std::map::operator[]).
    Value& operator[](const Key& key)
    {
        iterator it = find(key);
        if (it == end())
        {
            entries_.emplace_back(key, Value{});
            return entries_.back().second;
        }
        return it->second;
    }

    /// Read-only access by key. Precondition: key must exist (asserted).
    const Value& at(const Key& key) const noexcept
    {
        const_iterator it = find(key);
        assert(it != cend());
        return it->second;
    }

    /// Read-write access by key. Precondition: key must exist (asserted).
    Value& at(const Key& key) noexcept
    {
        iterator it = find(key);
        assert(it != end());
        return it->second;
    }

    iterator find(const Key& key) noexcept
    {
        return std::find_if(entries_.begin(), entries_.end(),
                            [&key](const value_type& e) noexcept { return e.first == key; });
    }

    const_iterator find(const Key& key) const noexcept
    {
        return std::find_if(entries_.cbegin(), entries_.cend(),
                            [&key](const value_type& e) noexcept { return e.first == key; });
    }

    /// Returns 1 if key is present, 0 otherwise. Matches std::map::count semantics.
    std::size_t count(const Key& key) const noexcept
    {
        return find(key) != cend() ? 1U : 0U;
    }

    std::size_t size()  const noexcept { return entries_.size(); }
    bool        empty() const noexcept { return entries_.empty(); }
    void        clear()       noexcept { entries_.clear(); }

    iterator       begin()        noexcept { return entries_.begin(); }
    iterator       end()          noexcept { return entries_.end(); }
    const_iterator begin()  const noexcept { return entries_.cbegin(); }
    const_iterator end()    const noexcept { return entries_.cend(); }
    const_iterator cbegin() const noexcept { return entries_.cbegin(); }
    const_iterator cend()   const noexcept { return entries_.cend(); }

    bool operator==(const InsertionOrderedMap& other) const noexcept
    {
        return entries_ == other.entries_;
    }

    bool operator!=(const InsertionOrderedMap& other) const noexcept
    {
        return entries_ != other.entries_;
    }

  private:
    std::vector<value_type> entries_;
};

/// String key/value map that preserves insertion order.
using KeyValueAttributes = InsertionOrderedMap<std::string, std::string>;

/************************************/
/* UDS Negative Response Code       */
/************************************/

namespace uds
{

/// cf. ISO 14229-1:2020, Table A.1
enum class NegativeResponseCode : std::uint8_t
{
    GeneralReject                                        = 0x10,
    ServiceNotSupported                                  = 0x11,
    SubFunctionNotSupported                              = 0x12,
    IncorrectMessageLengthOrInvalidFormat                = 0x13,
    ResponseTooLong                                      = 0x14,
    BusyRepeatRequest                                    = 0x21,
    ConditionsNotCorrect                                 = 0x22,
    NoResponseFromSubnetComponent                        = 0x23,
    RequestSequenceError                                 = 0x24,
    NoResponseFromSubNetComponent                        = 0x25,
    FailurePreventsExecutionOfRequestedAction            = 0x26,
    RequestOutOfRange                                    = 0x31,
    SecurityAccessDenied                                 = 0x33,
    AuthenticationRequired                               = 0x34,
    InvalidKey                                           = 0x35,
    ExceededNumberOfAttempts                             = 0x36,
    RequiredTimeDelayNotExpired                          = 0x37,
    SecureDataTransmissionRequired                       = 0x38,
    SecureDataTransmissionNotAllowed                     = 0x39,
    SecureDataVerificationFailed                         = 0x3A,
    CertificateVerificationFailedInvalidTimePeriod       = 0x50,
    CertificateVerificationFailedInvalidSignature        = 0x51,
    CertificateVerificationFailedInvalidChainOfTrust     = 0x52,
    CertificateVerificationFailedInvalidType             = 0x53,
    CertificateVerificationFailedInvalidFormat           = 0x54,
    CertificateVerificationFailedInvalidContent          = 0x55,
    CertificateVerificationFailedInvalidScope            = 0x56,
    CertificateVerificationFailedInvalidCertificate      = 0x57,
    OwnershipVerificationFailed                          = 0x58,
    ChallengeCalculationFailed                           = 0x59,
    SettingAccessRightsFailed                            = 0x5A,
    SessionKeyCreationOrDerivationFailed                 = 0x5B,
    ConfigurationDataUsageFailed                         = 0x5C,
    DeAuthenticationFailed                               = 0x5D,
    UploadDownloadNotAccepted                            = 0x70,
    TransferDataSuspended                                = 0x71,
    GeneralProgrammingFailure                            = 0x72,
    WrongBlockSequenceCounter                            = 0x73,
    RequestCorrectlyReceivedResponsePending              = 0x78,
    SubFunctionNotSupportedInActiveSession               = 0x7E,
    ServiceNotSupportedInActiveSession                   = 0x7F,
    RpmTooHigh                                           = 0x81,
    RpmTooLow                                            = 0x82,
    EngineIsRunning                                      = 0x83,
    EngineIsNotRunning                                   = 0x84,
    EngineRunTimeTooLow                                  = 0x85,
    TemperatureTooHigh                                   = 0x86,
    TemperatureTooLow                                    = 0x87,
    VehicleSpeedTooHigh                                  = 0x88,
    VehicleSpeedTooLow                                   = 0x89,
    ThrottleOrPedalTooHigh                               = 0x8A,
    ThrottleOrPedalTooLow                                = 0x8B,
    TransmissionRangeNotInNeutral                        = 0x8C,
    TransmissionRangeNotInGear                           = 0x8D,
    BrakeSwitchOrSwitchesNotClosed                       = 0x8F,
    ShifterLeverNotInPark                                = 0x90,
    TorqueConverterClutchLocked                          = 0x91,
    VoltageTooHigh                                       = 0x92,
    VoltageTooLow                                        = 0x93,
    ResourceTemporarilyNotAvailable                      = 0x94,
};

}  // namespace uds

/************************************/
/* Message payload types            */
/************************************/

/// Indicates whether a JSON schema is required as part of a reply message.
enum class JsonSchemaRequired : std::uint8_t
{
    Yes,
    No,
};

/// Representation of a request message payload for further processing.
struct RequestMessagePayload
{
    /// Payload variant tag
    enum class Kind : std::uint8_t
    {
        Binary,
        Json,
        Utf8,
    };

    Kind kind;
    ByteVector binary_data;  ///< valid when kind == Binary
    std::string text_data;   ///< valid when kind == Json or Utf8 (raw JSON string or UTF-8 string)

    static RequestMessagePayload from_bytes(ByteVector data) noexcept
    {
        return RequestMessagePayload{Kind::Binary, std::move(data), {}};
    }

    static RequestMessagePayload from_json(std::string json_str) noexcept
    {
        return RequestMessagePayload{Kind::Json, {}, std::move(json_str)};
    }

    static RequestMessagePayload from_string(std::string text) noexcept
    {
        return RequestMessagePayload{Kind::Utf8, {}, std::move(text)};
    }
};

/// Expected encoding of a reply message.
///
/// The JSON variant carries JsonSchemaRequired inline. Invalid states (e.g. Binary + schema
/// required) are impossible because construction is only possible via the factory methods.
class ReplyMessageEncoding
{
  public:
    enum class Kind : std::uint8_t { Binary, Json, Utf8 };

    /// Factory: Binary encoding.
    static ReplyMessageEncoding binary() noexcept
    {
        return ReplyMessageEncoding{Kind::Binary, JsonSchemaRequired::No};
    }

    /// Factory: JSON encoding carrying the schema-required flag.
    static ReplyMessageEncoding json(JsonSchemaRequired schema_required) noexcept
    {
        return ReplyMessageEncoding{Kind::Json, schema_required};
    }

    /// Factory: UTF-8 encoding.
    static ReplyMessageEncoding utf8() noexcept
    {
        return ReplyMessageEncoding{Kind::Utf8, JsonSchemaRequired::No};
    }

    Kind kind()      const noexcept { return kind_; }
    bool is_binary() const noexcept { return kind_ == Kind::Binary; }
    bool is_json()   const noexcept { return kind_ == Kind::Json; }
    bool is_utf8()   const noexcept { return kind_ == Kind::Utf8; }

    /// Returns the JsonSchemaRequired flag.
    /// Only meaningful when is_json() is true; returns No for Binary and UTF8.
    JsonSchemaRequired json_schema_required() const noexcept { return json_schema_required_; }

    bool operator==(const ReplyMessageEncoding& other) const noexcept
    {
        return kind_ == other.kind_ && json_schema_required_ == other.json_schema_required_;
    }

    bool operator!=(const ReplyMessageEncoding& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    ReplyMessageEncoding(Kind kind, JsonSchemaRequired schema_required) noexcept
        : kind_{kind}, json_schema_required_{schema_required}
    {}

    Kind               kind_{Kind::Binary};
    JsonSchemaRequired json_schema_required_{JsonSchemaRequired::No};
};

/// Representation of a reply message payload to be sent back to clients.
struct ReplyMessagePayload
{
    enum class Kind : std::uint8_t
    {
        Binary,
        Json,
        Utf8,
    };

    Kind kind;
    ByteVector binary_data;   ///< valid when kind == Binary
    std::string text_data;    ///< valid when kind == Json (raw JSON) or Utf8
    std::optional<std::string> json_schema;  ///< optional JSON schema string when kind == Json

    static ReplyMessagePayload from_byte_vector(ByteVector data) noexcept
    {
        return ReplyMessagePayload{Kind::Binary, std::move(data), {}, std::nullopt};
    }

    static ReplyMessagePayload from_json(std::string json_str,
                                         std::optional<std::string> schema = std::nullopt) noexcept
    {
        return ReplyMessagePayload{Kind::Json, {}, std::move(json_str), std::move(schema)};
    }

    static ReplyMessagePayload from_string(std::string text) noexcept
    {
        return ReplyMessagePayload{Kind::Utf8, {}, std::move(text), std::nullopt};
    }
};

/************************************/
/* DiagnosticReply                  */
/************************************/

/// Diagnostic reply type wrapping the reply message payload.
struct DiagnosticReply
{
    std::optional<ReplyMessagePayload> message_payload;
    std::optional<KeyValueAttributes>  additional_attrs;
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_RESULT_H
