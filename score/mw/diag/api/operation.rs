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
            last_executed_capability: match event_kind {
                // cf. ISO 17978-3:2025 Section 7.14.5, Table 178
                ExecutionEventKind::HandleCustomCapability(capability) => capability,
                ExecutionEventKind::ReportStatus => panic!("illegitimate capability"),
                ExecutionEventKind::Interrupt => "freeze".to_string(),
                ExecutionEventKind::Reset => "reset".to_string(),
                ExecutionEventKind::Stop => "stop".to_string(),
                _ => "execute".to_string(),
            },
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
