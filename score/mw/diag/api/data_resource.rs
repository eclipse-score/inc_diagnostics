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
