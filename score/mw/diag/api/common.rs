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

pub type KeyValueAttributes = indexmap::IndexMap<String, String>;

pub mod sovd {
    use super::KeyValueAttributes;

    /// cf. ISO 17978-3:2025 Section 5.8.4, Table 18
    pub enum ErrorCode {
        ErrorResponse,
        IncompleteRequest,
        InsufficientAccessRights,
        InvalidResponseContent,
        InvalidSignature,
        LockBroken,
        NotResponding,
        PreconditionNotFulfilled,
        SovdServerFailure,
        SovdServerMisconfigured,
        UpdateAutomatedNotSupported,
        UpdateExecutionInProgress,
        UpdatePreparationInProgress,
        UpdateProcessInProgress,
        VendorSpecific,
    }

    impl std::fmt::Display for ErrorCode {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            write!(
                f,
                "{}",
                match self {
                    ErrorCode::ErrorResponse => "error-response",
                    ErrorCode::IncompleteRequest => "incomplete-request",
                    ErrorCode::InsufficientAccessRights => "insufficient-access-rights",
                    ErrorCode::InvalidResponseContent => "invalid-response-content",
                    ErrorCode::InvalidSignature => "invalid-signature",
                    ErrorCode::LockBroken => "lock-broken",
                    ErrorCode::NotResponding => "not-responding",
                    ErrorCode::PreconditionNotFulfilled => "precondition-not-fulfilled",
                    ErrorCode::SovdServerFailure => "sovd-server-failure",
                    ErrorCode::SovdServerMisconfigured => "sovd-server-misconfigured",
                    ErrorCode::UpdateAutomatedNotSupported => "update-automated-not-supported",
                    ErrorCode::UpdateExecutionInProgress => "update-execution-in-progress",
                    ErrorCode::UpdatePreparationInProgress => "update-preparation-in-progress",
                    ErrorCode::UpdateProcessInProgress => "update-process-in-progress",
                    ErrorCode::VendorSpecific => "vendor-specific",
                }
            )
        }
    }

    /// cf. ISO 17978-3:2025 Section 5.8.3, Table 16
    #[derive(Clone, Debug, PartialEq)]
    #[must_use]
    pub struct GenericError {
        pub sovd_error: String,
        pub message_text: String,
        pub vendor_error: Option<String>,
        pub translation_id: Option<String>,
        pub additional_attrs: Option<KeyValueAttributes>,
    }

    impl GenericError {
        pub fn from_code(code: ErrorCode, message: String) -> Self {
            Self {
                sovd_error: code.to_string(),
                message_text: message,
                vendor_error: None,
                translation_id: None,
                additional_attrs: None,
            }
        }

        pub fn from_vendor_error(error: String, message: String) -> Self {
            Self {
                sovd_error: ErrorCode::VendorSpecific.to_string(),
                message_text: message,
                vendor_error: Some(error),
                translation_id: None,
                additional_attrs: None,
            }
        }

        pub fn with_translation_id(mut self, translation_id: String) -> Self {
            self.translation_id = Some(translation_id);
            self
        }

        pub fn with_additional_attrs(mut self, attrs: KeyValueAttributes) -> Self {
            self.additional_attrs = Some(attrs);
            self
        }
    }

    pub type Error = GenericError;

    /// cf. ISO 17978-3:2025 Section 5.8.3, Table 17
    #[derive(Clone, Debug)]
    #[must_use]
    pub struct DataError {
        pub path: String,
        pub error: Option<GenericError>,
    }

    impl DataError {
        /// According to ISO 17978-3:2025 Section 5.8.3 Table 17, `path` shall contain
        /// a "JSON Pointer describing which element of the response is erroneous".
        pub fn new(path: String) -> Self {
            Self {
                path: path,
                error: None,
            }
        }

        /// Convenience factory method to create a `DataError` just from `GenericError`.
        pub fn from_error(error: GenericError) -> Self {
            Self {
                path: String::default(),
                error: Some(error),
            }
        }

        /// Instance method to enrich a newly created `DataError` with a `GenericError`.
        pub fn with_error(mut self, error: GenericError) -> Self {
            self.error = Some(error);
            self
        }
    }
}

pub mod uds {

    /// cf. ISO 14229-1:2020, Table A.1
    #[derive(Clone, Debug, PartialEq)]
    pub enum NegativeResponseCode {
        // TO BE ADDED
    }
}

pub type ByteSlice<'a> = &'a [u8];
pub type ByteVector = Vec<u8>;

pub type JsonValue = serde_json::Value;
pub type JsonSchema = serde_json::Value;

/// Indicates whether a JSON schema is required as part of a reply message.
#[derive(Clone, Debug, PartialEq)]
pub enum JsonSchemaRequired {
    Yes,
    No,
}

/// Representation of a request message payload for further processing.
#[derive(Clone, Debug, PartialEq)]
pub enum RequestMessagePayload {
    Binary(ByteVector),
    JSON(JsonValue),
    UTF8(String),
}

/// Expected encoding of a reply message, used to specify the desired response format.
#[derive(Clone, Debug, PartialEq)]
pub enum ReplyMessageEncoding {
    Binary,
    JSON(JsonSchemaRequired),
    UTF8,
}

/// Representation of a reply message payload to be sent back to clients.
#[derive(Clone, Debug, PartialEq)]
pub enum ReplyMessagePayload {
    Binary(ByteVector),
    JSON(JsonValue, Option<JsonSchema>),
    UTF8(String),
}

impl ReplyMessagePayload {
    #[must_use]
    pub fn from_byte_vector(payload: ByteVector) -> Self {
        Self::Binary(payload)
    }

    #[must_use]
    pub fn from_json(payload: JsonValue, schema: Option<JsonSchema>) -> Self {
        Self::JSON(payload, schema)
    }

    #[must_use]
    pub fn from_string(payload: String) -> Self {
        Self::UTF8(payload)
    }
}

/// Diagonstic reply type wrapping the reply message payload.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct DiagnosticReply {
    pub message_payload: Option<ReplyMessagePayload>,
    pub additional_attrs: Option<KeyValueAttributes>,
}

/// Diagnostic error code encompassing protocol-specific error variants.
#[derive(Clone, Debug, PartialEq)]
pub enum ErrorCode {
    SOVD(sovd::Error),
    UDS(uds::NegativeResponseCode),
}

/// Error type for any diagnostic action
#[derive(Clone, Debug, PartialEq)]
#[must_use]
pub struct Error {
    pub code: ErrorCode,
    pub payload: Option<ReplyMessagePayload>,
}

impl Error {
    pub fn from_error(error: sovd::Error) -> Self {
        Self {
            code: ErrorCode::SOVD(error),
            payload: None,
        }
    }

    pub fn from_nrc(nrc: uds::NegativeResponseCode) -> Self {
        Self {
            code: ErrorCode::UDS(nrc),
            payload: None,
        }
    }

    pub fn with_payload(mut self, payload: ReplyMessagePayload) -> Self {
        self.payload = Some(payload);
        self
    }

    pub fn mutex_poisoned() -> Self {
        Self::from_error(sovd::GenericError::from_code(
            sovd::ErrorCode::SovdServerFailure,
            "mutex acquisition failed unexpectedly".to_string(),
        ))
    }
}

/// Result type for any diagnostic action
pub type Result<T> = std::result::Result<T, Error>;
