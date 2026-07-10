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

use common::sovd::{DataError, GenericError};
use common::Result as DiagResult;
use common::{
    KeyValueAttributes, ReplyMessageEncoding, ReplyMessagePayload, RequestMessagePayload,
};

use std::future::Future;
use std::pin::Pin;

/*******************/
/* General Types   */
/*******************/
pub mod sovd {
    /// cf. ISO 17978-3:2025 Section 7.9.1, Table 70
    #[derive(Clone, Debug, PartialEq)]
    pub enum DataCategory {
        IdentData,
        CurrentData,
        StoredData,
        SysInfo,
        Custom(String),
    }

    impl std::fmt::Display for DataCategory {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            match self {
                DataCategory::IdentData => write!(f, "identData"),
                DataCategory::CurrentData => write!(f, "currentData"),
                DataCategory::StoredData => write!(f, "storedData"),
                DataCategory::SysInfo => write!(f, "sysInfo"),
                DataCategory::Custom(name) => write!(f, "{}", name),
            }
        }
    }

    impl DataCategory {
        #[must_use]
        pub fn from_str(s: &str) -> Self {
            match s {
                "identData" => DataCategory::IdentData,
                "currentData" => DataCategory::CurrentData,
                "storedData" => DataCategory::StoredData,
                "sysInfo" => DataCategory::SysInfo,
                other => DataCategory::Custom(other.to_string()),
            }
        }
    }

    /// cf. ISO 17978-3:2025 Section 7.9.2.2, Table 73
    #[derive(Clone, Debug)]
    pub struct DataCategoryInfo {
        pub item: DataCategory,
        pub category_translation_id: Option<String>,
    }

    /*****************************/
    /* Data resource meta-data   */
    /*****************************/

    /// cf. ISO 17978-3:2025 Section 7.9.3.1, Table 81
    #[derive(Clone, Debug)]
    pub struct DataResourceMetadata {
        pub id: String,
        pub name: String,
        pub translation_id: Option<String>,
        pub read_only: bool,
        pub category: DataCategory,
        pub groups: Option<Vec<String>>,
    }
}

/*******************************/
/* Read / Write value types    */
/*******************************/

/// cf. ISO 17978-3:2025 Section 7.14.6, Table 83
#[derive(Debug)]
pub struct ReadValueArgs {
    pub reply_encoding: ReplyMessageEncoding,
    pub additional_attrs: Option<KeyValueAttributes>,
}

impl ReadValueArgs {
    #[must_use]
    pub fn new(encoding: ReplyMessageEncoding) -> Self {
        Self {
            reply_encoding: encoding,
            additional_attrs: None,
        }
    }

    #[must_use]
    pub fn with_additional_attrs(mut self, additional_attrs: KeyValueAttributes) -> Self {
        self.additional_attrs = Some(additional_attrs);
        self
    }
}

/// cf. ISO 17978-3:2025 Section 7.9.3.2, Table 85
#[derive(Debug)]
pub struct ReadValueReply {
    pub data: ReplyMessagePayload,
    pub errors: Option<Vec<DataError>>,
}

/// Internal design pattern supporting both synchronous and asynchronous read of data resources.
/// `ReadValueHandle` enables unified handling of immediate results via `Ready` or deferred
/// async read of a data resource via `Pending`.
pub enum ReadValueHandle {
    Ready(DiagResult<ReadValueReply>),
    Pending(Pin<Box<dyn Future<Output = DiagResult<ReadValueReply>> + Send>>),
}

impl ReadValueHandle {
    #[must_use]
    pub fn ready(reply: ReadValueReply) -> Self {
        Self::Ready(Ok(reply))
    }

    #[must_use]
    pub fn from_error(err: common::Error) -> Self {
        Self::Ready(Err(err))
    }

    #[must_use]
    pub fn from_future<Fut>(future: Fut) -> Self
    where
        Fut: Future<Output = DiagResult<ReadValueReply>> + Send + 'static,
    {
        Self::Pending(Box::pin(future))
    }

    #[must_use]
    pub fn from_closure<Func>(func: Func) -> Self
    where
        Func: FnOnce() -> DiagResult<ReadValueReply> + Send + 'static,
    {
        Self::Pending(Box::pin(async move { func() }))
    }
}

/// cf. ISO 17978-3:2025 Section 7.9.5.1, Table 99
#[derive(Debug, Default)]
pub struct WriteValueArgs {
    pub user_data: Option<RequestMessagePayload>,
    pub user_data_signature: Option<String>,
    pub additional_attrs: Option<KeyValueAttributes>,
}

/// Internal design pattern supporting both synchronous and asynchronous write of data resources.
/// `WriteValueHandle` enables unified handling of immediate results via `Ready` or deferred
/// async write of a data resource via `Pending`.
pub enum WriteValueHandle {
    Ready(Result<(), DataError>),
    Pending(Pin<Box<dyn Future<Output = Result<(), DataError>> + Send>>),
}

impl WriteValueHandle {
    #[must_use]
    pub fn ready() -> Self {
        Self::Ready(Ok(()))
    }

    #[must_use]
    pub fn from_error(err: DataError) -> Self {
        Self::Ready(Err(err))
    }

    #[must_use]
    pub fn from_future<Fut>(future: Fut) -> Self
    where
        Fut: Future<Output = Result<(), DataError>> + Send + 'static,
    {
        Self::Pending(Box::pin(future))
    }

    #[must_use]
    pub fn from_closure<Func>(func: Func) -> Self
    where
        Func: FnOnce() -> Result<(), DataError> + Send + 'static,
    {
        Self::Pending(Box::pin(async move { func() }))
    }
}

/*************************/
/* Data Resource API     */
/*************************/

/// Trait for a single data resource provider.
///
/// Implementations may optionally also provide write access to a specific data value
/// within an Entity, following the SOVD data resource model (ISO 17978-3 Section 7.9).
pub trait DataResource {
    /// Read the current value of this data resource.
    /// i.e. GET /{entity-path}/data/{data-id}
    fn read(&self, input: ReadValueArgs) -> ReadValueHandle;

    /// Write a new value to this data resource.
    /// i.e. PUT /{entity-path}/data/{data-id}
    /// The default implementation returns an error indicating that this data resource is read-only.
    fn write(&mut self, _input: WriteValueArgs) -> WriteValueHandle {
        WriteValueHandle::from_error(DataError::from_error(GenericError::from_code(
            common::sovd::ErrorCode::PreconditionNotFulfilled,
            "This data resource cannot be written to since it is a read-only one!".to_string(),
        )))
    }
}

/*******************/
/* Unit Tests      */
/*******************/

#[cfg(test)]
mod tests {
    use super::*;
    use sovd::DataCategory;

    // ── DataCategory Display ───────────────────────────────────────────

    #[test]
    fn data_category_display_ident_data() {
        assert_eq!(DataCategory::IdentData.to_string(), "identData");
    }

    #[test]
    fn data_category_display_current_data() {
        assert_eq!(DataCategory::CurrentData.to_string(), "currentData");
    }

    #[test]
    fn data_category_display_stored_data() {
        assert_eq!(DataCategory::StoredData.to_string(), "storedData");
    }

    #[test]
    fn data_category_display_sys_info() {
        assert_eq!(DataCategory::SysInfo.to_string(), "sysInfo");
    }

    #[test]
    fn data_category_display_custom() {
        assert_eq!(
            DataCategory::Custom("myCustom".to_string()).to_string(),
            "myCustom"
        );
    }

    // ── DataCategory from_str ──────────────────────────────────────────

    #[test]
    fn data_category_from_str_known_variants() {
        assert_eq!(DataCategory::from_str("identData"), DataCategory::IdentData);
        assert_eq!(
            DataCategory::from_str("currentData"),
            DataCategory::CurrentData
        );
        assert_eq!(
            DataCategory::from_str("storedData"),
            DataCategory::StoredData
        );
        assert_eq!(DataCategory::from_str("sysInfo"), DataCategory::SysInfo);
    }

    #[test]
    fn data_category_from_str_custom() {
        assert_eq!(
            DataCategory::from_str("something_else"),
            DataCategory::Custom("something_else".to_string())
        );
    }

    // ── DataResource default write (read-only) ────────────────────────

    struct ReadOnlyResource;

    impl DataResource for ReadOnlyResource {
        fn read(&self, _input: ReadValueArgs) -> ReadValueHandle {
            ReadValueHandle::ready(ReadValueReply {
                data: ReplyMessagePayload::UTF8("foo".to_string()),
                errors: None,
            })
        }
        // write() uses the default implementation → read-only error
    }

    #[test]
    fn default_write_returns_precondition_error() {
        let mut res = ReadOnlyResource;
        let handle = res.write(WriteValueArgs::default());
        match handle {
            WriteValueHandle::Ready(Err(err)) => {
                assert_eq!(
                    err.error.as_ref().unwrap().sovd_error,
                    "precondition-not-fulfilled"
                );
            }
            _ => panic!("expected Ready(Err(..))"),
        }
    }

    // ── DataResource read ──────────────────────────────────────────────

    #[test]
    fn data_resource_read_returns_reply() {
        let res = ReadOnlyResource;
        let input = ReadValueArgs::new(ReplyMessageEncoding::UTF8);
        match res.read(input) {
            ReadValueHandle::Ready(Ok(reply)) => {
                assert_eq!(reply.data, ReplyMessagePayload::UTF8("foo".to_string()));
                assert!(reply.errors.is_none());
            }
            _ => panic!("expected Ready(Ok(..))"),
        }
    }

    // ── DataResource with custom write ─────────────────────────────────

    struct WritableResource;

    impl DataResource for WritableResource {
        fn read(&self, _input: ReadValueArgs) -> ReadValueHandle {
            ReadValueHandle::ready(ReadValueReply {
                data: ReplyMessagePayload::from_json(serde_json::json!({"val": 42}), None),
                errors: None,
            })
        }

        fn write(&mut self, _input: WriteValueArgs) -> WriteValueHandle {
            WriteValueHandle::ready()
        }
    }

    #[test]
    fn writable_resource_write_succeeds() {
        let mut res = WritableResource;
        let handle = res.write(WriteValueArgs::default());
        match handle {
            WriteValueHandle::Ready(Ok(())) => {}
            _ => panic!("expected Ready(Ok(()))"),
        }
    }

    #[test]
    fn writable_resource_read_with_json_encoding() {
        let res = WritableResource;
        let input = ReadValueArgs::new(ReplyMessageEncoding::JSON(common::JsonSchemaRequired::No));
        match res.read(input) {
            ReadValueHandle::Ready(Ok(reply)) => {
                assert_eq!(
                    reply.data,
                    ReplyMessagePayload::from_json(serde_json::json!({"val": 42}), None)
                );
            }
            _ => panic!("expected Ready(Ok(..))"),
        }
    }

    // ── ReadValueArgs ──────────────────────────────────────────────────

    #[test]
    fn read_value_args_with_attrs() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("Accept".to_string(), "application/json".to_string());
        let args = ReadValueArgs::new(ReplyMessageEncoding::JSON(common::JsonSchemaRequired::Yes))
            .with_additional_attrs(attrs);
        assert!(args.additional_attrs.is_some());
        assert_eq!(
            args.additional_attrs.as_ref().unwrap().get("Accept"),
            Some(&"application/json".to_string())
        );
    }

    // ── ReadValueReply ─────────────────────────────────────────────────

    #[test]
    fn read_value_reply_with_errors() {
        let err = DataError::new("/data/x".to_string()).with_error(GenericError::from_code(
            common::sovd::ErrorCode::ErrorResponse,
            "bad".to_string(),
        ));
        let reply = ReadValueReply {
            data: ReplyMessagePayload::from_byte_vector(vec![0xFF]),
            errors: Some(vec![err]),
        };
        assert_eq!(reply.errors.as_ref().unwrap().len(), 1);
    }

    #[test]
    fn read_value_reply_debug() {
        let reply = ReadValueReply {
            data: ReplyMessagePayload::from_string("test".to_string()),
            errors: None,
        };
        let debug = format!("{:?}", reply);
        assert!(debug.contains("ReadValueReply"));
    }

    // ── WriteValueArgs ─────────────────────────────────────────────────

    #[test]
    fn write_value_args_default() {
        let args = WriteValueArgs::default();
        assert!(args.user_data_signature.is_none());
        assert!(args.user_data.is_none());
        assert!(args.additional_attrs.is_none());
    }

    #[test]
    fn write_value_args_with_all_fields() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("auth".to_string(), "token".to_string());
        let args = WriteValueArgs {
            user_data_signature: Some("sig-abc".to_string()),
            user_data: Some(RequestMessagePayload::JSON(serde_json::json!({"v": 1}))),
            additional_attrs: Some(attrs),
        };
        assert_eq!(args.user_data_signature.as_deref(), Some("sig-abc"));
        assert!(args.user_data.is_some());
        assert!(args.additional_attrs.is_some());
    }

    #[test]
    fn write_value_args_debug() {
        let args = WriteValueArgs::default();
        let debug = format!("{:?}", args);
        assert!(debug.contains("WriteValueArgs"));
    }

    // ── DataCategoryInfo ───────────────────────────────────────────────

    #[test]
    fn data_category_info_without_translation() {
        let info = sovd::DataCategoryInfo {
            item: DataCategory::IdentData,
            category_translation_id: None,
        };
        assert_eq!(info.item, DataCategory::IdentData);
        assert!(info.category_translation_id.is_none());
    }

    #[test]
    fn data_category_info_with_translation() {
        let info = sovd::DataCategoryInfo {
            item: DataCategory::Custom("mycat".to_string()),
            category_translation_id: Some("trans-1".to_string()),
        };
        assert_eq!(info.category_translation_id.as_deref(), Some("trans-1"));
    }

    #[test]
    fn data_category_info_clone() {
        let info = sovd::DataCategoryInfo {
            item: DataCategory::CurrentData,
            category_translation_id: Some("t".to_string()),
        };
        let cloned = info.clone();
        assert_eq!(cloned.item, DataCategory::CurrentData);
        assert_eq!(cloned.category_translation_id, info.category_translation_id);
    }

    #[test]
    fn data_category_info_debug() {
        let info = sovd::DataCategoryInfo {
            item: DataCategory::SysInfo,
            category_translation_id: None,
        };
        let debug = format!("{:?}", info);
        assert!(debug.contains("DataCategoryInfo"));
    }

    // ── DataResourceMetadata ───────────────────────────────────────────

    #[test]
    fn data_resource_metadata_minimal() {
        let meta = sovd::DataResourceMetadata {
            id: "dr-1".to_string(),
            name: "Battery Voltage".to_string(),
            translation_id: None,
            read_only: true,
            category: DataCategory::CurrentData,
            groups: None,
        };
        assert_eq!(meta.id, "dr-1");
        assert_eq!(meta.name, "Battery Voltage");
        assert!(meta.translation_id.is_none());
        assert_eq!(meta.category, DataCategory::CurrentData);
        assert!(meta.groups.is_none());
    }

    #[test]
    fn data_resource_metadata_with_all_fields() {
        let meta = sovd::DataResourceMetadata {
            id: "dr-2".to_string(),
            name: "ECU Serial".to_string(),
            translation_id: Some("dummy".to_string()),
            read_only: false,
            category: DataCategory::IdentData,
            groups: Some(vec!["group-a".to_string(), "group-b".to_string()]),
        };
        assert_eq!(meta.translation_id.as_deref(), Some("dummy"));
        assert_eq!(meta.groups.as_ref().unwrap().len(), 2);
    }

    #[test]
    fn data_resource_metadata_clone() {
        let meta = sovd::DataResourceMetadata {
            id: "dr-3".to_string(),
            name: "Name".to_string(),
            translation_id: None,
            read_only: true,
            category: DataCategory::StoredData,
            groups: Some(vec!["g1".to_string()]),
        };
        let cloned = meta.clone();
        assert_eq!(cloned.id, meta.id);
        assert_eq!(cloned.name, meta.name);
    }

    #[test]
    fn data_resource_metadata_debug() {
        let meta = sovd::DataResourceMetadata {
            id: "dr-4".to_string(),
            name: "Test".to_string(),
            translation_id: None,
            read_only: false,
            category: DataCategory::SysInfo,
            groups: None,
        };
        let debug = format!("{:?}", meta);
        assert!(debug.contains("DataResourceMetadata"));
    }

    // ── DataCategory clone and debug ───────────────────────────────────

    #[test]
    fn data_category_clone() {
        let cat = DataCategory::StoredData;
        let cloned = cat.clone();
        assert_eq!(cat, cloned);
    }

    #[test]
    fn data_category_debug() {
        let cat = DataCategory::IdentData;
        let debug = format!("{:?}", cat);
        assert!(debug.contains("IdentData"));
    }

    #[test]
    fn data_category_custom_clone() {
        let cat = DataCategory::Custom("x".to_string());
        let cloned = cat.clone();
        assert_eq!(cat, cloned);
    }
}
