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

#include "score/span.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
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
using ByteVector = std::vector<std::byte>;

/// Non-owning read-only view over a byte buffer (C++17 polyfill via score_baselibs).
/// Equivalent to std::span<const std::byte> (C++20).
using ByteView = score::cpp::span<const std::byte>;

/************************************/
/* Key-value attribute map          */
/************************************/

/// Insertion-order-preserving key/value map with O(1) average-case lookup.
///
/// Internally uses the same dual-structure layout as Python dicts (3.7+) and Rust's IndexMap:
///   entries_  — vector<pair<K,V>>        owns data and preserves insertion order
///   index_    — unordered_map<K,size_t>  maps key → position in entries_
///
/// Lookup, insert and update are O(1) average.  Iteration is in insertion order.
/// Note: erase is not provided; adding it would require O(n) index_ offset fixup.
template <typename Key, typename Value>
class InsertionOrderedMap
{
  public:
    using value_type     = std::pair<Key, Value>;
    using iterator       = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    /// Insert or update with value; Returns {iterator-to-element, true} on new insert,
    /// {iterator, false} on update (if key already exists the value is updated in-place, insertion order unchanged).
    std::pair<iterator, bool> emplace(Key key, Value value)
    {
        const auto idx_it = index_.find(key);
        if (idx_it != index_.end())
        {
            entries_[idx_it->second].second = std::move(value);
            return {entries_.begin() + static_cast<std::ptrdiff_t>(idx_it->second), false};
        }
        const std::size_t pos = entries_.size();
        index_.emplace(key, pos);  // copy key into index before moving into entries_
        entries_.emplace_back(std::move(key), std::move(value));
        return {entries_.end() - 1, true};
    }

    /// Access or default-insert by key (same semantics as std::map::operator[]).
    Value& operator[](const Key& key)
    {
        const auto idx_it = index_.find(key);
        if (idx_it != index_.end())
        {
            return entries_[idx_it->second].second;
        }
        const std::size_t pos = entries_.size();
        index_.emplace(key, pos);
        entries_.emplace_back(key, Value{});
        return entries_.back().second;
    }

    /// Read-only access by key. Precondition: key must exist (asserted).
    const Value& at(const Key& key) const noexcept
    {
        const auto idx_it = index_.find(key);
        assert(idx_it != index_.end());
        return entries_[idx_it->second].second;
    }

    /// Read-write access by key. Precondition: key must exist (asserted).
    Value& at(const Key& key) noexcept
    {
        const auto idx_it = index_.find(key);
        assert(idx_it != index_.end());
        return entries_[idx_it->second].second;
    }

    iterator find(const Key& key) noexcept
    {
        const auto idx_it = index_.find(key);
        if (idx_it == index_.end()) { return entries_.end(); }
        return entries_.begin() + static_cast<std::ptrdiff_t>(idx_it->second);
    }

    const_iterator find(const Key& key) const noexcept
    {
        const auto idx_it = index_.find(key);
        if (idx_it == index_.end()) { return entries_.cend(); }
        return entries_.cbegin() + static_cast<std::ptrdiff_t>(idx_it->second);
    }

    /// Returns 1 if key is present, 0 otherwise. Matches std::map::count semantics.
    std::size_t count(const Key& key) const noexcept
    {
        return index_.count(key);
    }

    std::size_t size()  const noexcept { return entries_.size(); }
    bool        empty() const noexcept { return entries_.empty(); }
    void        clear()       noexcept { entries_.clear(); index_.clear(); }

    iterator       begin()        noexcept { return entries_.begin(); }
    iterator       end()          noexcept { return entries_.end(); }
    const_iterator begin()  const noexcept { return entries_.cbegin(); }
    const_iterator end()    const noexcept { return entries_.cend(); }
    const_iterator cbegin() const noexcept { return entries_.cbegin(); }
    const_iterator cend()   const noexcept { return entries_.cend(); }

    /// Equality compares only entries_ (index_ is a derived structure).
    bool operator==(const InsertionOrderedMap& other) const noexcept
    {
        return entries_ == other.entries_;
    }

    bool operator!=(const InsertionOrderedMap& other) const noexcept
    {
        return entries_ != other.entries_;
    }

  private:
    std::vector<value_type>              entries_;  ///< insertion-ordered data store
    std::unordered_map<Key, std::size_t> index_;    ///< key → index into entries_
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
    NoProcessingNoResponse                               = 0xFF,
};

/// cf. ISO 14229-1:2020, Table A.1 (vehicleManufacturerSpecificConditionsNotCorrect)
/// Wraps an NRC byte in the manufacturer-specific range 0xF0–0xFE.
class VehicleManufacturerSpecificCNC
{
  public:
    /// Factory: construct from a validated byte value.
    /// Precondition: value must be in [0xF0, 0xFE].
    static VehicleManufacturerSpecificCNC from(std::uint8_t value) noexcept
    {
        assert(value >= 0xF0U && value <= 0xFEU && "VehicleManufacturerSpecificCNC out of range");
        return VehicleManufacturerSpecificCNC{value};
    }

    /// Returns the raw NRC byte value.
    std::uint8_t value() const noexcept { return value_; }

    bool operator==(const VehicleManufacturerSpecificCNC& other) const noexcept
    {
        return value_ == other.value_;
    }

    bool operator!=(const VehicleManufacturerSpecificCNC& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    explicit VehicleManufacturerSpecificCNC(std::uint8_t v) noexcept : value_{v} {}

    std::uint8_t value_{0xF0U};
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

/// Tag type for a JSON string payload.
/// Distinguishes JSON-encoded content from plain UTF-8 in RequestMessagePayload.
struct JsonPayload
{
    std::string value;
    bool operator==(const JsonPayload& other) const noexcept { return value == other.value; }
    bool operator!=(const JsonPayload& other) const noexcept { return !(*this == other); }
};

/// Request message payload — exactly one of raw bytes, a JSON string,
/// or a UTF-8 string. Construct only via the static factories below.
/// Pattern-match using the named alternative type aliases:
/// @code
///   std::get_if<RequestMessagePayload::Binary>(&p)  // ByteVector
///   std::get_if<RequestMessagePayload::Json  >(&p)  // JsonPayload (->value)
///   std::get_if<RequestMessagePayload::Utf8  >(&p)  // std::string
/// @endcode
/// @note Rust: enum RequestMessagePayload
///             { Binary(ByteVector), JSON(JsonValue), UTF8(String) }
class RequestMessagePayload : public std::variant<ByteVector, JsonPayload, std::string>
{
    using Base = std::variant<ByteVector, JsonPayload, std::string>;

  public:
    using Binary = ByteVector;   ///< Raw bytes.
    using Json   = JsonPayload;  ///< JSON string (access via `.value`).
    using Utf8   = std::string;  ///< UTF-8 text.

    using Base::Base;

    /// Factory: construct a Binary payload from raw bytes.
    static RequestMessagePayload from_bytes(ByteVector data) noexcept
    {
        return RequestMessagePayload{std::move(data)};
    }

    /// Factory: construct a Binary payload by copying a non-owning byte view.
    static RequestMessagePayload from_bytes(ByteView data) noexcept
    {
        return RequestMessagePayload{ByteVector{data.begin(), data.end()}};
    }

    /// Factory: construct a Json payload from a JSON string.
    static RequestMessagePayload from_json(std::string json_str) noexcept
    {
        return RequestMessagePayload{JsonPayload{std::move(json_str)}};
    }

    /// Factory: construct a Utf8 string payload.
    static RequestMessagePayload from_string(std::string text) noexcept
    {
        return RequestMessagePayload{std::move(text)};
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

    Kind kind{Kind::Binary};
    ByteVector binary_data;   ///< valid when kind == Binary
    std::string text_data;    ///< valid when kind == Json (raw JSON) or Utf8
    std::optional<std::string> json_schema;  ///< optional JSON schema string when kind == Json

    bool is_binary() const noexcept { return kind == Kind::Binary; }
    bool is_json()   const noexcept { return kind == Kind::Json; }
    bool is_utf8()   const noexcept { return kind == Kind::Utf8; }

    /// Factory: construct a Binary payload from an owning byte vector.
    static ReplyMessagePayload from_byte_vector(ByteVector data) noexcept
    {
        return ReplyMessagePayload{Kind::Binary, std::move(data), {}, std::nullopt};
    }

    /// Alias for from_byte_vector.
    static ReplyMessagePayload from_bytes(ByteVector data) noexcept
    {
        return from_byte_vector(std::move(data));
    }

    /// Factory: construct a Binary payload by copying a non-owning byte view.
    static ReplyMessagePayload from_bytes(ByteView data) noexcept
    {
        return from_byte_vector(ByteVector{data.begin(), data.end()});
    }

    /// Factory: construct a Json payload from a JSON string.
    static ReplyMessagePayload from_json(std::string json_str,
                                         std::optional<std::string> schema = std::nullopt) noexcept
    {
        return ReplyMessagePayload{Kind::Json, {}, std::move(json_str), std::move(schema)};
    }

    /// Factory: construct a Utf8 payload from a plain string.
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
