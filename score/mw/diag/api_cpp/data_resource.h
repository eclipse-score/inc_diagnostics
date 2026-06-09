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

/// @file data_resource.h
/// @brief SOVD data resource API: DataCategory, DataResourceMetadata, ReadValueArgs,
///        ReadValueReply, WriteValueArgs and the DataResource interface.

#ifndef SCORE_MW_DIAG_DATA_RESOURCE_H
#define SCORE_MW_DIAG_DATA_RESOURCE_H

#include "score/mw/diag/result.h"
#include "score/mw/diag/sovd_error.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace score
{
namespace mw
{
namespace diag
{
namespace sovd
{

/************************************/
/* DataCategory                     */
/************************************/

/// cf. ISO 17978-3:2025 Section 7.9.1, Table 70
class DataCategory
{
  public:
    enum class Kind : std::uint8_t { IdentData, CurrentData, StoredData, SysInfo, Custom };

    static const DataCategory IdentData;
    static const DataCategory CurrentData;
    static const DataCategory StoredData;
    static const DataCategory SysInfo;

    DataCategory() noexcept : kind_{Kind::IdentData} {}

    /// Factory: construct a custom category carrying a name.
    static DataCategory custom(std::string name) noexcept
    {
        return DataCategory{Kind::Custom, std::move(name)};
    }

    Kind kind() const noexcept { return kind_; }

    /// Returns true when this is a custom (non-standard) category.
    bool is_custom() const noexcept { return kind_ == Kind::Custom; }

    /// Returns the custom name. Empty string for standard categories.
    const std::string& custom_name() const noexcept { return custom_name_; }

    /// Returns the SOVD wire-format string.
    /// Standard categories return their wire name (e.g. "identData").
    /// Custom categories return the custom name string.
    std::string_view to_string() const noexcept
    {
        switch (kind_)
        {
            case Kind::IdentData:   return "identData";
            case Kind::CurrentData: return "currentData";
            case Kind::StoredData:  return "storedData";
            case Kind::SysInfo:     return "sysInfo";
            case Kind::Custom:      return custom_name_;
        }
        return "";
    }

    bool operator==(const DataCategory& other) const noexcept
    {
        return kind_ == other.kind_ && custom_name_ == other.custom_name_;
    }

    bool operator!=(const DataCategory& other) const noexcept
    {
        return !(*this == other);
    }

  private:
    explicit DataCategory(Kind kind) noexcept : kind_{kind} {}
    DataCategory(Kind kind, std::string name) noexcept
        : kind_{kind}, custom_name_{std::move(name)} {}

    Kind        kind_{Kind::IdentData};
    std::string custom_name_;
};

inline const DataCategory DataCategory::IdentData{DataCategory::Kind::IdentData};
inline const DataCategory DataCategory::CurrentData{DataCategory::Kind::CurrentData};
inline const DataCategory DataCategory::StoredData{DataCategory::Kind::StoredData};
inline const DataCategory DataCategory::SysInfo{DataCategory::Kind::SysInfo};

/// Returns the SOVD wire-format string for a DataCategory. Free-function alias for consistency.
inline std::string_view to_string(const DataCategory& category) noexcept
{
    return category.to_string();
}

/// Parses a SOVD wire-format string into a DataCategory.
/// Returns DataCategory::custom(s) for non-standard names, preserving the name.
inline DataCategory data_category_from_string(std::string_view s) noexcept
{
    if (s == "identData")    { return DataCategory::IdentData; }
    if (s == "currentData")  { return DataCategory::CurrentData; }
    if (s == "storedData")   { return DataCategory::StoredData; }
    if (s == "sysInfo")      { return DataCategory::SysInfo; }
    return DataCategory::custom(std::string{s});
}

/************************************/
/* DataCategoryInfo                 */
/************************************/

/// cf. ISO 17978-3:2025 Section 7.9.2.2, Table 73
struct DataCategoryInfo
{
    DataCategory               item;
    std::optional<std::string> category_translation_id;
};

/************************************/
/* DataResourceMetadata             */
/************************************/

/// cf. ISO 17978-3:2025 Section 7.9.3.1, Table 81
struct DataResourceMetadata
{
    std::string                     id;
    std::string                     name;
    std::optional<std::string>      translation_id;
    bool                            read_only{true};
    DataCategory                    category;
    std::optional<std::vector<std::string>> groups;
};

}  // namespace sovd

/************************************/
/* Read / Write value types         */
/************************************/

/// cf. ISO 17978-3:2025 Section 7.14.6, Table 83
struct ReadValueArgs
{
    ReplyMessageEncoding              reply_encoding{ReplyMessageEncoding::binary()};
    std::optional<KeyValueAttributes> additional_attrs;

    /// Factory: construct with a reply encoding.
    static ReadValueArgs with_encoding(ReplyMessageEncoding encoding) noexcept
    {
        return ReadValueArgs{encoding, std::nullopt};
    }

    /// Builder: attach additional attributes.
    ReadValueArgs with_additional_attrs(KeyValueAttributes attrs) && noexcept
    {
        additional_attrs = std::move(attrs);
        return std::move(*this);
    }
};

/// cf. ISO 17978-3:2025 Section 7.9.3.2, Table 85
struct ReadValueReply
{
    ReplyMessagePayload                    data;
    std::optional<std::vector<sovd::DataError>> errors;
};

/// cf. ISO 17978-3:2025 Section 7.9.5.1, Table 99
struct WriteValueArgs
{
    std::optional<RequestMessagePayload>  user_data;
    std::optional<std::string>            user_data_signature;
    std::optional<KeyValueAttributes>     additional_attrs;
};

/************************************/
/* DataResource interface           */
/************************************/

/// Interface for a single SOVD data resource provider.
///
/// Implementations may optionally also provide write access to a specific data value
/// within an Entity, following the SOVD data resource model (ISO 17978-3 Section 7.9).
///
/// @note The Rust API returns ReadValueHandle / WriteValueHandle which support both
///       synchronous (Ready) and asynchronous (Pending) paths. In C++, the async path
///       is modelled via std::function to keep the header free of executor dependencies.
///       Implementations that require async behaviour shall store the deferred work in a
///       callable and return it inside ReadValueReply / WriteResult.
class DataResource
{
  public:
    /// Read the current value of this data resource.
    /// i.e. GET /{entity-path}/data/{data-id}
    virtual Result<ReadValueReply> read(ReadValueArgs input) = 0;

    /// Write a new value to this data resource.
    /// i.e. PUT /{entity-path}/data/{data-id}
    ///
    /// The default implementation returns a PreconditionNotFulfilled error
    /// indicating that this data resource is read-only.
    virtual std::variant<std::monostate, sovd::DataError> write(WriteValueArgs input)
    {
        (void)input;
        return sovd::DataError::from_error(
            sovd::GenericError::from_code(
                sovd::ErrorCode::PreconditionNotFulfilled,
                "This data resource cannot be written to since it is a read-only one!"));
    }

    DataResource() = default;
    DataResource(const DataResource&) = delete;
    DataResource(DataResource&&) noexcept = delete;
    DataResource& operator=(const DataResource&) & = delete;
    DataResource& operator=(DataResource&&) & noexcept = delete;
    virtual ~DataResource() noexcept = default;
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_DATA_RESOURCE_H
