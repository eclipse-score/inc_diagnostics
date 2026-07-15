// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

use common::DiagnosticReply;
use common::Result as DiagResult;
use futures::StreamExt;
use operation::{
    ExecuteArguments, ExecutionControl, ExecutionEventKind, ExecutionHandle, ExecutionId,
    ExecutionStatus, ExecutionStatusDetails,
};

use std::sync::{Arc, Mutex};

/// Simplified operation trait. Such a simple operation can be configured as synchronous
/// or asynchronous operation but is never allowed to get executed concurrently.
pub trait SimpleOperation {
    /// Start the operation with the given input arguments.
    /// Returns a handle whose future resolves to the operation result.
    fn start(&mut self, input: ExecuteArguments) -> DiagResult<ExecutionHandle>;

    /// Stop the operation, optionally with the given input arguments
    /// Returns an optional diagnostic reply.
    fn stop(&mut self, input: Option<ExecuteArguments>) -> DiagResult<Option<DiagnosticReply>>;

    /// Optionally provide the current completion percentage
    fn completion_percentage(&self) -> Option<u8> {
        None
    }
}

/// Adapter that wraps a `SimpleOperation` but implements the full `Operation` trait,
/// bridging the simplified interface to the runtime's execution control model.
pub struct SimpleOperationAdapter {
    data: Arc<Mutex<SimpleOperationAdapterData>>,
}

struct SimpleOperationAdapterData {
    op_instance: Box<dyn SimpleOperation + Send>,
    active_exec_id: Option<ExecutionId>,
}

impl SimpleOperationAdapter {
    #[must_use]
    pub fn new(instance: impl SimpleOperation + Send + 'static) -> Self {
        Self {
            data: Arc::new(Mutex::new(SimpleOperationAdapterData {
                op_instance: Box::new(instance),
                active_exec_id: None,
            })),
        }
    }
}

impl operation::Operation for SimpleOperationAdapter {
    fn execute(
        &mut self,
        input: ExecuteArguments,
        mut exec_control: Box<dyn ExecutionControl>,
    ) -> DiagResult<ExecutionHandle> {
        let mut data = self
            .data
            .lock()
            .map_err(|_| common::Error::mutex_poisoned())?;
        if data.active_exec_id.is_some() {
            return Err(common::Error::from_error(
                common::sovd::GenericError::from_code(
                    common::sovd::ErrorCode::PreconditionNotFulfilled,
                    "operation is already executing".to_string(),
                ),
            ));
        }
        data.active_exec_id = Some(exec_control.exec_id().clone());

        let exec_handle = data.op_instance.start(input)?;

        let op_adapter_data = Arc::clone(&self.data);

        let exec_control_future = async move {
            let mut last_exec_event_kind = ExecutionEventKind::Resume;
            let mut exec_errors: Vec<common::Error> = Vec::new();
            let mut exec_status = ExecutionStatus::Running;

            loop {
                let Some(exec_event) = exec_control.next().await else {
                    break;
                };
                match exec_event.kind {
                    ExecutionEventKind::ControlGone => break,

                    ExecutionEventKind::ReportStatus => {
                        let mut details = ExecutionStatusDetails::new(last_exec_event_kind.clone());
                        if let Some(percentage) = op_adapter_data
                            .lock()
                            .ok()
                            .and_then(|data| data.op_instance.completion_percentage())
                        {
                            details = details.with_completion_percentage(percentage);
                        }
                        if !exec_errors.is_empty() {
                            details = details.with_exec_errors(exec_errors.clone());
                        }
                        exec_event.status_reporter.put(exec_status.clone(), details);
                    }

                    _ => {
                        last_exec_event_kind = exec_event.kind;
                        match last_exec_event_kind {
                            ExecutionEventKind::Stop => {
                                match op_adapter_data
                                    .lock()
                                    .map_err(|_| common::Error::mutex_poisoned())
                                    .and_then(|mut data| data.op_instance.stop(exec_event.args))
                                {
                                    Ok(Some(result)) => {
                                        exec_status = ExecutionStatus::Stopped;
                                        exec_event.status_reporter.put(
                                            exec_status.clone(),
                                            ExecutionStatusDetails::default()
                                                .with_reply_data(result),
                                        );
                                    }
                                    Ok(None) => {
                                        exec_status = ExecutionStatus::Stopped;
                                    }
                                    Err(err) => {
                                        exec_errors.push(err);
                                        exec_event.status_reporter.put(
                                            exec_status.clone(),
                                            ExecutionStatusDetails::default()
                                                .with_exec_errors(exec_errors.clone()),
                                        );
                                    }
                                }
                            }

                            _ => {
                                exec_status = ExecutionStatus::UnsupportedCapability(
                                    last_exec_event_kind.to_string(),
                                )
                            }
                        }
                    }
                }
            }

            if exec_status == ExecutionStatus::Stopped {
                return Err(common::Error::from_error(
                    common::sovd::GenericError::from_code(
                        common::sovd::ErrorCode::ErrorResponse,
                        "operation was stopped".to_string(),
                    ),
                ));
            }

            // Control ended normally; yield to let the operation future provide the result.
            Ok(DiagnosticReply::default())
        };

        Ok(ExecutionHandle {
            future: Box::pin(async move {
                tokio::select! {
                    op_result = exec_handle.future => op_result,
                    ctrl_result = exec_control_future => ctrl_result,
                }
            }),
            reply: exec_handle.reply,
        })
    }
}

/*******************/
/* Unit Tests      */
/*******************/

#[cfg(test)]
mod tests {
    use super::*;
    use common::ReplyMessagePayload;
    use operation::{ExecutionControl, ExecutionEvent, ExecutionEventKind, ExecutionHandle};
    use std::pin::Pin;
    use std::sync::{Arc, Mutex};
    use std::task::{Context, Poll};

    // ── Mock infrastructure ──────────────────────────────────────────

    /// Configurable mock implementation of `SimpleOperation`.
    struct MockSimpleOp {
        start_fn: Box<dyn FnMut(ExecuteArguments) -> DiagResult<ExecutionHandle> + Send>,
        stop_fn:
            Box<dyn FnMut(Option<ExecuteArguments>) -> DiagResult<Option<DiagnosticReply>> + Send>,
        completion_pct: Option<u8>,
    }

    impl MockSimpleOp {
        fn new<S, T>(start_fn: S, stop_fn: T) -> Self
        where
            S: FnMut(ExecuteArguments) -> DiagResult<ExecutionHandle> + Send + 'static,
            T: FnMut(Option<ExecuteArguments>) -> DiagResult<Option<DiagnosticReply>>
                + Send
                + 'static,
        {
            Self {
                start_fn: Box::new(start_fn),
                stop_fn: Box::new(stop_fn),
                completion_pct: None,
            }
        }

        fn with_completion_percentage(mut self, pct: u8) -> Self {
            self.completion_pct = Some(pct);
            self
        }
    }

    impl SimpleOperation for MockSimpleOp {
        fn start(&mut self, input: ExecuteArguments) -> DiagResult<ExecutionHandle> {
            (self.start_fn)(input)
        }

        fn stop(&mut self, input: Option<ExecuteArguments>) -> DiagResult<Option<DiagnosticReply>> {
            (self.stop_fn)(input)
        }

        fn completion_percentage(&self) -> Option<u8> {
            self.completion_pct
        }
    }

    /// Mock `ExecutionControl` that returns a configurable sequence of events.
    /// Alternates between `Pending` and `Ready` to simulate async delivery.
    struct MockExecControl {
        events: Vec<ExecutionEvent>,
        exec_id: ExecutionId,
        yield_next: bool,
    }

    impl MockExecControl {
        fn from_kinds(kinds: Vec<ExecutionEventKind>, exec_id: String) -> Self {
            Self {
                events: kinds.into_iter().map(|k| ExecutionEvent::new(k)).collect(),
                exec_id,
                yield_next: true,
            }
        }

        fn from_events(events: Vec<ExecutionEvent>, exec_id: String) -> Self {
            Self {
                events,
                exec_id,
                yield_next: true,
            }
        }
    }

    impl futures::Stream for MockExecControl {
        type Item = ExecutionEvent;
        fn poll_next(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
            let this = self.get_mut();
            if this.yield_next {
                this.yield_next = false;
                cx.waker().wake_by_ref();
                Poll::Pending
            } else {
                this.yield_next = true;
                if this.events.is_empty() {
                    Poll::Ready(None)
                } else {
                    let event = this.events.remove(0);
                    Poll::Ready(Some(event))
                }
            }
        }
    }

    impl ExecutionControl for MockExecControl {
        fn exec_id(&self) -> &ExecutionId {
            &self.exec_id
        }
    }

    /// Helper: create default `ExecuteArguments` for tests.
    fn default_exec_args() -> ExecuteArguments {
        ExecuteArguments {
            reply_encoding: common::ReplyMessageEncoding::UTF8,
            user_parameters: None,
            additional_attrs: None,
            proximity_response: None,
        }
    }

    /// Helper: create an `ExecutionControl` from a list of event kinds.
    fn exec_control_from_kinds(
        kinds: Vec<ExecutionEventKind>,
        exec_id: &str,
    ) -> Box<dyn ExecutionControl> {
        Box::new(MockExecControl::from_kinds(kinds, exec_id.to_string()))
    }

    /// Helper: create an `ExecutionControl` from a list of events.
    fn exec_control_from_events(
        events: Vec<ExecutionEvent>,
        exec_id: &str,
    ) -> Box<dyn ExecutionControl> {
        Box::new(MockExecControl::from_events(events, exec_id.to_string()))
    }

    // ── Test Adapter construction ────────────────────────────────────

    #[test]
    fn adapter_from_creates_instance() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_closure(|| {
                    Ok(DiagnosticReply::default())
                }))
            },
            |_| Ok(None),
        );
        let _adapter = SimpleOperationAdapter::new(op);
    }

    // ── Test Adapter execute — happy path ────────────────────────────

    #[tokio::test]
    async fn adapter_execute_resolves_op_future() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_closure(|| {
                    Ok(DiagnosticReply {
                        message_payload: Some(ReplyMessagePayload::from_string("done".to_string())),
                        additional_attrs: None,
                    })
                }))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);
        let ctrl = exec_control_from_kinds(vec![ExecutionEventKind::ControlGone], "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let result = handle.future.await.unwrap();
        assert_eq!(
            result.message_payload,
            Some(ReplyMessagePayload::from_string("done".to_string()))
        );
    }

    #[tokio::test]
    async fn adapter_execute_with_initial_reply() {
        let initial_reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string("initial".to_string())),
            additional_attrs: None,
        };
        let final_reply = DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string("final".to_string())),
            additional_attrs: None,
        };
        let op = MockSimpleOp::new(
            move |_| {
                Ok(ExecutionHandle::from_closure({
                    let reply = final_reply.clone();
                    move || Ok(reply)
                })
                .with_reply(initial_reply.clone()))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);
        let ctrl = exec_control_from_kinds(vec![ExecutionEventKind::ControlGone], "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        assert!(handle.reply.is_some());
        assert_eq!(
            handle.reply.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string("initial".to_string()))
        );
        let result = handle.future.await.unwrap();
        assert_eq!(
            result.message_payload,
            Some(ReplyMessagePayload::from_string("final".to_string()))
        );
    }

    // ── Test Adapter execute — ReportStatus event handling ───────────

    #[tokio::test]
    async fn adapter_reports_status_while_running() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_status: Arc<Mutex<Option<ExecutionStatus>>> = Arc::new(Mutex::new(None));
        let captured_details: Arc<Mutex<Option<ExecutionStatusDetails>>> =
            Arc::new(Mutex::new(None));
        let cs = captured_status.clone();
        let cd = captured_details.clone();

        let report_event = ExecutionEvent::new(ExecutionEventKind::ReportStatus)
            .with_status_reporter(move |status, details| {
                *cs.lock().unwrap() = Some(status);
                *cd.lock().unwrap() = Some(details);
            });
        let events = vec![
            report_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let status = captured_status
            .lock()
            .unwrap()
            .take()
            .expect("no status was reported");
        assert!(matches!(status, ExecutionStatus::Running));

        let details = captured_details
            .lock()
            .unwrap()
            .take()
            .expect("no details were reported");
        assert!(details.completion_percentage.is_none());
        assert!(details.event_result.is_none());
        assert!(details.exec_errors.is_none());
    }

    #[tokio::test]
    async fn adapter_reports_status_with_completion_percentage() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        )
        .with_completion_percentage(50);
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_details: Arc<Mutex<Option<ExecutionStatusDetails>>> =
            Arc::new(Mutex::new(None));
        let cd = captured_details.clone();

        let report_event = ExecutionEvent::new(ExecutionEventKind::ReportStatus)
            .with_status_reporter(move |_status, details| {
                *cd.lock().unwrap() = Some(details);
            });
        let events = vec![
            report_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let details = captured_details
            .lock()
            .unwrap()
            .take()
            .expect("no details were reported");
        assert_eq!(details.completion_percentage, Some(50));
    }

    // ── Test Adapter execute — Stop event handling ───────────────────

    #[tokio::test]
    async fn adapter_stop_returns_reply() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| {
                Ok(Some(DiagnosticReply {
                    message_payload: Some(ReplyMessagePayload::from_string(
                        "stopped successfully".to_string(),
                    )),
                    additional_attrs: None,
                }))
            },
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_status: Arc<Mutex<Option<ExecutionStatus>>> = Arc::new(Mutex::new(None));
        let captured_details: Arc<Mutex<Option<ExecutionStatusDetails>>> =
            Arc::new(Mutex::new(None));
        let cs = captured_status.clone();
        let cd = captured_details.clone();

        let stop_event = ExecutionEvent::new(ExecutionEventKind::Stop).with_status_reporter(
            move |status, details| {
                *cs.lock().unwrap() = Some(status);
                *cd.lock().unwrap() = Some(details);
            },
        );
        let events = vec![
            stop_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let status = captured_status
            .lock()
            .unwrap()
            .take()
            .expect("no status was reported");
        assert_eq!(status, ExecutionStatus::Stopped);

        let details = captured_details
            .lock()
            .unwrap()
            .take()
            .expect("no details were reported");
        assert!(details.event_result.is_some());
        assert_eq!(
            details.event_result.unwrap().message_payload,
            Some(ReplyMessagePayload::from_string(
                "stopped successfully".to_string()
            ))
        );
        assert!(details.exec_errors.is_none());
    }

    #[tokio::test]
    async fn adapter_stop_returns_none() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let events = vec![
            ExecutionEvent::new(ExecutionEventKind::Stop),
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let result = handle.future.await;
        assert!(result.is_err());
        match result.unwrap_err().code {
            common::ErrorCode::SOVD(inner) => {
                assert_eq!(inner.sovd_error, "error-response");
                assert_eq!(inner.message_text, "operation was stopped");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    #[tokio::test]
    async fn adapter_stop_returns_error() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| {
                Err(common::Error::from_error(
                    common::sovd::GenericError::from_code(
                        common::sovd::ErrorCode::ErrorResponse,
                        "stop failed".to_string(),
                    ),
                ))
            },
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_details: Arc<Mutex<Option<ExecutionStatusDetails>>> =
            Arc::new(Mutex::new(None));
        let cd = captured_details.clone();

        let stop_event = ExecutionEvent::new(ExecutionEventKind::Stop).with_status_reporter(
            move |_status, details| {
                *cd.lock().unwrap() = Some(details);
            },
        );
        let events = vec![
            stop_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let details = captured_details
            .lock()
            .unwrap()
            .take()
            .expect("no details were reported");
        assert!(details.exec_errors.is_some());
        assert!(details.event_result.is_none());
        let exec_errors = details.exec_errors.unwrap();
        assert_eq!(exec_errors.len(), 1);
        match &exec_errors[0].code {
            common::ErrorCode::SOVD(inner) => {
                assert_eq!(inner.sovd_error, "error-response");
                assert_eq!(inner.message_text, "stop failed");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    // ── Test Adapter execute — ControlGone event ─────────────────────

    #[tokio::test]
    async fn adapter_control_gone_completes_status() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let events = vec![ExecutionEvent::new(ExecutionEventKind::ControlGone)];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let result = handle.future.await;
        // ControlGone breaks the loop and returns Ok from the control future
        assert!(result.is_ok());
    }

    // ── Test Adapter execute — unsupported events ────────────────────

    #[tokio::test]
    async fn adapter_unsupported_event_kind_interrupt() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_status: Arc<Mutex<Option<ExecutionStatus>>> = Arc::new(Mutex::new(None));
        let cs = captured_status.clone();

        let report_event = ExecutionEvent::new(ExecutionEventKind::ReportStatus)
            .with_status_reporter(move |status, _details| {
                *cs.lock().unwrap() = Some(status);
            });
        let events = vec![
            ExecutionEvent::new(ExecutionEventKind::Interrupt),
            report_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let status = captured_status
            .lock()
            .unwrap()
            .take()
            .expect("no status was reported");
        assert_eq!(
            status,
            ExecutionStatus::UnsupportedCapability("freeze".to_string())
        );
    }

    #[tokio::test]
    async fn adapter_unsupported_event_kind_resume() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_status: Arc<Mutex<Option<ExecutionStatus>>> = Arc::new(Mutex::new(None));
        let cs = captured_status.clone();

        let report_event = ExecutionEvent::new(ExecutionEventKind::ReportStatus)
            .with_status_reporter(move |status, _details| {
                *cs.lock().unwrap() = Some(status);
            });
        let events = vec![
            ExecutionEvent::new(ExecutionEventKind::Resume),
            report_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let status = captured_status
            .lock()
            .unwrap()
            .take()
            .expect("no status was reported");
        assert_eq!(
            status,
            ExecutionStatus::UnsupportedCapability("execute".to_string())
        );
    }

    #[tokio::test]
    async fn adapter_unsupported_event_kind_custom_capability() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_status: Arc<Mutex<Option<ExecutionStatus>>> = Arc::new(Mutex::new(None));
        let cs = captured_status.clone();

        let report_event = ExecutionEvent::new(ExecutionEventKind::ReportStatus)
            .with_status_reporter(move |status, _details| {
                *cs.lock().unwrap() = Some(status);
            });
        let events = vec![
            ExecutionEvent::new(ExecutionEventKind::HandleCustomCapability(
                "custom".to_string(),
            )),
            report_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let status = captured_status
            .lock()
            .unwrap()
            .take()
            .expect("no status was reported");
        assert_eq!(
            status,
            ExecutionStatus::UnsupportedCapability("custom".to_string())
        );
    }

    // ── Test Adapter execute — concurrent execution guard ────────────

    #[tokio::test]
    async fn adapter_rejects_concurrent_execution() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        // First execution — uses a pending future so it never completes.
        let ctrl1 = exec_control_from_kinds(vec![], "exec-id-first");
        let _handle1 =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl1).unwrap();

        // Second execution — should be rejected because active_exec_id is set.
        let ctrl2 =
            exec_control_from_kinds(vec![ExecutionEventKind::ControlGone], "exec-id-second");
        let result = operation::Operation::execute(&mut adapter, default_exec_args(), ctrl2);
        match result {
            Err(err) => match &err.code {
                common::ErrorCode::SOVD(inner) => {
                    assert_eq!(inner.sovd_error, "precondition-not-fulfilled");
                    assert_eq!(inner.message_text, "operation is already executing");
                }
                _ => panic!("expected SOVD error code"),
            },
            Ok(_) => panic!("expected error for concurrent execution"),
        }
    }

    // ── Test Adapter execute — error scenarios ───────────────────────

    #[tokio::test]
    async fn adapter_execute_when_start_fails() {
        let op = MockSimpleOp::new(
            |_| {
                Err(common::Error::from_error(
                    common::sovd::GenericError::from_code(
                        common::sovd::ErrorCode::ErrorResponse,
                        "start failed".to_string(),
                    ),
                ))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let ctrl = exec_control_from_kinds(vec![ExecutionEventKind::ControlGone], "exec-id");
        let result = operation::Operation::execute(&mut adapter, default_exec_args(), ctrl);
        match result {
            Err(err) => match &err.code {
                common::ErrorCode::SOVD(inner) => {
                    assert_eq!(inner.sovd_error, "error-response");
                    assert_eq!(inner.message_text, "start failed");
                }
                _ => panic!("expected SOVD error code"),
            },
            Ok(_) => panic!("expected error when start fails"),
        }
    }

    #[tokio::test]
    async fn adapter_report_status_with_accumulated_errors() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| {
                Err(common::Error::from_error(
                    common::sovd::GenericError::from_code(
                        common::sovd::ErrorCode::ErrorResponse,
                        "stop failed".to_string(),
                    ),
                ))
            },
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let captured_details: Arc<Mutex<Option<ExecutionStatusDetails>>> =
            Arc::new(Mutex::new(None));
        let cd = captured_details.clone();

        // ReportStatus after a failed Stop should show accumulated errors.
        let report_event = ExecutionEvent::new(ExecutionEventKind::ReportStatus)
            .with_status_reporter(move |_status, details| {
                *cd.lock().unwrap() = Some(details);
            });
        let events = vec![
            ExecutionEvent::new(ExecutionEventKind::Stop),
            report_event,
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let _ = handle.future.await;

        let details = captured_details
            .lock()
            .unwrap()
            .take()
            .expect("details were reported");
        assert!(details.exec_errors.is_some());
        let exec_errors = details.exec_errors.unwrap();
        assert_eq!(exec_errors.len(), 1);
        match &exec_errors[0].code {
            common::ErrorCode::SOVD(inner) => {
                assert_eq!(inner.sovd_error, "error-response");
                assert_eq!(inner.message_text, "stop failed");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    #[tokio::test]
    async fn adapter_control_gone_after_stop_preserves_stopped_status() {
        let op = MockSimpleOp::new(
            |_| {
                Ok(ExecutionHandle::from_future(Box::pin(
                    futures::future::pending(),
                )))
            },
            |_| Ok(None),
        );
        let mut adapter = SimpleOperationAdapter::new(op);

        let events = vec![
            ExecutionEvent::new(ExecutionEventKind::Stop),
            ExecutionEvent::new(ExecutionEventKind::ControlGone),
        ];
        let ctrl = exec_control_from_events(events, "exec-id");
        let handle =
            operation::Operation::execute(&mut adapter, default_exec_args(), ctrl).unwrap();
        let result = handle.future.await;
        assert!(result.is_err());
        match result.unwrap_err().code {
            common::ErrorCode::SOVD(inner) => {
                assert_eq!(inner.sovd_error, "error-response");
                assert_eq!(inner.message_text, "operation was stopped");
            }
            _ => panic!("expected SOVD error code"),
        }
    }
}
