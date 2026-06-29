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
    #[repr(u8)]
    pub enum NegativeResponseCode {
        GeneralReject = 0x10,
        ServiceNotSupported = 0x11,
        SubFunctionNotSupported = 0x12,
        IncorrectMessageLengthOrInvalidFormat = 0x13,
        ResponseTooLong = 0x14,
        BusyRepeatRequest = 0x21,
        ConditionsNotCorrect = 0x22,
        NoResponseFromSubnetComponent = 0x23,
        RequestSequenceError = 0x24,
        NoResponseFromSubNetComponent = 0x25,
        FailurePreventsExecutionOfRequestedAction = 0x26,
        RequestOutOfRange = 0x31,
        SecurityAccessDenied = 0x33,
        AuthenticationRequired = 0x34,
        InvalidKey = 0x35,
        ExceededNumberOfAttempts = 0x36,
        RequiredTimeDelayNotExpired = 0x37,
        SecureDataTransmissionRequired = 0x38,
        SecureDataTransmissionNotAllowed = 0x39,
        SecureDataVerificationFailed = 0x3A,
        CertificateVerificationFailedInvalidTimePeriod = 0x50,
        CertificateVerificationFailedInvalidSignature = 0x51,
        CertificateVerificationFailedInvalidChainOfTrust = 0x52,
        CertificateVerificationFailedInvalidType = 0x53,
        CertificateVerificationFailedInvalidFormat = 0x54,
        CertificateVerificationFailedInvalidContent = 0x55,
        CertificateVerificationFailedInvalidScope = 0x56,
        CertificateVerificationFailedInvalidCertificate = 0x57,
        OwnershipVerificationFailed = 0x58,
        ChallengeCalculationFailed = 0x59,
        SettingAccessRightsFailed = 0x5A,
        SessionKeyCreationOrDerivationFailed = 0x5B,
        ConfigurationDataUsageFailed = 0x5C,
        DeAuthenticationFailed = 0x5D,
        UploadDownloadNotAccepted = 0x70,
        TransferDataSuspended = 0x71,
        GeneralProgrammingFailure = 0x72,
        WrongBlockSequenceCounter = 0x73,
        RequestCorrectlyReceivedResponsePending = 0x78,
        SubFunctionNotSupportedInActiveSession = 0x7E,
        ServiceNotSupportedInActiveSession = 0x7F,
        RpmTooHigh = 0x81,
        RpmTooLow = 0x82,
        EngineIsRunning = 0x83,
        EngineIsNotRunning = 0x84,
        EngineRunTimeTooLow = 0x85,
        TemperatureTooHigh = 0x86,
        TemperatureTooLow = 0x87,
        VehicleSpeedTooHigh = 0x88,
        VehicleSpeedTooLow = 0x89,
        ThrottleOrPedalTooHigh = 0x8A,
        ThrottleOrPedalTooLow = 0x8B,
        TransmissionRangeNotInNeutral = 0x8C,
        TransmissionRangeNotInGear = 0x8D,
        BrakeSwitchOrSwitchesNotClosed = 0x8F,
        ShifterLeverNotInPark = 0x90,
        TorqueConverterClutchLocked = 0x91,
        VoltageTooHigh = 0x92,
        VoltageTooLow = 0x93,
        ResourceTemporarilyNotAvailable = 0x94,
        VehicleManufacturerSpecific(VehicleManufacturerSpecificCNC),
    }

    impl NegativeResponseCode {
        pub fn from(cnc: VehicleManufacturerSpecificCNC) -> Self {
            Self::VehicleManufacturerSpecific(cnc)
        }
    }

    impl From<NegativeResponseCode> for u8 {
        fn from(nrc: NegativeResponseCode) -> Self {
            match nrc {
                NegativeResponseCode::GeneralReject => 0x10,
                NegativeResponseCode::ServiceNotSupported => 0x11,
                NegativeResponseCode::SubFunctionNotSupported => 0x12,
                NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat => 0x13,
                NegativeResponseCode::ResponseTooLong => 0x14,
                NegativeResponseCode::BusyRepeatRequest => 0x21,
                NegativeResponseCode::ConditionsNotCorrect => 0x22,
                NegativeResponseCode::NoResponseFromSubnetComponent => 0x23,
                NegativeResponseCode::RequestSequenceError => 0x24,
                NegativeResponseCode::NoResponseFromSubNetComponent => 0x25,
                NegativeResponseCode::FailurePreventsExecutionOfRequestedAction => 0x26,
                NegativeResponseCode::RequestOutOfRange => 0x31,
                NegativeResponseCode::SecurityAccessDenied => 0x33,
                NegativeResponseCode::AuthenticationRequired => 0x34,
                NegativeResponseCode::InvalidKey => 0x35,
                NegativeResponseCode::ExceededNumberOfAttempts => 0x36,
                NegativeResponseCode::RequiredTimeDelayNotExpired => 0x37,
                NegativeResponseCode::SecureDataTransmissionRequired => 0x38,
                NegativeResponseCode::SecureDataTransmissionNotAllowed => 0x39,
                NegativeResponseCode::SecureDataVerificationFailed => 0x3A,
                NegativeResponseCode::CertificateVerificationFailedInvalidTimePeriod => 0x50,
                NegativeResponseCode::CertificateVerificationFailedInvalidSignature => 0x51,
                NegativeResponseCode::CertificateVerificationFailedInvalidChainOfTrust => 0x52,
                NegativeResponseCode::CertificateVerificationFailedInvalidType => 0x53,
                NegativeResponseCode::CertificateVerificationFailedInvalidFormat => 0x54,
                NegativeResponseCode::CertificateVerificationFailedInvalidContent => 0x55,
                NegativeResponseCode::CertificateVerificationFailedInvalidScope => 0x56,
                NegativeResponseCode::CertificateVerificationFailedInvalidCertificate => 0x57,
                NegativeResponseCode::OwnershipVerificationFailed => 0x58,
                NegativeResponseCode::ChallengeCalculationFailed => 0x59,
                NegativeResponseCode::SettingAccessRightsFailed => 0x5A,
                NegativeResponseCode::SessionKeyCreationOrDerivationFailed => 0x5B,
                NegativeResponseCode::ConfigurationDataUsageFailed => 0x5C,
                NegativeResponseCode::DeAuthenticationFailed => 0x5D,
                NegativeResponseCode::UploadDownloadNotAccepted => 0x70,
                NegativeResponseCode::TransferDataSuspended => 0x71,
                NegativeResponseCode::GeneralProgrammingFailure => 0x72,
                NegativeResponseCode::WrongBlockSequenceCounter => 0x73,
                NegativeResponseCode::RequestCorrectlyReceivedResponsePending => 0x78,
                NegativeResponseCode::SubFunctionNotSupportedInActiveSession => 0x7E,
                NegativeResponseCode::ServiceNotSupportedInActiveSession => 0x7F,
                NegativeResponseCode::RpmTooHigh => 0x81,
                NegativeResponseCode::RpmTooLow => 0x82,
                NegativeResponseCode::EngineIsRunning => 0x83,
                NegativeResponseCode::EngineIsNotRunning => 0x84,
                NegativeResponseCode::EngineRunTimeTooLow => 0x85,
                NegativeResponseCode::TemperatureTooHigh => 0x86,
                NegativeResponseCode::TemperatureTooLow => 0x87,
                NegativeResponseCode::VehicleSpeedTooHigh => 0x88,
                NegativeResponseCode::VehicleSpeedTooLow => 0x89,
                NegativeResponseCode::ThrottleOrPedalTooHigh => 0x8A,
                NegativeResponseCode::ThrottleOrPedalTooLow => 0x8B,
                NegativeResponseCode::TransmissionRangeNotInNeutral => 0x8C,
                NegativeResponseCode::TransmissionRangeNotInGear => 0x8D,
                NegativeResponseCode::BrakeSwitchOrSwitchesNotClosed => 0x8F,
                NegativeResponseCode::ShifterLeverNotInPark => 0x90,
                NegativeResponseCode::TorqueConverterClutchLocked => 0x91,
                NegativeResponseCode::VoltageTooHigh => 0x92,
                NegativeResponseCode::VoltageTooLow => 0x93,
                NegativeResponseCode::ResourceTemporarilyNotAvailable => 0x94,
                NegativeResponseCode::VehicleManufacturerSpecific(cnc) => cnc.into(),
            }
        }
    }

    /// cf. ISO 14229-1:2020, Table A.1 (vehicleManufacturerSpecificConditionsNotCorrect)
    /// Valid NRC range: 0xF0..0xFE
    #[derive(Clone, Debug, PartialEq)]
    pub struct VehicleManufacturerSpecificCNC(u8);

    impl From<u8> for VehicleManufacturerSpecificCNC {
        fn from(value: u8) -> Self {
            match value {
                0xF0..=0xFE => Self(value),
                _ => panic!("Provided value for uds::VehicleManufacturerSpecificCNC is out of permitted range 0xF0..0xFE: {:#04X}", value),
            }
        }
    }

    impl From<VehicleManufacturerSpecificCNC> for u8 {
        fn from(cnc: VehicleManufacturerSpecificCNC) -> Self {
            cnc.0
        }
    }
}

pub type ByteSlice<'a> = &'a [u8];
pub type ByteVector = Vec<u8>;

pub type JsonValue = serde_json::Value;
pub type JsonSchema = serde_json::Value;

/// Representation of a request message payload for further processing.
#[derive(Clone, Debug, PartialEq)]
pub enum RequestMessagePayload {
    Binary(ByteVector),
    JSON(JsonValue),
    UTF8(String),
}

/// Indicates whether a JSON schema is required as part of a reply message.
#[derive(Clone, Debug, PartialEq)]
pub enum JsonSchemaRequired {
    Yes,
    No,
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

/*******************/
/* Unit Tests      */
/*******************/

#[cfg(test)]
mod tests {
    use super::*;

    // ── sovd::ErrorCode Display ────────────────────────────────────────

    #[test]
    fn error_code_display_error_response() {
        assert_eq!(sovd::ErrorCode::ErrorResponse.to_string(), "error-response");
    }

    #[test]
    fn error_code_display_incomplete_request() {
        assert_eq!(
            sovd::ErrorCode::IncompleteRequest.to_string(),
            "incomplete-request"
        );
    }

    #[test]
    fn error_code_display_insufficient_access_rights() {
        assert_eq!(
            sovd::ErrorCode::InsufficientAccessRights.to_string(),
            "insufficient-access-rights"
        );
    }

    #[test]
    fn error_code_display_invalid_response_content() {
        assert_eq!(
            sovd::ErrorCode::InvalidResponseContent.to_string(),
            "invalid-response-content"
        );
    }

    #[test]
    fn error_code_display_invalid_signature() {
        assert_eq!(
            sovd::ErrorCode::InvalidSignature.to_string(),
            "invalid-signature"
        );
    }

    #[test]
    fn error_code_display_lock_broken() {
        assert_eq!(sovd::ErrorCode::LockBroken.to_string(), "lock-broken");
    }

    #[test]
    fn error_code_display_not_responding() {
        assert_eq!(sovd::ErrorCode::NotResponding.to_string(), "not-responding");
    }

    #[test]
    fn error_code_display_precondition_not_fulfilled() {
        assert_eq!(
            sovd::ErrorCode::PreconditionNotFulfilled.to_string(),
            "precondition-not-fulfilled"
        );
    }

    #[test]
    fn error_code_display_sovd_server_failure() {
        assert_eq!(
            sovd::ErrorCode::SovdServerFailure.to_string(),
            "sovd-server-failure"
        );
    }

    #[test]
    fn error_code_display_sovd_server_misconfigured() {
        assert_eq!(
            sovd::ErrorCode::SovdServerMisconfigured.to_string(),
            "sovd-server-misconfigured"
        );
    }

    #[test]
    fn error_code_display_update_automated_not_supported() {
        assert_eq!(
            sovd::ErrorCode::UpdateAutomatedNotSupported.to_string(),
            "update-automated-not-supported"
        );
    }

    #[test]
    fn error_code_display_update_execution_in_progress() {
        assert_eq!(
            sovd::ErrorCode::UpdateExecutionInProgress.to_string(),
            "update-execution-in-progress"
        );
    }

    #[test]
    fn error_code_display_update_preparation_in_progress() {
        assert_eq!(
            sovd::ErrorCode::UpdatePreparationInProgress.to_string(),
            "update-preparation-in-progress"
        );
    }

    #[test]
    fn error_code_display_update_process_in_progress() {
        assert_eq!(
            sovd::ErrorCode::UpdateProcessInProgress.to_string(),
            "update-process-in-progress"
        );
    }

    #[test]
    fn error_code_display_vendor_specific() {
        assert_eq!(
            sovd::ErrorCode::VendorSpecific.to_string(),
            "vendor-specific"
        );
    }

    // ── sovd::GenericError constructors ────────────────────────────────

    #[test]
    fn generic_error_from_code() {
        let err = sovd::GenericError::from_code(
            sovd::ErrorCode::ErrorResponse,
            "test message".to_string(),
        );
        assert_eq!(err.sovd_error, "error-response");
        assert_eq!(err.message_text, "test message");
        assert!(err.vendor_error.is_none());
        assert!(err.translation_id.is_none());
        assert!(err.additional_attrs.is_none());
    }

    #[test]
    fn generic_error_with_translation_id() {
        let err =
            sovd::GenericError::from_code(sovd::ErrorCode::NotResponding, "timeout".to_string())
                .with_translation_id("tid_123".to_string());
        assert_eq!(err.translation_id.as_deref(), Some("tid_123"));
    }

    #[test]
    fn generic_error_from_code_with_translation() {
        let err =
            sovd::GenericError::from_code(sovd::ErrorCode::IncompleteRequest, "msg".to_string())
                .with_translation_id("trans_id".to_string());
        assert_eq!(err.sovd_error, "incomplete-request");
        assert_eq!(err.message_text, "msg");
        assert!(err.vendor_error.is_none());
        assert_eq!(err.translation_id.as_deref(), Some("trans_id"));
        assert!(err.additional_attrs.is_none());
    }

    #[test]
    fn generic_error_from_vendor_error() {
        let err =
            sovd::GenericError::from_vendor_error("custom-err".to_string(), "msg".to_string());
        assert_eq!(err.sovd_error, "vendor-specific");
        assert_eq!(err.message_text, "msg");
        assert_eq!(err.vendor_error.as_deref(), Some("custom-err"));
        assert!(err.translation_id.is_none());
        assert!(err.additional_attrs.is_none());
    }

    #[test]
    fn generic_error_from_vendor_error_with_translation() {
        let err =
            sovd::GenericError::from_vendor_error("custom-err".to_string(), "msg".to_string())
                .with_translation_id("tid".to_string());
        assert_eq!(err.sovd_error, "vendor-specific");
        assert_eq!(err.vendor_error.as_deref(), Some("custom-err"));
        assert_eq!(err.translation_id.as_deref(), Some("tid"));
    }

    // ── sovd::GenericError additional attrs ────────────────────────────

    #[test]
    fn generic_error_with_additional_attrs() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("foo".to_string(), "bar".to_string());
        let err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string())
            .with_additional_attrs(attrs);
        let result_attrs = err.additional_attrs.as_ref().unwrap();
        assert_eq!(result_attrs.get("foo"), Some(&"bar".to_string()));
    }

    #[test]
    fn generic_error_with_additional_attrs_single_entry() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("key1".to_string(), "val1".to_string());
        let err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string())
            .with_additional_attrs(attrs);
        let result_attrs = err.additional_attrs.as_ref().unwrap();
        assert_eq!(result_attrs.get("key1"), Some(&"val1".to_string()));
    }

    #[test]
    fn generic_error_with_additional_attrs_multiple_entries() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("k1".to_string(), "v1".to_string());
        attrs.insert("k2".to_string(), "v2".to_string());
        let err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string())
            .with_additional_attrs(attrs);
        let result_attrs = err.additional_attrs.as_ref().unwrap();
        assert_eq!(result_attrs.len(), 2);
        assert_eq!(result_attrs.get("k1"), Some(&"v1".to_string()));
        assert_eq!(result_attrs.get("k2"), Some(&"v2".to_string()));
    }

    #[test]
    fn generic_error_with_additional_attrs_replaces() {
        let mut old_attrs = KeyValueAttributes::new();
        old_attrs.insert("old".to_string(), "val".to_string());
        let mut new_attrs = KeyValueAttributes::new();
        new_attrs.insert("new_key".to_string(), "new_val".to_string());
        let err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string())
            .with_additional_attrs(old_attrs)
            .with_additional_attrs(new_attrs);
        let attrs = err.additional_attrs.as_ref().unwrap();
        assert_eq!(attrs.len(), 1);
        assert_eq!(attrs.get("new_key"), Some(&"new_val".to_string()));
        assert!(attrs.get("old").is_none());
    }

    // ── sovd::GenericError clone ──────────────────────────────────────

    #[test]
    fn generic_error_clone() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("k".to_string(), "v".to_string());
        let err = sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string())
            .with_additional_attrs(attrs);
        let cloned = err.clone();
        assert_eq!(cloned.sovd_error, err.sovd_error);
        assert_eq!(cloned.message_text, err.message_text);
        assert_eq!(cloned.additional_attrs, err.additional_attrs);
    }

    // ── sovd::DataError ───────────────────────────────────────────────

    #[test]
    fn data_error_new_with_path_and_error() {
        let generic_error =
            sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string());
        let data_err = sovd::DataError::new("/entity/data/1".to_string()).with_error(generic_error);
        assert_eq!(data_err.path, "/entity/data/1".to_string());
        assert!(data_err.error.is_some());
        assert_eq!(
            data_err.error.as_ref().unwrap().sovd_error,
            "error-response"
        );
    }

    #[test]
    fn data_error_with_lock_broken() {
        let generic_error =
            sovd::GenericError::from_code(sovd::ErrorCode::LockBroken, "locked".to_string());
        let data_err =
            sovd::DataError::new("/entity/data/123".to_string()).with_error(generic_error);
        assert_eq!(data_err.path, "/entity/data/123".to_string());
        assert!(data_err.error.is_some());
        assert_eq!(data_err.error.as_ref().unwrap().sovd_error, "lock-broken");
        assert_eq!(data_err.error.as_ref().unwrap().message_text, "locked");
    }

    #[test]
    fn data_error_clone() {
        let generic_error =
            sovd::GenericError::from_code(sovd::ErrorCode::LockBroken, "locked".to_string());
        let data_err =
            sovd::DataError::new("/entity/data/123".to_string()).with_error(generic_error);
        let cloned = data_err.clone();
        assert_eq!(cloned.path, data_err.path);
        assert_eq!(cloned.error, data_err.error);
    }

    #[test]
    fn data_error_with_error_builder() {
        let initial_error =
            sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "initial".to_string());
        let data_err = sovd::DataError::new("/path".to_string()).with_error(initial_error);
        let new_error = sovd::GenericError::from_code(
            sovd::ErrorCode::PreconditionNotFulfilled,
            "updated".to_string(),
        );
        let updated = data_err.with_error(new_error);
        assert_eq!(updated.path, "/path".to_string());
        assert_eq!(
            updated.error.as_ref().unwrap().sovd_error,
            "precondition-not-fulfilled"
        );
        assert_eq!(updated.error.as_ref().unwrap().message_text, "updated");
    }

    #[test]
    fn data_error_path_only() {
        let data_err: sovd::DataError = sovd::DataError {
            path: "/only/path".to_string(),
            error: None,
        };
        assert_eq!(data_err.path, "/only/path".to_string());
        assert!(data_err.error.is_none());
    }

    #[test]
    fn data_error_error_only() {
        let error = sovd::GenericError::from_code(
            sovd::ErrorCode::InvalidSignature,
            "signature invalid".to_string(),
        );
        let data_err = sovd::DataError::from_error(error.clone());
        assert_eq!(data_err.path, String::default());
        assert_eq!(
            data_err.error.as_ref().unwrap().sovd_error,
            "invalid-signature"
        );
    }

    // ── uds::VehicleManufacturerSpecificCNC ───────────────────────────

    #[test]
    fn vehicle_manufacturer_specific_cnc_from_u8_lower_bound() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xF0);
        assert_eq!(u8::from(cnc), 0xF0);
    }

    #[test]
    fn vehicle_manufacturer_specific_cnc_from_u8_upper_bound() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xFE);
        assert_eq!(u8::from(cnc), 0xFE);
    }

    #[test]
    fn vehicle_manufacturer_specific_cnc_from_u8_mid_range() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xF5);
        assert_eq!(u8::from(cnc), 0xF5);
    }

    #[test]
    #[should_panic(expected = "out of permitted range")]
    fn vehicle_manufacturer_specific_cnc_from_u8_below_range() {
        let _ = uds::VehicleManufacturerSpecificCNC::from(0xEF);
    }

    #[test]
    #[should_panic(expected = "out of permitted range")]
    fn vehicle_manufacturer_specific_cnc_from_u8_above_range() {
        let _ = uds::VehicleManufacturerSpecificCNC::from(0xFF);
    }

    #[test]
    #[should_panic(expected = "out of permitted range")]
    fn vehicle_manufacturer_specific_cnc_from_u8_zero() {
        let _ = uds::VehicleManufacturerSpecificCNC::from(0x00);
    }

    #[test]
    fn vehicle_manufacturer_specific_cnc_clone() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xF3);
        let cloned = cnc.clone();
        assert_eq!(cnc, cloned);
    }

    #[test]
    fn vehicle_manufacturer_specific_cnc_roundtrip() {
        for val in 0xF0..=0xFE {
            let cnc = uds::VehicleManufacturerSpecificCNC::from(val);
            assert_eq!(u8::from(cnc), val);
        }
    }

    // ── uds::NegativeResponseCode::from (VehicleManufacturerSpecific) ─

    #[test]
    fn negative_response_code_from_vehicle_manufacturer_specific() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xF0);
        let nrc = uds::NegativeResponseCode::from(cnc.clone());
        assert_eq!(
            nrc,
            uds::NegativeResponseCode::VehicleManufacturerSpecific(cnc)
        );
    }

    #[test]
    fn error_from_nrc_vehicle_manufacturer_specific() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xF2);
        let nrc = uds::NegativeResponseCode::from(cnc);
        let err = Error::from_nrc(nrc);
        match &err.code {
            ErrorCode::UDS(uds::NegativeResponseCode::VehicleManufacturerSpecific(inner)) => {
                assert_eq!(u8::from(inner.clone()), 0xF2);
            }
            _ => {
                panic!("expected UDS VehicleManufacturerSpecific error code");
            }
        }
        assert!(err.payload.is_none());
    }

    // ── uds::NegativeResponseCode → u8 conversion ──────────────────────

    #[test]
    fn nrc_to_u8_general_reject() {
        assert_eq!(u8::from(uds::NegativeResponseCode::GeneralReject), 0x10);
    }

    #[test]
    fn nrc_to_u8_service_not_supported() {
        assert_eq!(u8::from(uds::NegativeResponseCode::ServiceNotSupported), 0x11);
    }

    #[test]
    fn nrc_to_u8_sub_function_not_supported() {
        assert_eq!(u8::from(uds::NegativeResponseCode::SubFunctionNotSupported), 0x12);
    }

    #[test]
    fn nrc_to_u8_incorrect_message_length_or_invalid_format() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat),
            0x13
        );
    }

    #[test]
    fn nrc_to_u8_response_too_long() {
        assert_eq!(u8::from(uds::NegativeResponseCode::ResponseTooLong), 0x14);
    }

    #[test]
    fn nrc_to_u8_busy_repeat_request() {
        assert_eq!(u8::from(uds::NegativeResponseCode::BusyRepeatRequest), 0x21);
    }

    #[test]
    fn nrc_to_u8_conditions_not_correct() {
        assert_eq!(u8::from(uds::NegativeResponseCode::ConditionsNotCorrect), 0x22);
    }

    #[test]
    fn nrc_to_u8_no_response_from_subnet_component() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::NoResponseFromSubnetComponent),
            0x23
        );
    }

    #[test]
    fn nrc_to_u8_request_sequence_error() {
        assert_eq!(u8::from(uds::NegativeResponseCode::RequestSequenceError), 0x24);
    }

    #[test]
    fn nrc_to_u8_no_response_from_sub_net_component() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::NoResponseFromSubNetComponent),
            0x25
        );
    }

    #[test]
    fn nrc_to_u8_failure_prevents_execution() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::FailurePreventsExecutionOfRequestedAction),
            0x26
        );
    }

    #[test]
    fn nrc_to_u8_request_out_of_range() {
        assert_eq!(u8::from(uds::NegativeResponseCode::RequestOutOfRange), 0x31);
    }

    #[test]
    fn nrc_to_u8_security_access_denied() {
        assert_eq!(u8::from(uds::NegativeResponseCode::SecurityAccessDenied), 0x33);
    }

    #[test]
    fn nrc_to_u8_authentication_required() {
        assert_eq!(u8::from(uds::NegativeResponseCode::AuthenticationRequired), 0x34);
    }

    #[test]
    fn nrc_to_u8_invalid_key() {
        assert_eq!(u8::from(uds::NegativeResponseCode::InvalidKey), 0x35);
    }

    #[test]
    fn nrc_to_u8_exceeded_number_of_attempts() {
        assert_eq!(u8::from(uds::NegativeResponseCode::ExceededNumberOfAttempts), 0x36);
    }

    #[test]
    fn nrc_to_u8_required_time_delay_not_expired() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::RequiredTimeDelayNotExpired),
            0x37
        );
    }

    #[test]
    fn nrc_to_u8_secure_data_transmission_required() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::SecureDataTransmissionRequired),
            0x38
        );
    }

    #[test]
    fn nrc_to_u8_secure_data_transmission_not_allowed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::SecureDataTransmissionNotAllowed),
            0x39
        );
    }

    #[test]
    fn nrc_to_u8_secure_data_verification_failed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::SecureDataVerificationFailed),
            0x3A
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_time_period() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidTimePeriod),
            0x50
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_signature() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidSignature),
            0x51
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_chain_of_trust() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidChainOfTrust),
            0x52
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_type() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidType),
            0x53
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_format() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidFormat),
            0x54
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_content() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidContent),
            0x55
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_scope() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidScope),
            0x56
        );
    }

    #[test]
    fn nrc_to_u8_cert_verification_failed_invalid_certificate() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::CertificateVerificationFailedInvalidCertificate),
            0x57
        );
    }

    #[test]
    fn nrc_to_u8_ownership_verification_failed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::OwnershipVerificationFailed),
            0x58
        );
    }

    #[test]
    fn nrc_to_u8_challenge_calculation_failed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::ChallengeCalculationFailed),
            0x59
        );
    }

    #[test]
    fn nrc_to_u8_setting_access_rights_failed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::SettingAccessRightsFailed),
            0x5A
        );
    }

    #[test]
    fn nrc_to_u8_session_key_creation_or_derivation_failed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::SessionKeyCreationOrDerivationFailed),
            0x5B
        );
    }

    #[test]
    fn nrc_to_u8_configuration_data_usage_failed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::ConfigurationDataUsageFailed),
            0x5C
        );
    }

    #[test]
    fn nrc_to_u8_de_authentication_failed() {
        assert_eq!(u8::from(uds::NegativeResponseCode::DeAuthenticationFailed), 0x5D);
    }

    #[test]
    fn nrc_to_u8_upload_download_not_accepted() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::UploadDownloadNotAccepted),
            0x70
        );
    }

    #[test]
    fn nrc_to_u8_transfer_data_suspended() {
        assert_eq!(u8::from(uds::NegativeResponseCode::TransferDataSuspended), 0x71);
    }

    #[test]
    fn nrc_to_u8_general_programming_failure() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::GeneralProgrammingFailure),
            0x72
        );
    }

    #[test]
    fn nrc_to_u8_wrong_block_sequence_counter() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::WrongBlockSequenceCounter),
            0x73
        );
    }

    #[test]
    fn nrc_to_u8_request_correctly_received_response_pending() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::RequestCorrectlyReceivedResponsePending),
            0x78
        );
    }

    #[test]
    fn nrc_to_u8_sub_function_not_supported_in_active_session() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::SubFunctionNotSupportedInActiveSession),
            0x7E
        );
    }

    #[test]
    fn nrc_to_u8_service_not_supported_in_active_session() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::ServiceNotSupportedInActiveSession),
            0x7F
        );
    }

    #[test]
    fn nrc_to_u8_rpm_too_high() {
        assert_eq!(u8::from(uds::NegativeResponseCode::RpmTooHigh), 0x81);
    }

    #[test]
    fn nrc_to_u8_rpm_too_low() {
        assert_eq!(u8::from(uds::NegativeResponseCode::RpmTooLow), 0x82);
    }

    #[test]
    fn nrc_to_u8_engine_is_running() {
        assert_eq!(u8::from(uds::NegativeResponseCode::EngineIsRunning), 0x83);
    }

    #[test]
    fn nrc_to_u8_engine_is_not_running() {
        assert_eq!(u8::from(uds::NegativeResponseCode::EngineIsNotRunning), 0x84);
    }

    #[test]
    fn nrc_to_u8_engine_run_time_too_low() {
        assert_eq!(u8::from(uds::NegativeResponseCode::EngineRunTimeTooLow), 0x85);
    }

    #[test]
    fn nrc_to_u8_temperature_too_high() {
        assert_eq!(u8::from(uds::NegativeResponseCode::TemperatureTooHigh), 0x86);
    }

    #[test]
    fn nrc_to_u8_temperature_too_low() {
        assert_eq!(u8::from(uds::NegativeResponseCode::TemperatureTooLow), 0x87);
    }

    #[test]
    fn nrc_to_u8_vehicle_speed_too_high() {
        assert_eq!(u8::from(uds::NegativeResponseCode::VehicleSpeedTooHigh), 0x88);
    }

    #[test]
    fn nrc_to_u8_vehicle_speed_too_low() {
        assert_eq!(u8::from(uds::NegativeResponseCode::VehicleSpeedTooLow), 0x89);
    }

    #[test]
    fn nrc_to_u8_throttle_or_pedal_too_high() {
        assert_eq!(u8::from(uds::NegativeResponseCode::ThrottleOrPedalTooHigh), 0x8A);
    }

    #[test]
    fn nrc_to_u8_throttle_or_pedal_too_low() {
        assert_eq!(u8::from(uds::NegativeResponseCode::ThrottleOrPedalTooLow), 0x8B);
    }

    #[test]
    fn nrc_to_u8_transmission_range_not_in_neutral() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::TransmissionRangeNotInNeutral),
            0x8C
        );
    }

    #[test]
    fn nrc_to_u8_transmission_range_not_in_gear() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::TransmissionRangeNotInGear),
            0x8D
        );
    }

    #[test]
    fn nrc_to_u8_brake_switch_or_switches_not_closed() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::BrakeSwitchOrSwitchesNotClosed),
            0x8F
        );
    }

    #[test]
    fn nrc_to_u8_shifter_lever_not_in_park() {
        assert_eq!(u8::from(uds::NegativeResponseCode::ShifterLeverNotInPark), 0x90);
    }

    #[test]
    fn nrc_to_u8_torque_converter_clutch_locked() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::TorqueConverterClutchLocked),
            0x91
        );
    }

    #[test]
    fn nrc_to_u8_voltage_too_high() {
        assert_eq!(u8::from(uds::NegativeResponseCode::VoltageTooHigh), 0x92);
    }

    #[test]
    fn nrc_to_u8_voltage_too_low() {
        assert_eq!(u8::from(uds::NegativeResponseCode::VoltageTooLow), 0x93);
    }

    #[test]
    fn nrc_to_u8_resource_temporarily_not_available() {
        assert_eq!(
            u8::from(uds::NegativeResponseCode::ResourceTemporarilyNotAvailable),
            0x94
        );
    }

    #[test]
    fn nrc_to_u8_vehicle_manufacturer_specific_lower_bound() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xF0);
        let nrc = uds::NegativeResponseCode::VehicleManufacturerSpecific(cnc);
        assert_eq!(u8::from(nrc), 0xF0);
    }

    #[test]
    fn nrc_to_u8_vehicle_manufacturer_specific_upper_bound() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xFE);
        let nrc = uds::NegativeResponseCode::VehicleManufacturerSpecific(cnc);
        assert_eq!(u8::from(nrc), 0xFE);
    }

    #[test]
    fn nrc_to_u8_vehicle_manufacturer_specific_mid_range() {
        let cnc = uds::VehicleManufacturerSpecificCNC::from(0xF7);
        let nrc = uds::NegativeResponseCode::VehicleManufacturerSpecific(cnc);
        assert_eq!(u8::from(nrc), 0xF7);
    }

    #[test]
    fn nrc_to_u8_vehicle_manufacturer_specific_roundtrip_all() {
        for val in 0xF0..=0xFE {
            let cnc = uds::VehicleManufacturerSpecificCNC::from(val);
            let nrc = uds::NegativeResponseCode::VehicleManufacturerSpecific(cnc);
            assert_eq!(u8::from(nrc), val);
        }
    }

    // ── RequestMessagePayload ─────────────────────────────────────────

    #[test]
    fn request_message_payload_binary() {
        let payload = RequestMessagePayload::Binary(vec![0x01, 0x02, 0x03]);
        if let RequestMessagePayload::Binary(data) = &payload {
            assert_eq!(data, &[0x01, 0x02, 0x03]);
        } else {
            panic!("expected Binary variant");
        }
    }

    #[test]
    fn request_message_payload_json() {
        let json_val = serde_json::json!({"key": "value"});
        let payload = RequestMessagePayload::JSON(json_val.clone());
        assert_eq!(payload, RequestMessagePayload::JSON(json_val));
    }

    #[test]
    fn request_message_payload_utf8() {
        let payload = RequestMessagePayload::UTF8("hello".to_string());
        assert_eq!(payload, RequestMessagePayload::UTF8("hello".to_string()));
    }

    #[test]
    fn request_message_payload_clone() {
        let original = RequestMessagePayload::Binary(vec![42]);
        let cloned = original.clone();
        assert_eq!(original, cloned);
    }

    #[test]
    fn request_message_payload_equality_different_variants() {
        let binary = RequestMessagePayload::Binary(vec![1]);
        let utf8 = RequestMessagePayload::UTF8("1".to_string());
        assert_ne!(binary, utf8);
    }

    // ── JsonSchemaRequired ────────────────────────────────────────────

    #[test]
    fn json_schema_required_clone_and_eq() {
        let yes = JsonSchemaRequired::Yes;
        let no = JsonSchemaRequired::No;
        assert_eq!(yes.clone(), JsonSchemaRequired::Yes);
        assert_eq!(no.clone(), JsonSchemaRequired::No);
        assert_ne!(yes, no);
    }

    // ── ReplyMessageEncoding ──────────────────────────────────────────

    #[test]
    fn reply_message_encoding_binary() {
        let enc = ReplyMessageEncoding::Binary;
        assert_eq!(enc, ReplyMessageEncoding::Binary);
    }

    #[test]
    fn reply_message_encoding_json_with_schema() {
        let enc = ReplyMessageEncoding::JSON(JsonSchemaRequired::Yes);
        assert_eq!(enc, ReplyMessageEncoding::JSON(JsonSchemaRequired::Yes));
    }

    #[test]
    fn reply_message_encoding_json_without_schema() {
        let enc = ReplyMessageEncoding::JSON(JsonSchemaRequired::No);
        assert_eq!(enc, ReplyMessageEncoding::JSON(JsonSchemaRequired::No));
    }

    #[test]
    fn reply_message_encoding_utf8() {
        let enc = ReplyMessageEncoding::UTF8;
        assert_eq!(enc, ReplyMessageEncoding::UTF8);
    }

    #[test]
    fn reply_message_encoding_clone() {
        let enc = ReplyMessageEncoding::JSON(JsonSchemaRequired::Yes);
        let cloned = enc.clone();
        assert_eq!(enc, cloned);
    }

    #[test]
    fn reply_message_encoding_inequality() {
        assert_ne!(ReplyMessageEncoding::Binary, ReplyMessageEncoding::UTF8);
        assert_ne!(
            ReplyMessageEncoding::JSON(JsonSchemaRequired::Yes),
            ReplyMessageEncoding::JSON(JsonSchemaRequired::No)
        );
    }

    // ── ReplyMessagePayload enum ──────────────────────────────────────

    #[test]
    fn reply_message_payload_binary() {
        let payload = ReplyMessagePayload::Binary(vec![1, 2]);
        if let ReplyMessagePayload::Binary(data) = &payload {
            assert_eq!(data, &[1, 2]);
        } else {
            panic!("expected Binary variant");
        }
    }

    #[test]
    fn reply_message_payload_json_variant() {
        let val = serde_json::json!("x");
        let schema = serde_json::json!({"type": "string"});
        let payload = ReplyMessagePayload::JSON(val.clone(), Some(schema.clone()));
        assert_eq!(payload, ReplyMessagePayload::JSON(val, Some(schema)));
    }

    #[test]
    fn reply_message_payload_clone() {
        let original = ReplyMessagePayload::UTF8("hello".to_string());
        let cloned = original.clone();
        assert_eq!(original, cloned);
    }

    #[test]
    fn reply_message_payload_equality_different_variants() {
        let binary = ReplyMessagePayload::Binary(vec![1]);
        let utf8 = ReplyMessagePayload::UTF8("1".to_string());
        assert_ne!(binary, utf8);
    }

    // ── ReplyMessagePayload factory methods ───────────────────────────

    #[test]
    fn reply_message_payload_from_byte_vector() {
        let payload = ReplyMessagePayload::from_byte_vector(vec![0xAA, 0xBB]);
        assert_eq!(payload, ReplyMessagePayload::Binary(vec![0xAA, 0xBB]));
    }

    #[test]
    fn reply_message_payload_from_json() {
        let json_val = serde_json::json!({"a": 1});
        let payload = ReplyMessagePayload::from_json(json_val.clone(), None);
        assert_eq!(payload, ReplyMessagePayload::JSON(json_val, None));
    }

    #[test]
    fn reply_message_payload_from_json_with_schema() {
        let json_val = serde_json::json!({"a": 1});
        let schema = serde_json::json!({"type": "object"});
        let payload = ReplyMessagePayload::from_json(json_val.clone(), Some(schema.clone()));
        assert_eq!(payload, ReplyMessagePayload::JSON(json_val, Some(schema)));
    }

    #[test]
    fn reply_message_payload_from_string() {
        let payload = ReplyMessagePayload::from_string("text".to_string());
        assert_eq!(payload, ReplyMessagePayload::UTF8("text".to_string()));
    }

    // ── DiagnosticReply with ReplyMessagePayload::JSON ────────────────

    #[test]
    fn diagnostic_reply_with_json_payload() {
        let json_val = serde_json::json!({"result": true});
        let schema = serde_json::json!({"type": "object"});
        let reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::JSON(
                json_val.clone(),
                Some(schema.clone()),
            )),
            additional_attrs: None,
        };
        assert_eq!(
            reply.message_payload,
            Some(ReplyMessagePayload::JSON(json_val, Some(schema)))
        );
    }

    #[test]
    fn diagnostic_reply_empty() {
        let reply = DiagnosticReply {
            message_payload: None,
            additional_attrs: None,
        };
        assert!(reply.message_payload.is_none());
        assert!(reply.additional_attrs.is_none());
    }

    // ── DiagnosticReply ───────────────────────────────────────────────

    #[test]
    fn diagnostic_reply_with_payload() {
        let reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::UTF8("result".to_string())),
            additional_attrs: None,
        };
        assert_eq!(
            reply.message_payload,
            Some(ReplyMessagePayload::UTF8("result".to_string()))
        );
        assert!(reply.additional_attrs.is_none());
    }

    #[test]
    fn diagnostic_reply_with_attrs() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("status".to_string(), "ok".to_string());
        let reply = DiagnosticReply {
            message_payload: None,
            additional_attrs: Some(attrs),
        };
        assert!(reply.message_payload.is_none());
        assert_eq!(
            reply.additional_attrs.as_ref().unwrap().get("status"),
            Some(&"ok".to_string())
        );
    }

    #[test]
    fn diagnostic_reply_clone() {
        let reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::Binary(vec![1, 2, 3])),
            additional_attrs: None,
        };
        let cloned = reply.clone();
        assert_eq!(reply, cloned);
    }

    #[test]
    fn diagnostic_reply_equality() {
        let reply_a = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::UTF8("a".to_string())),
            additional_attrs: None,
        };
        let reply_b = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::UTF8("b".to_string())),
            additional_attrs: None,
        };
        assert_ne!(reply_a, reply_b);
    }

    // ── Error / ErrorCode ─────────────────────────────────────────────

    #[test]
    fn error_from_sovd_error() {
        let sovd_err = sovd::GenericError::from_code(
            sovd::ErrorCode::SovdServerFailure,
            "server down".to_string(),
        );
        let err = Error::from_error(sovd_err);
        match err.code {
            ErrorCode::SOVD(inner) => {
                assert_eq!(inner.sovd_error, "sovd-server-failure");
            }
            _ => {
                panic!("expected SOVD error code");
            }
        };
        assert!(err.payload.is_none());
    }

    #[test]
    fn error_clone() {
        let err = Error::from_error(sovd::GenericError::from_code(
            sovd::ErrorCode::ErrorResponse,
            "msg".to_string(),
        ));
        let cloned = err.clone();
        if let (ErrorCode::SOVD(ref a), ErrorCode::SOVD(ref b)) = (&err.code, &cloned.code) {
            assert_eq!(a.sovd_error, b.sovd_error);
            assert_eq!(a.message_text, b.message_text);
        } else {
            panic!("expected SOVD variants");
        }
    }

    // ── Result type alias ─────────────────────────────────────────────

    #[test]
    fn result_ok_variant() {
        let result: Result<i32> = Ok(42);
        assert_eq!(result.unwrap(), 42);
    }

    #[test]
    fn result_err_variant() {
        let result: Result<i32> = Err(Error::from_error(sovd::GenericError::from_code(
            sovd::ErrorCode::ErrorResponse,
            "fail".to_string(),
        )));
        assert!(result.is_err());
    }

    // ── Error with payload ────────────────────────────────────────────

    #[test]
    fn error_from_sovd_error_with_payload() {
        let sovd_err =
            sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string());
        let mut err = Error::from_error(sovd_err);
        assert!(err.payload.is_none());
        err.payload = Some(ReplyMessagePayload::UTF8("detail".to_string()));
        assert_eq!(
            err.payload,
            Some(ReplyMessagePayload::UTF8("detail".to_string()))
        );
    }

    #[test]
    fn error_with_payload_builder() {
        let sovd_err =
            sovd::GenericError::from_code(sovd::ErrorCode::ErrorResponse, "msg".to_string());
        let err =
            Error::from_error(sovd_err).with_payload(ReplyMessagePayload::Binary(vec![0xCA, 0xFE]));
        assert_eq!(
            err.payload,
            Some(ReplyMessagePayload::Binary(vec![0xCA, 0xFE]))
        );
    }

    #[test]
    fn error_mutex_error() {
        let err = Error::mutex_poisoned();
        match &err.code {
            ErrorCode::SOVD(inner) => {
                assert_eq!(inner.sovd_error, "sovd-server-failure");
                assert_eq!(inner.message_text, "mutex acquisition failed unexpectedly");
            }
            _ => {
                panic!("expected SOVD error code");
            }
        }
        assert!(err.payload.is_none());
    }

    // ── DiagnosticReply Default ───────────────────────────────────────

    #[test]
    fn diagnostic_reply_default() {
        let reply = DiagnosticReply::default();
        assert!(reply.message_payload.is_none());
        assert!(reply.additional_attrs.is_none());
    }
}
