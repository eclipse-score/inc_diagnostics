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

use common::Result as DiagResult;
use common::{DiagnosticReply, KeyValueAttributes, ReplyMessageEncoding, RequestMessagePayload};
use indexmap::IndexMap;

use std::future::Future;
use std::pin::Pin;

use futures::future::BoxFuture;

/*****************/
/* General Types */
/*****************/

/// Alias for an operation's input/output user parameters.
pub type UserParameters = RequestMessagePayload;

/// cf. ISO 17978-3:2025 Section 7.14.6, Table 181
pub struct ExecuteArguments {
    pub reply_encoding: ReplyMessageEncoding,
    pub user_parameters: Option<UserParameters>,
    pub additional_attrs: Option<KeyValueAttributes>,
    pub proximity_response: Option<String>,
}

/// cf. ISO 17978-3:2025 Section 7.14.9, Table 194
pub type CustomCapability = String;

/// cf. ISO 17978-3:2025 Section 7.14.6, Table 185
#[derive(Clone, Debug, PartialEq)]
pub enum ExecutionStatus {
    UnsupportedCapability(CustomCapability),
    Unknown,
    Scheduled,
    Running,
    Interrupted,
    Completed,
    Stopped,
    Failed,
}

/// cf. ISO 17978-3:2025 Section 7.14.7, Table 186
pub type ExecutionId = String;

/// cf. ISO 17978-3:2025 Section 7.14.7, Table 189
pub struct ExecutionStatusDetails {
    pub last_executed_capability: String,
    pub completion_percentage: Option<u8>,
    pub event_result: Option<DiagnosticReply>,
    pub exec_errors: Option<Vec<::common::Error>>,
}

impl ExecutionStatusDetails {
    #[must_use]
    pub fn default() -> Self {
        Self {
            last_executed_capability: "n/a".to_string(),
            completion_percentage: None,
            event_result: None,
            exec_errors: None,
        }
    }

    #[must_use]
    pub fn new(event_kind: ExecutionEventKind) -> Self {
        Self {
            last_executed_capability: event_kind.to_string(),
            completion_percentage: None,
            event_result: None,
            exec_errors: None,
        }
    }

    #[must_use]
    pub fn with_completion_percentage(mut self, completion_percentage: u8) -> Self {
        self.completion_percentage = Some(completion_percentage);
        self
    }

    #[must_use]
    pub fn with_reply_data(mut self, event_result: DiagnosticReply) -> Self {
        self.event_result = Some(event_result);
        self
    }

    #[must_use]
    pub fn with_exec_errors(mut self, exec_errors: Vec<::common::Error>) -> Self {
        self.exec_errors = Some(exec_errors);
        self
    }
}

/// Reports the current execution status back to the runtime.
/// Used as callback as part of an `ExecutionEvent`.
pub struct StatusReporter {
    inner: Option<Box<dyn FnOnce(ExecutionStatus, ExecutionStatusDetails) + Send>>,
}

impl StatusReporter {
    #[must_use]
    pub fn default() -> Self {
        Self { inner: None }
    }

    #[must_use]
    pub fn new<Func>(func: Func) -> Self
    where
        Func: FnOnce(ExecutionStatus, ExecutionStatusDetails) + Send + 'static,
    {
        Self {
            inner: Some(Box::new(func)),
        }
    }

    pub fn put(self, status: ExecutionStatus, details: ExecutionStatusDetails) {
        if let Some(reporter) = self.inner {
            (reporter)(status, details)
        }
    }
}

/// Kind type for events delivered to an operation's execution control loop.
/// Maps to the SOVD capability model (cf. ISO 17978-3:2025 Section 7.14.9).
#[derive(Clone)]
pub enum ExecutionEventKind {
    HandleCustomCapability(CustomCapability),
    ReportStatus,
    ControlGone,
    Interrupt,
    Resume,
    Reset,
    Stop,
}

impl std::fmt::Display for ExecutionEventKind {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ExecutionEventKind::HandleCustomCapability(cap) => write!(f, "{cap}"),
            ExecutionEventKind::ReportStatus => write!(f, "status"),
            ExecutionEventKind::ControlGone => write!(f, "unknown"),
            ExecutionEventKind::Interrupt => write!(f, "freeze"),
            ExecutionEventKind::Resume => write!(f, "execute"),
            ExecutionEventKind::Reset => write!(f, "reset"),
            ExecutionEventKind::Stop => write!(f, "stop"),
        }
    }
}

/// Events delivered to an operation's execution control loop.
/// Maps to the SOVD capability model (cf. ISO 17978-3:2025 Section 7.14.5, Table 178).
pub struct ExecutionEvent {
    pub kind: ExecutionEventKind,
    pub args: Option<ExecuteArguments>,
    pub status_reporter: StatusReporter,
}

impl ExecutionEvent {
    #[must_use]
    pub fn new(kind: ExecutionEventKind) -> Self {
        Self {
            kind: kind,
            args: None,
            status_reporter: StatusReporter::default(),
        }
    }

    #[must_use]
    pub fn with_args(mut self, args: ExecuteArguments) -> Self {
        self.args = Some(args);
        self
    }

    #[must_use]
    pub fn with_status_reporter<Func>(mut self, func: Func) -> Self
    where
        Func: FnOnce(ExecutionStatus, ExecutionStatusDetails) + Send + 'static,
    {
        self.status_reporter = StatusReporter::new(func);
        self
    }
}

/// Trait for receiving execution control events from the runtime.
/// cf. ISO 17978-3:2025 Sections 7.14.7 / 7.14.9
pub trait ExecutionControlApi {
    /// Await the next execution control event from the runtime.
    #[must_use]
    fn next_exec_event(&mut self) -> BoxFuture<'_, ExecutionEvent>;
}

/// Handle for receiving execution control events, wrapping an `ExecutionControlApi` implementation.
pub struct ExecutionControl {
    inner: Box<dyn ExecutionControlApi + Send>,
    exec_id: ExecutionId,
}

impl ExecutionControl {
    #[must_use]
    pub fn new(api: impl ExecutionControlApi + Send + 'static, exec_id: ExecutionId) -> Self {
        Self {
            inner: Box::new(api),
            exec_id: exec_id,
        }
    }

    #[must_use]
    pub fn next_exec_event(&mut self) -> BoxFuture<'_, ExecutionEvent> {
        self.inner.next_exec_event()
    }

    #[must_use]
    pub fn exec_id(&self) -> &ExecutionId {
        &self.exec_id
    }
}

/// The result of an operation execution: either a successful `DiagnosticReply`
/// or a failure indicated by the contained error object.
pub type ExecutionResult = DiagResult<DiagnosticReply>;

/// Returned by `Operation::execute` and contains a future which produces an `ExecutionResult`,
/// along with an optional initial `DiagnosticReply` which, if present, is intended to get
/// used as reply to the `Operation::execute` request instead of the default one.
#[must_use]
pub struct ExecutionHandle {
    pub future: Pin<Box<dyn Future<Output = ExecutionResult> + Send>>,
    pub reply: Option<DiagnosticReply>,
}

impl ExecutionHandle {
    pub fn from_future<Fut>(future: Fut) -> Self
    where
        Fut: Future<Output = ExecutionResult> + Send + 'static,
    {
        Self {
            future: Box::pin(future),
            reply: None,
        }
    }

    pub fn from_closure<Func>(func: Func) -> Self
    where
        Func: FnOnce() -> ExecutionResult + Send + 'static,
    {
        Self {
            future: Box::pin(async move { func() }),
            reply: None,
        }
    }

    pub fn with_reply(mut self, r: DiagnosticReply) -> Self {
        self.reply = Some(r);
        self
    }
}

/*************************/
/* Operation metadata    */
/*************************/

/// cf. ISO 17978-3:2025 Section 7.14.5, Table 176
#[derive(Clone, Debug)]
pub struct OperationMetadata {
    /// cf. ISO 17978-3:2025 Table 169: If true, execution requires proof of co-location.
    pub proximity_proof_required: bool,
    /// cf. ISO 17978-3:2025 Table 169: If true, execution shall get performed synchronously.
    pub synchronous_execution: bool,
    /// If true, executions shall not get performed at the same time in parallel.
    pub exclusive_execution: bool,
    /// cf. ISO 17978-3:2025 Table 176: Required modes to execute the operation.
    /// Key is the mode-id, value lists the valid mode values.
    pub supported_modes: Option<IndexMap<String, Vec<String>>>,
}

/*********************/
/* Operation API     */
/*********************/

/// Trait representing a single SOVD operation that can be executed on an Entity.
/// cf. ISO 17978-3:2025 Section 7.14
pub trait Operation {
    /// Execute the operation with the given input arguments and execution control handle.
    ///
    /// NOTE: This method is conceptually async since the returned `ExecutionHandle`
    ///       contains (in case of success) the respective `Future` object.
    fn execute(
        &mut self,
        input: ExecuteArguments,
        control: ExecutionControl,
    ) -> DiagResult<ExecutionHandle>;
}

/*******************/
/* Unit Tests      */
/*******************/

#[cfg(test)]
mod tests {
    use super::*;
    use common::ReplyMessagePayload;

    // ── ExecutionStatus ───────────────────────────────────────────────

    #[test]
    fn execution_status_clone() {
        let status = ExecutionStatus::Running;
        let cloned = status.clone();
        assert_eq!(status, cloned);
    }

    #[test]
    fn execution_status_equality() {
        assert_eq!(ExecutionStatus::Completed, ExecutionStatus::Completed);
        assert_ne!(ExecutionStatus::Running, ExecutionStatus::Stopped);
    }

    #[test]
    fn execution_status_all_variants_are_distinct() {
        let variants = [
            ExecutionStatus::UnsupportedCapability("custom".to_string()),
            ExecutionStatus::Unknown,
            ExecutionStatus::Scheduled,
            ExecutionStatus::Running,
            ExecutionStatus::Interrupted,
            ExecutionStatus::Completed,
            ExecutionStatus::Stopped,
            ExecutionStatus::Failed,
        ];
        for i in 0..variants.len() {
            for j in (i + 1)..variants.len() {
                assert_ne!(variants[i], variants[j]);
            }
        }
    }

    #[test]
    fn execution_status_debug() {
        assert_eq!(format!("{:?}", ExecutionStatus::Running), "Running");
        assert_eq!(format!("{:?}", ExecutionStatus::Failed), "Failed");
    }

    // ── ExecuteArguments ──────────────────────────────────────────────

    #[test]
    fn execute_arguments_all_none() {
        let args = ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::UTF8,
            user_parameters: None,
            additional_attrs: None,
            proximity_response: None,
        };
        assert!(args.user_parameters.is_none());
        assert!(args.additional_attrs.is_none());
        assert!(args.proximity_response.is_none());
    }

    #[test]
    fn execute_arguments_with_user_parameters() {
        let args = ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::UTF8,
            user_parameters: Some(RequestMessagePayload::UTF8("input".to_string())),
            additional_attrs: None,
            proximity_response: None,
        };
        assert_eq!(
            args.user_parameters,
            Some(RequestMessagePayload::UTF8("input".to_string()))
        );
    }

    #[test]
    fn execute_arguments_with_all_fields() {
        let mut attrs = KeyValueAttributes::new();
        attrs.insert("key".to_string(), "val".to_string());
        let args = ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::Binary,
            user_parameters: Some(RequestMessagePayload::Binary(vec![1, 2])),
            additional_attrs: Some(attrs),
            proximity_response: Some("proof".to_string()),
        };
        assert!(args.user_parameters.is_some());
        assert!(args.additional_attrs.is_some());
        assert_eq!(args.proximity_response.as_deref(), Some("proof"));
    }

    #[test]
    fn execute_arguments_reply_encoding_json_with_schema() {
        let args = ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::JSON(::common::JsonSchemaRequired::Yes),
            user_parameters: None,
            additional_attrs: None,
            proximity_response: None,
        };
        assert_eq!(
            args.reply_encoding,
            ReplyMessageEncoding::JSON(::common::JsonSchemaRequired::Yes)
        );
    }

    // ── ExecutionStatusDetails ────────────────────────────────────────

    #[test]
    fn execution_status_details_all_none() {
        let details = ExecutionStatusDetails::default();
        assert_eq!(details.last_executed_capability, "n/a");
        assert!(details.completion_percentage.is_none());
        assert!(details.event_result.is_none());
        assert!(details.exec_errors.is_none());
    }

    #[test]
    fn execution_status_details_with_reply_data() {
        let details = ExecutionStatusDetails::default().with_reply_data(DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string("done".to_string())),
            additional_attrs: None,
        });
        assert!(details.completion_percentage.is_none());
        assert!(details.event_result.is_some());
        assert!(details.exec_errors.is_none());
    }

    #[test]
    fn execution_status_details_with_errors() {
        let err = ::common::Error::from_error(::common::sovd::GenericError::from_code(
            ::common::sovd::ErrorCode::ErrorResponse,
            "fail".to_string(),
        ));
        let details = ExecutionStatusDetails::default().with_exec_errors(vec![err]);
        assert_eq!(details.exec_errors.as_ref().unwrap().len(), 1);
    }

    #[test]
    fn execution_status_details_with_all_fields() {
        let err = ::common::Error::from_error(::common::sovd::GenericError::from_code(
            ::common::sovd::ErrorCode::NotResponding,
            "timeout".to_string(),
        ));
        let details = ExecutionStatusDetails::new(ExecutionEventKind::Stop)
            .with_completion_percentage(100)
            .with_reply_data(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::from_string("ok".to_string())),
                additional_attrs: None,
            })
            .with_exec_errors(vec![err]);
        assert_eq!(details.last_executed_capability, "stop");
        assert_eq!(details.completion_percentage, Some(100));
        assert!(details.event_result.is_some());
        assert_eq!(details.exec_errors.as_ref().unwrap().len(), 1);
    }

    // ── StatusReporter ────────────────────────────────────────────────

    #[test]
    fn status_reporter_invokes_callback() {
        use std::sync::{Arc, Mutex};
        let received = Arc::new(Mutex::new(None));
        let received_clone = received.clone();
        let reporter = StatusReporter::new(move |status, _details| {
            *received_clone.lock().unwrap() = Some(status);
        });
        reporter.put(
            ExecutionStatus::Completed,
            ExecutionStatusDetails::default(),
        );
        assert_eq!(*received.lock().unwrap(), Some(ExecutionStatus::Completed));
    }

    #[test]
    fn status_reporter_receives_details() {
        use std::sync::{Arc, Mutex};
        let received_pct = Arc::new(Mutex::new(None));
        let pct_clone = received_pct.clone();
        let reporter = StatusReporter::new(move |_status, details| {
            *pct_clone.lock().unwrap() = details.completion_percentage;
        });
        let details =
            ExecutionStatusDetails::new(ExecutionEventKind::Resume).with_completion_percentage(42);
        reporter.put(ExecutionStatus::Running, details);
        assert_eq!(*received_pct.lock().unwrap(), Some(42));
    }

    #[test]
    fn status_reporter_receives_result_data() {
        use std::sync::{Arc, Mutex};
        let received = Arc::new(Mutex::new(None));
        let recv_clone = received.clone();
        let reporter = StatusReporter::new(move |_status, details| {
            *recv_clone.lock().unwrap() = details.event_result;
        });
        let details = ExecutionStatusDetails::new(ExecutionEventKind::Resume).with_reply_data(
            DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::from_string("payload".to_string())),
                additional_attrs: None,
            },
        );
        reporter.put(ExecutionStatus::Running, details);
        assert!(received.lock().unwrap().is_some());
    }

    #[test]
    fn status_reporter_default_does_not_panic() {
        let reporter = StatusReporter::default();
        reporter.put(
            ExecutionStatus::Completed,
            ExecutionStatusDetails::default(),
        );
    }

    // ── ExecutionEventKind ────────────────────────────────────────────

    #[test]
    fn execution_event_kind_display_handle_custom_capability() {
        let kind = ExecutionEventKind::HandleCustomCapability("my_custom_capability".to_string());
        assert_eq!(kind.to_string(), "my_custom_capability");
    }

    #[test]
    fn execution_event_kind_display_report_status() {
        assert_eq!(ExecutionEventKind::ReportStatus.to_string(), "status");
    }

    #[test]
    fn execution_event_kind_display_control_gone() {
        assert_eq!(ExecutionEventKind::ControlGone.to_string(), "unknown");
    }

    #[test]
    fn execution_event_kind_display_interrupt() {
        assert_eq!(ExecutionEventKind::Interrupt.to_string(), "freeze");
    }

    #[test]
    fn execution_event_kind_display_resume() {
        assert_eq!(ExecutionEventKind::Resume.to_string(), "execute");
    }

    #[test]
    fn execution_event_kind_display_reset() {
        assert_eq!(ExecutionEventKind::Reset.to_string(), "reset");
    }

    #[test]
    fn execution_event_kind_display_stop() {
        assert_eq!(ExecutionEventKind::Stop.to_string(), "stop");
    }

    // ── ExecutionEvent ────────────────────────────────────────────────

    #[test]
    fn execution_event_from_kind_has_no_args() {
        let event = ExecutionEvent::new(ExecutionEventKind::Stop);
        assert!(event.args.is_none());
        assert!(matches!(event.kind, ExecutionEventKind::Stop));
    }

    #[test]
    fn execution_event_from_kind_and_args() {
        let args = ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::Binary,
            user_parameters: Some(RequestMessagePayload::Binary(vec![0xAB])),
            additional_attrs: None,
            proximity_response: None,
        };
        let event = ExecutionEvent::new(ExecutionEventKind::Interrupt).with_args(args);
        assert!(matches!(event.kind, ExecutionEventKind::Interrupt));
        assert!(event.args.is_some());
        assert_eq!(
            event.args.unwrap().user_parameters,
            Some(RequestMessagePayload::Binary(vec![0xAB]))
        );
    }

    #[test]
    fn execution_event_from_kind_control_gone() {
        let event = ExecutionEvent::new(ExecutionEventKind::ControlGone);
        assert!(matches!(event.kind, ExecutionEventKind::ControlGone));
    }

    #[test]
    fn execution_event_from_kind_resume() {
        let event = ExecutionEvent::new(ExecutionEventKind::Resume);
        assert!(matches!(event.kind, ExecutionEventKind::Resume));
    }

    #[test]
    fn execution_event_from_kind_handle_custom_capability() {
        let event = ExecutionEvent::new(ExecutionEventKind::HandleCustomCapability(
            "my_cap".to_string(),
        ));
        if let ExecutionEventKind::HandleCustomCapability(val) = event.kind {
            assert_eq!(val, "my_cap");
        } else {
            panic!("expected HandleCustomCapability");
        }
    }

    #[test]
    fn execution_event_report_status_with_reporter() {
        use std::sync::{Arc, Mutex};
        let called = Arc::new(Mutex::new(false));
        let called_clone = called.clone();
        let event = ExecutionEvent::new(ExecutionEventKind::ReportStatus).with_status_reporter(
            move |_status, _details| {
                *called_clone.lock().unwrap() = true;
            },
        );
        assert!(event.args.is_none());
        assert!(matches!(event.kind, ExecutionEventKind::ReportStatus));
        event
            .status_reporter
            .put(ExecutionStatus::Running, ExecutionStatusDetails::default());
        assert!(*called.lock().unwrap());
    }

    #[test]
    fn execution_event_from_kind_interrupt() {
        let event = ExecutionEvent::new(ExecutionEventKind::Interrupt);
        assert!(matches!(event.kind, ExecutionEventKind::Interrupt));
        assert!(event.args.is_none());
    }

    #[test]
    fn execution_event_with_args_builder() {
        let args = ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::UTF8,
            user_parameters: Some(RequestMessagePayload::UTF8("test".to_string())),
            additional_attrs: None,
            proximity_response: None,
        };
        let event = ExecutionEvent::new(ExecutionEventKind::Stop).with_args(args);
        assert!(event.args.is_some());
        assert_eq!(
            event.args.unwrap().user_parameters,
            Some(RequestMessagePayload::UTF8("test".to_string()))
        );
    }

    #[test]
    fn execution_event_with_status_reporter_builder() {
        use std::sync::{Arc, Mutex};
        let called = Arc::new(Mutex::new(false));
        let called_clone = called.clone();
        let event = ExecutionEvent::new(ExecutionEventKind::Interrupt).with_status_reporter(
            move |_status, _details| {
                *called_clone.lock().unwrap() = true;
            },
        );
        event.status_reporter.put(
            ExecutionStatus::Interrupted,
            ExecutionStatusDetails::default(),
        );
        assert!(*called.lock().unwrap());
    }

    #[test]
    fn execution_event_from_kind_report_status() {
        let event = ExecutionEvent::new(ExecutionEventKind::ReportStatus);
        assert!(matches!(event.kind, ExecutionEventKind::ReportStatus));
        assert!(event.args.is_none());
    }

    // ── ExecutionHandle ───────────────────────────────────────────────

    #[tokio::test]
    async fn execution_handle_from_closure_ok() {
        let reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string("result".to_string())),
            additional_attrs: None,
        };
        let handle = ExecutionHandle::from_closure(move || Ok(reply));
        assert!(handle.reply.is_none());
        let result = handle.future.await;
        assert!(result.is_ok());
        assert_eq!(
            result.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string("result".to_string()))
        );
    }

    #[tokio::test]
    async fn execution_handle_from_closure_err() {
        let handle = ExecutionHandle::from_closure(|| Err(::common::Error::mutex_poisoned()));
        let result = handle.future.await;
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert!(err.payload.is_none());
        assert!(matches!(err.code, ::common::ErrorCode::SOVD(_)));
        let ::common::ErrorCode::SOVD(sovd_err) = err.code else {
            panic!("expected SOVD error code")
        };
        assert_eq!(sovd_err.sovd_error, "sovd-server-failure");
        assert_eq!(
            sovd_err.message_text,
            "mutex acquisition failed unexpectedly"
        );
    }

    #[tokio::test]
    async fn execution_handle_from_closure_and_reply() {
        let initial_reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string("initial".to_string())),
            additional_attrs: None,
        };
        let final_reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string("final".to_string())),
            additional_attrs: None,
        };
        let handle =
            ExecutionHandle::from_closure(move || Ok(final_reply)).with_reply(initial_reply);
        assert!(handle.reply.is_some());
        assert_eq!(
            handle.reply.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string("initial".to_string()))
        );
        let result = handle.future.await;
        assert_eq!(
            result.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string("final".to_string()))
        );
    }

    #[tokio::test]
    async fn execution_handle_from_future() {
        let handle = ExecutionHandle::from_future(async {
            Ok(DiagnosticReply {
                message_payload: None,
                additional_attrs: None,
            })
        });
        assert!(handle.reply.is_none());
        let result = handle.future.await;
        assert!(result.is_ok());
        assert!(result.unwrap().message_payload.is_none());
    }

    #[tokio::test]
    async fn execution_handle_from_future_and_reply() {
        let reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_byte_vector(vec![1])),
            additional_attrs: None,
        };
        let err = ::common::Error::from_error(::common::sovd::GenericError::from_code(
            ::common::sovd::ErrorCode::ErrorResponse,
            "stopped".to_string(),
        ));
        let handle = ExecutionHandle::from_future(async { Err(err) }).with_reply(reply);
        assert!(handle.reply.is_some());
        let result = handle.future.await;
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert!(matches!(err.code, ::common::ErrorCode::SOVD(_)));
        let ::common::ErrorCode::SOVD(sovd_err) = err.code else {
            panic!("expected SOVD error code")
        };
        assert_eq!(sovd_err.sovd_error, "error-response");
        assert_eq!(sovd_err.message_text, "stopped");
    }

    #[tokio::test]
    async fn execution_handle_from_future_and_reply_ok() {
        let reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string("initial".to_string())),
            additional_attrs: None,
        };
        let handle = ExecutionHandle::from_future(async {
            Ok(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::from_string("final".to_string())),
                additional_attrs: None,
            })
        })
        .with_reply(reply);
        assert_eq!(
            handle.reply.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string("initial".to_string()))
        );
        let result = handle.future.await;
        assert_eq!(
            result.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string("final".to_string()))
        );
    }

    #[tokio::test]
    async fn execution_handle_from_closure_stopped() {
        let handle = ExecutionHandle::from_closure(|| {
            Err(::common::Error::from_error(
                ::common::sovd::GenericError::from_code(
                    ::common::sovd::ErrorCode::ErrorResponse,
                    "stopped".to_string(),
                ),
            ))
        });
        let result = handle.future.await;
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert!(matches!(err.code, ::common::ErrorCode::SOVD(_)));
        let ::common::ErrorCode::SOVD(sovd_err) = err.code else {
            panic!("expected SOVD error code")
        };
        assert_eq!(sovd_err.sovd_error, "error-response");
        assert_eq!(sovd_err.message_text, "stopped");
    }

    #[test]
    fn execution_handle_from_error() {
        let result: DiagResult<ExecutionHandle> = Err(::common::Error::from_error(
            ::common::sovd::GenericError::from_code(
                ::common::sovd::ErrorCode::ErrorResponse,
                "test error".to_string(),
            ),
        ));
        assert!(result.is_err());
    }

    // ── OperationMetadata ─────────────────────────────────────────────

    #[test]
    fn operation_metadata_default_fields() {
        let meta = OperationMetadata {
            proximity_proof_required: false,
            synchronous_execution: false,
            exclusive_execution: false,
            supported_modes: None,
        };
        assert!(!meta.proximity_proof_required);
        assert!(!meta.synchronous_execution);
        assert!(!meta.exclusive_execution);
        assert!(meta.supported_modes.is_none());
    }

    #[test]
    fn operation_metadata_with_modes() {
        let mut modes = IndexMap::<String, Vec<String>>::new();
        modes.insert(
            "mode_a".to_string(),
            vec!["val1".to_string(), "val2".to_string()],
        );
        let meta = OperationMetadata {
            proximity_proof_required: true,
            synchronous_execution: true,
            exclusive_execution: true,
            supported_modes: Some(modes),
        };
        assert!(meta.proximity_proof_required);
        assert!(meta.synchronous_execution);
        assert!(meta.exclusive_execution);
        let modes = meta.supported_modes.as_ref().unwrap();
        assert_eq!(modes.len(), 1);
        assert_eq!(modes.get("mode_a").unwrap(), &["val1", "val2"]);
    }

    #[test]
    fn operation_metadata_clone() {
        let meta = OperationMetadata {
            proximity_proof_required: true,
            synchronous_execution: false,
            exclusive_execution: true,
            supported_modes: None,
        };
        let cloned = meta.clone();
        assert_eq!(
            cloned.proximity_proof_required,
            meta.proximity_proof_required
        );
        assert_eq!(cloned.synchronous_execution, meta.synchronous_execution);
        assert_eq!(cloned.exclusive_execution, meta.exclusive_execution);
    }

    #[test]
    fn operation_metadata_debug() {
        let meta = OperationMetadata {
            proximity_proof_required: false,
            synchronous_execution: false,
            exclusive_execution: false,
            supported_modes: None,
        };
        let debug_str = format!("{:?}", meta);
        assert!(debug_str.contains("OperationMetadata"));
    }

    #[test]
    fn operation_metadata_with_multiple_modes() {
        let mut modes = IndexMap::<String, Vec<String>>::new();
        modes.insert("mode_a".to_string(), vec!["v1".to_string()]);
        modes.insert(
            "mode_b".to_string(),
            vec!["v2".to_string(), "v3".to_string()],
        );
        let meta = OperationMetadata {
            proximity_proof_required: false,
            synchronous_execution: true,
            exclusive_execution: false,
            supported_modes: Some(modes),
        };
        let modes = meta.supported_modes.as_ref().unwrap();
        assert_eq!(modes.len(), 2);
        assert_eq!(modes.get("mode_b").unwrap().len(), 2);
    }

    // ── ExecutionControl ──────────────────────────────────────────────

    #[tokio::test]
    async fn execution_control_wraps_api() {
        struct MockControl {
            events: Vec<ExecutionEventKind>,
        }

        impl ExecutionControlApi for MockControl {
            fn next_exec_event(&mut self) -> BoxFuture<'_, ExecutionEvent> {
                let kind = if self.events.is_empty() {
                    ExecutionEventKind::ControlGone
                } else {
                    self.events.remove(0)
                };
                Box::pin(async move {
                    ExecutionEvent {
                        kind,
                        args: None,
                        status_reporter: StatusReporter::default(),
                    }
                })
            }
        }

        let mock = MockControl {
            events: vec![ExecutionEventKind::Stop],
        };
        let mut ctrl = ExecutionControl::new(mock, "exec-1".to_string());
        let event = ctrl.next_exec_event().await;
        assert!(matches!(event.kind, ExecutionEventKind::Stop));

        let event2 = ctrl.next_exec_event().await;
        assert!(matches!(event2.kind, ExecutionEventKind::ControlGone));
    }

    #[tokio::test]
    async fn execution_control_event_with_args() {
        struct MockControlWithArgs;

        impl ExecutionControlApi for MockControlWithArgs {
            fn next_exec_event(&mut self) -> BoxFuture<'_, ExecutionEvent> {
                Box::pin(async move {
                    ExecutionEvent::new(ExecutionEventKind::Resume).with_args(ExecuteArguments {
                        reply_encoding: ReplyMessageEncoding::Binary,
                        user_parameters: Some(RequestMessagePayload::Binary(vec![0xCD])),
                        additional_attrs: None,
                        proximity_response: None,
                    })
                })
            }
        }

        let mut ctrl = ExecutionControl::new(MockControlWithArgs, "exec-2".to_string());
        let event = ctrl.next_exec_event().await;
        assert!(matches!(event.kind, ExecutionEventKind::Resume));
        let args = event.args.unwrap();
        assert_eq!(
            args.user_parameters,
            Some(RequestMessagePayload::Binary(vec![0xCD]))
        );
    }

    #[tokio::test]
    async fn execution_control_get_exec_id() {
        struct DummyControl;

        impl ExecutionControlApi for DummyControl {
            fn next_exec_event(&mut self) -> BoxFuture<'_, ExecutionEvent> {
                Box::pin(async { ExecutionEvent::new(ExecutionEventKind::ControlGone) })
            }
        }

        let ctrl = ExecutionControl::new(DummyControl, "my-exec-42".to_string());
        assert_eq!(ctrl.exec_id(), "my-exec-42");
    }

    // ── Operation trait ───────────────────────────────────────────────

    #[tokio::test]
    async fn operation_trait_mock_execute() {
        struct MockOp;

        impl Operation for MockOp {
            fn execute(
                &mut self,
                _input: ExecuteArguments,
                _control: ExecutionControl,
            ) -> DiagResult<ExecutionHandle> {
                Ok(ExecutionHandle::from_closure(|| {
                    Ok(DiagnosticReply {
                        message_payload: Some(ReplyMessagePayload::from_string("mock".to_string())),
                        additional_attrs: None,
                    })
                }))
            }
        }

        struct NoOpControl;

        impl ExecutionControlApi for NoOpControl {
            fn next_exec_event(&mut self) -> BoxFuture<'_, ExecutionEvent> {
                Box::pin(async { ExecutionEvent::new(ExecutionEventKind::ControlGone) })
            }
        }

        let mut op = MockOp;
        let args = ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::UTF8,
            user_parameters: None,
            additional_attrs: None,
            proximity_response: None,
        };
        let ctrl = ExecutionControl::new(NoOpControl, "exec-1".to_string());
        let handle = op.execute(args, ctrl).unwrap();
        let result = handle.future.await;
        assert_eq!(
            result.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string("mock".to_string()))
        );
    }
}
