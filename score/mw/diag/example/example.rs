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

use diag_api::sovd::data_resource::*;
use diag_api::sovd::operation::*;
use diag_api::uds::ReadDataByIdentifier;
use diag_api::Result as DiagResult;
use diag_api::*;

use diag_runtime::*;

use tokio::sync::mpsc;

/*************************************/
/* user implementation of a UDS RDBI */
/*************************************/

struct MyReadDataByIdentifier {}

impl ReadDataByIdentifier for MyReadDataByIdentifier {
    fn read(&self) -> DiagResult<Vec<u8>> {
        Ok(vec![0xDE, 0xAD, 0xBE, 0xEF])
    }
}

/******************************************/
/* user implementation of a data resource */
/******************************************/

struct MyDataResource {
    value: String,
}

impl DataResource for MyDataResource {
    fn read(&self, input: ReadValueArgs) -> ReadValueHandle {
        assert_eq!(input.reply_encoding, ReplyMessageEncoding::UTF8);
        ReadValueHandle::ready(ReadValueReply {
            data: ReplyMessagePayload::UTF8(self.value.clone()),
            errors: None,
        })
    }
}

/*************************************/
/* user implementation of operations */
/*************************************/

struct MySyncOperation {}

impl Operation for MySyncOperation {
    fn execute(
        &mut self,
        input: ExecuteArguments,
        _control: ExecutionControl,
    ) -> DiagResult<ExecutionHandle> {
        assert_eq!(input.reply_encoding, ReplyMessageEncoding::UTF8);
        Ok(ExecutionHandle::from_closure(move || {
            println!("Sync operation execution got initiated ...");
            ExecutionResult::Ok(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::UTF8(
                    "This was a synchronous SOVD operation!".to_string(),
                )),
                additional_attrs: None,
            })
        }))
    }
}

struct MyAsyncOperation {}

impl MyAsyncOperation {
    async fn user_code(
        _input: ExecuteArguments,
        mut notification: mpsc::Receiver<()>,
    ) -> ExecutionResult {
        println!("Async operation execution got initiated ...");

        /* some complex long-running business logic here */

        notification.recv().await;

        println!("Async operation's execution finished successfully!");

        ExecutionResult::Ok(DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::UTF8(
                "This was an asynchronous SOVD operation!".to_string(),
            )),
            additional_attrs: None,
        })
    }

    async fn exec_control(mut control: ExecutionControl, notifier: mpsc::Sender<()>) {
        let mut exec_status = ExecutionStatus::Scheduled;
        loop {
            let exec_event = control.next_exec_event().await;
            match exec_event.kind {
                ExecutionEventKind::HandleCustomCapability(custom_capability) => {
                    println!("Async operation's execution received custom capability event!");
                    assert_eq!("An OEM specific capability", custom_capability);
                    exec_status = ExecutionStatus::UnsupportedCapability(custom_capability);
                }

                ExecutionEventKind::ReportStatus => {
                    println!("Async operation's execution received report status event!");
                    exec_event
                        .status_reporter
                        .put(exec_status.clone(), ExecutionStatusDetails::default());
                    match exec_status {
                        ExecutionStatus::Scheduled => {
                            exec_status = ExecutionStatus::Running;
                        }
                        _other => {
                            exec_status = ExecutionStatus::Running;
                            notifier
                                .send(())
                                .await
                                .expect("Signalling user code failed!") // wakeup above business logic
                        }
                    }
                }

                ExecutionEventKind::ControlGone => {
                    println!("Async operation's execution control channel is gone, terminating execution!");
                    break;
                }

                ExecutionEventKind::Stop => {
                    println!("Async operation execution received stop event!");
                    exec_event
                        .status_reporter
                        .put(ExecutionStatus::Stopped, ExecutionStatusDetails::default());
                    break;
                }

                _other => panic!("Unsupported ExecutionEventKind!"),
            }
        }
    }
}

impl Operation for MyAsyncOperation {
    fn execute(
        &mut self,
        input: ExecuteArguments,
        control: ExecutionControl,
    ) -> DiagResult<ExecutionHandle> {
        let (notifier, notification) = mpsc::channel(10);
        Ok(ExecutionHandle::from_future(async move {
            tokio::select! {
               result_data = Self::user_code(input, notification) => result_data,
               _ = Self::exec_control(control, notifier) => ExecutionResult::Err(Error::from_error(diag_api::sovd::GenericError::from_code(
                   diag_api::sovd::ErrorCode::ErrorResponse,
                   "execution got stopped".to_string(),
               ))),
            }
        }))
    }
}

/**********************/
/* execute operations */
/**********************/

#[cfg(test)]
mod tests {
    use super::*;

    const ENTITY_ID: &str = "test_entity";
    const SYNC_OP_ID: &str = "my_sync_op";
    const ASYNC_OP_ID: &str = "my_async_op";
    const DATA_RESOURCE_ID: &str = "my_data_resource";
    const UDS_DATA_RESOURCE_ID: &str = "uds_data_resource";

    fn setup_runtime() -> Runtime {
        let runtime = Runtime::new();

        let entity = runtime.get_or_create_entity(ENTITY_ID.to_string());

        entity.register_data_resource(
            MyDataResource {
                value: "This is an SOVD data resource value.".to_string(),
            },
            DATA_RESOURCE_ID.to_string(),
            DataResourceMetadata {
                id: DATA_RESOURCE_ID.to_string(),
                name: "My Data Resource".to_string(),
                translation_id: None,
                read_only: false,
                category: DataCategory::CurrentData,
                groups: None,
            },
        );
        entity.register_data_resource(
            diag_api::uds::DataResourceAdapter::new().with_rdbi(MyReadDataByIdentifier {}),
            UDS_DATA_RESOURCE_ID.to_string(),
            DataResourceMetadata {
                id: UDS_DATA_RESOURCE_ID.to_string(),
                name: "My UDS Data Resource".to_string(),
                translation_id: None,
                read_only: true,
                category: DataCategory::StoredData,
                groups: None,
            },
        );

        entity.register_operation(
            MySyncOperation {},
            SYNC_OP_ID.to_string(),
            OperationMetadata {
                proximity_proof_required: false,
                synchronous_execution: true,
                exclusive_execution: false,
                supported_modes: None,
            },
        );
        entity.register_operation(
            MyAsyncOperation {},
            ASYNC_OP_ID.to_string(),
            OperationMetadata {
                proximity_proof_required: false,
                synchronous_execution: false,
                exclusive_execution: false,
                supported_modes: None,
            },
        );

        runtime
    }

    //
    // list registered data resources and verify that the expected metadata as well as content is returned
    //
    #[tokio::test]
    async fn test_data_resource_read() {
        let runtime = setup_runtime();
        let runtime_join_handle = tokio::spawn(runtime.run());

        match runtime
            .send(SOVDMessage::ListDataResources(ENTITY_ID.to_string()))
            .await
        {
            SOVDReply::ListDataResources(Ok(resources)) => {
                assert_eq!(resources.len(), 2);
                assert_eq!(resources[0].id, DATA_RESOURCE_ID);
                assert_eq!(resources[0].name, "My Data Resource");
                assert_eq!(resources[0].category, DataCategory::CurrentData);
                assert_eq!(resources[1].id, UDS_DATA_RESOURCE_ID);
                assert_eq!(resources[1].name, "My UDS Data Resource");
                assert_eq!(resources[1].category, DataCategory::StoredData);
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        // read the data resource's value and verify it matches the predefined value
        match runtime
            .send(SOVDMessage::ReadDataResource(
                ENTITY_ID.to_string(),
                DATA_RESOURCE_ID.to_string(),
                ReadValueArgs::new(ReplyMessageEncoding::UTF8),
            ))
            .await
        {
            SOVDReply::ReadDataResource(Ok(read_value)) => {
                assert_eq!(
                    read_value.data,
                    ReplyMessagePayload::UTF8("This is an SOVD data resource value.".to_string())
                );
                assert!(read_value.errors.is_none());
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        runtime.shutdown().await;
        let _ = runtime_join_handle.await;
    }

    //
    // verify that a UDS RDBI data resource can be read successfully and that writing to it gets rejected
    //
    #[tokio::test]
    async fn test_data_resource_uds_rdbi() {
        let runtime = setup_runtime();
        let runtime_join_handle = tokio::spawn(runtime.run());

        // test successful UDS read
        match runtime
            .send(SOVDMessage::ReadDataResource(
                ENTITY_ID.to_string(),
                UDS_DATA_RESOURCE_ID.to_string(),
                ReadValueArgs::new(ReplyMessageEncoding::Binary),
            ))
            .await
        {
            SOVDReply::ReadDataResource(Ok(read_value)) => {
                assert_eq!(
                    read_value.data,
                    ReplyMessagePayload::Binary(vec![0xDE, 0xAD, 0xBE, 0xEF])
                );
                assert!(read_value.errors.is_none());
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        // verify error upon write (RDBI does not support write)
        let mut rdbi_resource =
            diag_api::uds::DataResourceAdapter::new().with_rdbi(MyReadDataByIdentifier {});
        let write_handle = rdbi_resource.write(WriteValueArgs {
            user_data_signature: None,
            user_data: None,
            additional_attrs: None,
        });
        let err = match write_handle {
            WriteValueHandle::Ready(result) => result.unwrap_err(),
            WriteValueHandle::Pending(_) => panic!("expected Ready, got Pending"),
        };
        assert_eq!(
            err.error.as_ref().unwrap().sovd_error,
            "precondition-not-fulfilled"
        );

        runtime.shutdown().await;
        let _ = runtime_join_handle.await;
    }

    //
    // trigger execution of a simple sync SOVD operation via the runtime
    //
    #[tokio::test]
    async fn test_execution_of_synchronous_operation() {
        println!();

        let runtime = setup_runtime();
        let runtime_join_handle = tokio::spawn(runtime.run());

        let exec_id = match runtime
            .send(SOVDMessage::ExecuteOperation((
                ENTITY_ID.to_string(),
                SYNC_OP_ID.to_string(),
                None,
            )))
            .await
        {
            SOVDReply::ExecuteOperation(Ok(id)) => id,
            other => panic!("Unexpected reply: {:?}", other),
        };
        println!("Sync operation executed, execution id: {}", exec_id);

        // check the execution result
        match runtime
            .send(SOVDMessage::GetOperationExecutionResult((
                ENTITY_ID.to_string(),
                SYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::GetOperationExecutionResult(Ok(exec_result)) => {
                assert_eq!(
                    exec_result,
                    ExecutionResult::Ok(DiagnosticReply {
                        message_payload: Some(ReplyMessagePayload::UTF8(
                            "This was a synchronous SOVD operation!".to_string(),
                        )),
                        additional_attrs: None,
                    })
                );
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        runtime.shutdown().await;
        let _ = runtime_join_handle.await;

        println!("DONE");
    }

    //
    // trigger execution of an async SOVD operation via the runtime, then stop it
    //
    #[tokio::test]
    async fn test_execution_and_stop_of_asynchronous_operation() {
        println!();

        let runtime = setup_runtime();
        let runtime_join_handle = tokio::spawn(runtime.run());

        let exec_id = match runtime
            .send(SOVDMessage::ExecuteOperation((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                None,
            )))
            .await
        {
            SOVDReply::ExecuteOperation(Ok(id)) => id,
            _other => panic!("Unexpected reply!"),
        };
        println!("Async operation executed, execution id: {}", exec_id);

        // stop the execution
        match runtime
            .send(SOVDMessage::StopOperationExecution((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::StopOperationExecution(Ok(())) => {
                println!("Async operation execution stopped successfully!");
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        // allow the spawned task to complete after stop
        tokio::task::yield_now().await;

        // check the execution result
        match runtime
            .send(SOVDMessage::GetOperationExecutionResult((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::GetOperationExecutionResult(Ok(exec_result)) => {
                let err = exec_result.expect_err("expected an error result after stop");
                assert_eq!(
                    err.code,
                    ErrorCode::SOVD(diag_api::sovd::GenericError::from_code(
                        diag_api::sovd::ErrorCode::ErrorResponse,
                        "execution got stopped".to_string(),
                    ))
                );
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        runtime.shutdown().await;
        let _ = runtime_join_handle.await;

        println!("DONE");
    }

    //
    // trigger execution of an async SOVD operation with a timeout that expires
    //
    #[tokio::test]
    async fn test_execution_timeout_of_asynchronous_operation() {
        println!();

        let runtime = setup_runtime();
        let runtime_join_handle = tokio::spawn(runtime.run());

        // execute with a 3-second timeout
        let exec_id = match runtime
            .send(SOVDMessage::ExecuteOperation((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                Some(std::time::Duration::from_secs(3)),
            )))
            .await
        {
            SOVDReply::ExecuteOperation(Ok(id)) => id,
            other => panic!("Unexpected reply: {:?}", other),
        };
        println!(
            "Async operation executed with 3s timeout, execution id: {}",
            exec_id
        );

        // wait for the timeout to expire (+ a small margin)
        tokio::time::sleep(std::time::Duration::from_millis(3250)).await;

        // check the execution result — should be `Stopped` due to timeout
        match runtime
            .send(SOVDMessage::GetOperationExecutionResult((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::GetOperationExecutionResult(Ok(exec_result)) => {
                let err = exec_result.expect_err("expected an error result after timeout");
                assert_eq!(
                    err.code,
                    ErrorCode::SOVD(diag_api::sovd::GenericError::from_code(
                        diag_api::sovd::ErrorCode::ErrorResponse,
                        "execution got stopped due to timeout".to_string(),
                    ))
                );
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        runtime.shutdown().await;
        let _ = runtime_join_handle.await;

        println!("DONE");
    }

    //
    // trigger execution of an async SOVD operation via the runtime
    //
    #[tokio::test]
    async fn test_execution_and_query_of_asynchronous_operation() {
        println!();

        let runtime = setup_runtime();
        let runtime_join_handle = tokio::spawn(runtime.run());

        let exec_id = match runtime
            .send(SOVDMessage::ExecuteOperation((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                None,
            )))
            .await
        {
            SOVDReply::ExecuteOperation(Ok(id)) => id,
            _other => panic!("Unexpected reply!"),
        };
        println!("Async operation executed, execution id: {}", exec_id);

        // request current status of the execution (should be `Scheduled`)
        match runtime
            .send(SOVDMessage::GetOperationExecutionStatus((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::GetOperationExecutionStatus(Ok(status)) => {
                assert_eq!(status, ExecutionStatus::Scheduled);
                println!("Execution status: {:?}", status);
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        // request processing of a custom capability
        match runtime
            .send(SOVDMessage::ExecuteOperationCapability((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
                "An OEM specific capability".to_string(),
            )))
            .await
        {
            SOVDReply::ExecuteOperationCapability(Ok(())) => {
                println!("Custom capability processed!");
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        // request current status (should result in UnsupportedCapability, which also signals user_code)
        match runtime
            .send(SOVDMessage::GetOperationExecutionStatus((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::GetOperationExecutionStatus(Ok(status)) => {
                assert_eq!(
                    status,
                    ExecutionStatus::UnsupportedCapability(
                        "An OEM specific capability".to_string()
                    )
                );
                println!("Execution status: {:?}", status);
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        // request current status once more (should be `Running` now)
        match runtime
            .send(SOVDMessage::GetOperationExecutionStatus((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::GetOperationExecutionStatus(Ok(status)) => {
                assert_eq!(status, ExecutionStatus::Running);
                println!("Execution status: {:?}", status);
            }
            // execution may have already finished
            SOVDReply::GetOperationExecutionStatus(Err(_)) => {
                println!("Execution already finished");
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        // allow the spawned task to complete
        tokio::task::yield_now().await;

        // check the execution result
        match runtime
            .send(SOVDMessage::GetOperationExecutionResult((
                ENTITY_ID.to_string(),
                ASYNC_OP_ID.to_string(),
                exec_id.clone(),
            )))
            .await
        {
            SOVDReply::GetOperationExecutionResult(Ok(exec_result)) => {
                assert_eq!(
                    exec_result,
                    ExecutionResult::Ok(DiagnosticReply {
                        message_payload: Some(ReplyMessagePayload::UTF8(
                            "This was an asynchronous SOVD operation!".to_string(),
                        )),
                        additional_attrs: None,
                    })
                );
            }
            other => panic!("Unexpected reply: {:?}", other),
        }

        runtime.shutdown().await;
        let _ = runtime_join_handle.await;

        println!("DONE");
    }
}
