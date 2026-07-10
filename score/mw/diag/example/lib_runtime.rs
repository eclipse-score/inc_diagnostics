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

use diag_api::sovd::data_resource::{
    DataResource, DataResourceMetadata, ReadValueArgs, ReadValueHandle, ReadValueReply,
};
use diag_api::sovd::operation::{
    ExecuteArguments, ExecutionControl, ExecutionEvent, ExecutionEventKind, ExecutionResult,
    ExecutionStatus, Operation, OperationMetadata,
};
use diag_api::Error as DiagError;
use diag_api::Result as DiagResult;
use diag_api::*;

use futures::FutureExt;

use indexmap::IndexMap;

use tokio::sync::{mpsc, oneshot};

use std::future::Future;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

pub type EntityId = String;
pub type ExecutionId = String;
pub type OperationId = String;
pub type DataResourceId = String;
pub type ExecutionTimeout = Duration;

static EXECUTION_ID_COUNTER: AtomicU64 = AtomicU64::new(1);

fn issue_new_execution_id() -> ExecutionId {
    EXECUTION_ID_COUNTER
        .fetch_add(1, Ordering::Relaxed)
        .to_string()
}

// this message type is for demonstration purposes only
#[derive(Debug)]
pub enum SOVDMessage {
    ListDataResources(EntityId),
    ReadDataResource(EntityId, DataResourceId, ReadValueArgs),
    ListOperations(EntityId),
    GetOperationMetadata((EntityId, OperationId)),
    ExecuteOperation((EntityId, OperationId, Option<ExecutionTimeout>)),
    ExecuteOperationCapability((EntityId, OperationId, ExecutionId, String)),
    GetOperationExecutionStatus((EntityId, OperationId, ExecutionId)),
    GetOperationExecutionResult((EntityId, OperationId, ExecutionId)),
    StopOperationExecution((EntityId, OperationId, ExecutionId)),
    RemoveOperationExecution((EntityId, OperationId, ExecutionId)),
}

// this message type is for demonstration purposes only
#[derive(Debug)]
pub enum SOVDReply {
    ListDataResources(DiagResult<Vec<DataResourceMetadata>>),
    ReadDataResource(DiagResult<ReadValueReply>),
    ListOperations(DiagResult<Vec<OperationMetadata>>),
    GetOperationMetadata(DiagResult<OperationMetadata>),
    ExecuteOperation(DiagResult<ExecutionId>),
    ExecuteOperationCapability(DiagResult<()>),
    GetOperationExecutionStatus(DiagResult<ExecutionStatus>),
    GetOperationExecutionResult(DiagResult<ExecutionResult>),
    StopOperationExecution(DiagResult<()>),
    RemoveOperationExecution(DiagResult<()>),
}

/***********************************/
/* ExecutionControl for operations */
/***********************************/

struct ExecutionControlImpl {
    exec_events: mpsc::Receiver<ExecutionEvent>,
    exec_id: ExecutionId,
}

impl ExecutionControlImpl {
    fn new(exec_events: mpsc::Receiver<ExecutionEvent>, exec_id: ExecutionId) -> Self {
        Self {
            exec_events,
            exec_id,
        }
    }
}

impl futures::Stream for ExecutionControlImpl {
    type Item = ExecutionEvent;
    fn poll_next(
        self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
    ) -> std::task::Poll<Option<Self::Item>> {
        self.get_mut().exec_events.poll_recv(cx)
    }
}

impl ExecutionControl for ExecutionControlImpl {
    fn exec_id(&self) -> &ExecutionId {
        &self.exec_id
    }
}

/**********************/
/* Active executions  */
/**********************/

enum ActiveExecution {
    Completed(ExecutionResult),
    Running {
        exec_control: mpsc::Sender<ExecutionEvent>,
        join_handle: Option<tokio::task::JoinHandle<ExecutionResult>>,
    },
}

impl ActiveExecution {
    fn try_resolve(&mut self) {
        if let ActiveExecution::Running {
            join_handle: Some(handle),
            ..
        } = self
        {
            if handle.is_finished() {
                let old = std::mem::replace(
                    self,
                    ActiveExecution::Completed(Err(DiagError::from_error(
                        sovd::GenericError::from_code(
                            sovd::ErrorCode::SovdServerFailure,
                            "execution result unavailable".to_string(),
                        ),
                    ))),
                );
                if let ActiveExecution::Running {
                    join_handle: Some(handle),
                    ..
                } = old
                {
                    if let Some(Ok(result)) = handle.now_or_never() {
                        *self = ActiveExecution::Completed(result);
                    }
                }
            }
        }
    }

    fn get_exec_control(&self) -> DiagResult<mpsc::Sender<ExecutionEvent>> {
        match self {
            ActiveExecution::Running { exec_control, .. } => Ok(exec_control.clone()),
            ActiveExecution::Completed(_) => {
                Err(DiagError::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "execution is already completed".to_string(),
                )))
            }
        }
    }

    fn get_result(&self) -> DiagResult<ExecutionResult> {
        match self {
            ActiveExecution::Completed(result) => Ok(result.clone()),
            ActiveExecution::Running { .. } => {
                Err(DiagError::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "execution is still running".to_string(),
                )))
            }
        }
    }
}

/***********/
/* Runtime */
/***********/

pub struct Runtime {
    inner: Arc<Mutex<RuntimeImpl>>,
}

impl Runtime {
    pub fn new() -> Self {
        Self {
            inner: Arc::new(Mutex::new(RuntimeImpl::new())),
        }
    }

    pub fn get_or_create_entity(&self, id: EntityId) -> Arc<Entity> {
        self.inner
            .lock()
            .expect("mutex acquisition failed")
            .get_or_create_entity(id)
    }

    pub fn run(&self) -> impl Future<Output = ()> {
        self.inner.lock().expect("mutex acquisition failed").run()
    }

    // This method is for demonstration purposes only!
    pub fn send(&self, message: SOVDMessage) -> impl Future<Output = SOVDReply> {
        self.inner
            .lock()
            .expect("mutex acquisition failed")
            .send(message)
    }

    pub fn shutdown(&self) -> impl Future<Output = ()> {
        self.inner
            .lock()
            .expect("mutex acquisition failed")
            .shutdown()
    }
}

struct RuntimeImpl {
    entities: Arc<Mutex<IndexMap<EntityId, Arc<Entity>>>>,
    sovd_sender: mpsc::Sender<(SOVDMessage, oneshot::Sender<SOVDReply>)>,
    sovd_messages: Option<mpsc::Receiver<(SOVDMessage, oneshot::Sender<SOVDReply>)>>,
    request_shutdown: Option<oneshot::Sender<()>>,
    shutdown_requested: Option<oneshot::Receiver<()>>,
}

impl RuntimeImpl {
    pub fn new() -> Self {
        let (sovd_sender, sovd_receiver) =
            mpsc::channel::<(SOVDMessage, oneshot::Sender<SOVDReply>)>(10);
        let (shutdown_sender, shutdown_receiver) = oneshot::channel();
        RuntimeImpl {
            entities: Arc::new(Mutex::new(IndexMap::<EntityId, Arc<Entity>>::new())),
            sovd_sender,
            sovd_messages: Some(sovd_receiver),
            request_shutdown: Some(shutdown_sender),
            shutdown_requested: Some(shutdown_receiver),
        }
    }

    pub fn run(&mut self) -> impl Future<Output = ()> {
        let shutdown_requested = self
            .shutdown_requested
            .take()
            .expect("Runtime got started already!");
        let mut sovd_messages = self
            .sovd_messages
            .take()
            .expect("Runtime got started already!");
        let entities = self.entities.clone();

        async move {
            let shutdown_request_received = async move {
                let _ = shutdown_requested.await;
            };

            let main_loop = async move {
                loop {
                    match sovd_messages.recv().await {
                        Some((message, reply_sender)) => {
                            let reply = Self::handle(&entities, message).await;
                            let _ = reply_sender.send(reply);
                        }
                        None => break,
                    }
                }
            };

            tokio::select! {
                _ = shutdown_request_received => (),
                _ = main_loop => (),
            }
        }
    }

    async fn handle(
        entities: &Mutex<IndexMap<EntityId, Arc<Entity>>>,
        message: SOVDMessage,
    ) -> SOVDReply {
        match message {
            SOVDMessage::ListDataResources(entity_id) => {
                let reply = Self::select_entity(entities, &entity_id, |entity| {
                    Ok(entity.list_data_resources())
                });
                SOVDReply::ListDataResources(reply)
            }

            SOVDMessage::ReadDataResource(entity_id, data_resource_id, read_value_args) => {
                let reply = Self::select_entity(entities, &entity_id, |entity| {
                    entity.read_data_resource(&data_resource_id, read_value_args)
                });
                SOVDReply::ReadDataResource(reply)
            }

            SOVDMessage::ListOperations(entity_id) => {
                let reply = if entity_id.is_empty() || entity_id == "*" {
                    // Global: list operations from all entities
                    let all_entities: Vec<Arc<Entity>> = entities
                        .lock()
                        .expect("mutex acquisition failed")
                        .values()
                        .cloned()
                        .collect();
                    let mut all_ops = Vec::new();
                    for entity in &all_entities {
                        all_ops.extend(entity.list_operations());
                    }
                    Ok(all_ops)
                } else {
                    // Filtered: list operations for specific entity
                    Self::select_entity(entities, &entity_id, |entity| Ok(entity.list_operations()))
                };
                SOVDReply::ListOperations(reply)
            }

            SOVDMessage::GetOperationMetadata((entity_id, op_id)) => {
                let reply = Self::select_entity(entities, &entity_id, |entity| {
                    entity.get_operation_info(&op_id)
                });
                SOVDReply::GetOperationMetadata(reply)
            }

            SOVDMessage::ExecuteOperation((entity_id, op_id, timeout)) => {
                let reply = Self::select_entity(entities, &entity_id, |entity| {
                    entity.execute_operation(&op_id, timeout)
                });
                SOVDReply::ExecuteOperation(reply)
            }

            SOVDMessage::ExecuteOperationCapability((entity_id, op_id, exec_id, value)) => {
                let exec_control = Self::select_entity(entities, &entity_id, |entity| {
                    entity.get_execution_control(&op_id, &exec_id)
                });
                let reply = match exec_control {
                    Ok(exec_control) => {
                        let event =
                            ExecutionEvent::new(ExecutionEventKind::HandleCustomCapability(value));
                        exec_control.send(event).await.map_err(|_| {
                            DiagError::from_error(sovd::GenericError::from_code(
                                sovd::ErrorCode::ErrorResponse,
                                "Execution is no longer active!".to_string(),
                            ))
                        })
                    }
                    Err(e) => Err(e),
                };
                SOVDReply::ExecuteOperationCapability(reply)
            }

            SOVDMessage::GetOperationExecutionStatus((entity_id, op_id, exec_id)) => {
                let exec_control = Self::select_entity(entities, &entity_id, |entity| {
                    entity.get_execution_control(&op_id, &exec_id)
                });
                let reply = match exec_control {
                    Ok(exec_control) => {
                        let (status_tx, status_rx) = oneshot::channel::<ExecutionStatus>();
                        let event = ExecutionEvent::new(ExecutionEventKind::ReportStatus)
                            .with_status_reporter(move |status: ExecutionStatus, _| {
                                let _ = status_tx.send(status);
                            });
                        if exec_control.send(event).await.is_err() {
                            Err(DiagError::from_error(sovd::GenericError::from_code(
                                sovd::ErrorCode::ErrorResponse,
                                "Execution is no longer active!".to_string(),
                            )))
                        } else {
                            status_rx.await.map_err(|_| {
                                DiagError::from_error(sovd::GenericError::from_code(
                                    sovd::ErrorCode::SovdServerFailure,
                                    "Failed to receive execution status!".to_string(),
                                ))
                            })
                        }
                    }
                    Err(e) => Err(e),
                };
                SOVDReply::GetOperationExecutionStatus(reply)
            }

            SOVDMessage::GetOperationExecutionResult((entity_id, op_id, exec_id)) => {
                let reply = Self::select_entity(entities, &entity_id, |entity| {
                    entity.get_execution_result(&op_id, &exec_id)
                });
                SOVDReply::GetOperationExecutionResult(reply)
            }

            SOVDMessage::StopOperationExecution((entity_id, op_id, exec_id)) => {
                let exec_control = Self::select_entity(entities, &entity_id, |entity| {
                    entity.get_execution_control(&op_id, &exec_id)
                });
                let reply = match exec_control {
                    Ok(exec_control) => exec_control
                        .send(ExecutionEvent::new(ExecutionEventKind::Stop))
                        .await
                        .map_err(|_| {
                            DiagError::from_error(sovd::GenericError::from_code(
                                sovd::ErrorCode::ErrorResponse,
                                "Execution is no longer active!".to_string(),
                            ))
                        }),
                    Err(e) => Err(e),
                };
                SOVDReply::StopOperationExecution(reply)
            }

            SOVDMessage::RemoveOperationExecution((entity_id, op_id, exec_id)) => {
                let reply = Self::select_entity(entities, &entity_id, |entity| {
                    entity.remove_execution(&op_id, &exec_id)
                });
                SOVDReply::RemoveOperationExecution(reply)
            }
        }
    }

    fn select_entity<T>(
        entities: &Mutex<IndexMap<EntityId, Arc<Entity>>>,
        entity_id: &EntityId,
        invoker: impl FnOnce(&Entity) -> DiagResult<T>,
    ) -> DiagResult<T> {
        match entities
            .lock()
            .map_err(|_| DiagError::mutex_poisoned())?
            .get(entity_id)
        {
            Some(entity) => invoker(entity),
            None => Err(DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!("Entity with id '{}' could not be found!", entity_id),
            ))),
        }
    }

    // This method is just for demonstration purposes!
    pub fn send(&mut self, message: SOVDMessage) -> impl Future<Output = SOVDReply> {
        let (reply_sender, reply_receiver) = oneshot::channel::<SOVDReply>();
        let sovd_sender = self.sovd_sender.clone();
        async move {
            sovd_sender
                .send((message, reply_sender))
                .await
                .expect("sending SOVD message failed, channel is gone");
            reply_receiver
                .await
                .expect("receiving SOVD reply failed, channel is gone")
        }
    }

    pub fn get_or_create_entity(&self, id: EntityId) -> Arc<Entity> {
        self.entities
            .lock()
            .expect("mutex acquisition failed")
            .entry(id.clone())
            .or_insert(Arc::new(Entity::new(id)))
            .clone()
    }

    // FIXME: implement unregister_entity(&self, id: EntityId)

    pub fn shutdown(&mut self) -> impl Future<Output = ()> {
        let request_shutdown = self
            .request_shutdown
            .take()
            .expect("runtime got shut down already");
        async move {
            let _ = request_shutdown.send(());
        }
    }
}

pub struct Entity {
    #[allow(unused)] // since this is just example code
    data_resources: Mutex<IndexMap<DataResourceId, DataResourceHolder>>,
    operations: Mutex<IndexMap<OperationId, OperationHolder>>,
    id: EntityId,
}

impl Entity {
    pub fn new(id: EntityId) -> Self {
        Self {
            data_resources: Mutex::new(IndexMap::<DataResourceId, DataResourceHolder>::new()),
            operations: Mutex::new(IndexMap::<OperationId, OperationHolder>::new()),
            id: id,
        }
    }

    pub fn register_data_resource(
        &self,
        resource: impl DataResource + Send + 'static,
        resource_id: DataResourceId,
        resource_metadata: DataResourceMetadata,
    ) {
        match self
            .data_resources
            .lock()
            .expect("mutex acquisition failed")
            .insert(
                resource_id.clone(),
                DataResourceHolder::new(resource, resource_metadata),
            ) {
            Some(_) => panic!(
                "A data resource with id '{}' got already registered!",
                resource_id
            ),
            None => (),
        }
    }

    // FIXME: implement unregister data resource

    pub fn register_operation(
        &self,
        op: impl Operation + Send + 'static,
        op_id: OperationId,
        op_metadata: OperationMetadata,
    ) {
        match self
            .operations
            .lock()
            .expect("mutex acquisition failed")
            .insert(op_id.clone(), OperationHolder::new(op, op_metadata))
        {
            Some(_) => panic!("An operation with id '{}' got already registered!", op_id),
            None => (),
        }
    }

    // FIXME: implement unregister operation

    pub fn list_data_resources(&self) -> Vec<DataResourceMetadata> {
        self.data_resources
            .lock()
            .expect("mutex acquisition failed")
            .values()
            .map(|resource| resource.metadata.clone())
            .collect()
    }

    pub fn read_data_resource(
        &self,
        id: &DataResourceId,
        args: ReadValueArgs,
    ) -> DiagResult<ReadValueReply> {
        let handle = self
            .data_resources
            .lock()
            .map_err(|_| DiagError::mutex_poisoned())?
            .get(id)
            .ok_or_else(|| {
                DiagError::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::ErrorResponse,
                    format!(
                        "Data resource with id '{}' not found in entity '{}'",
                        id, self.id
                    ),
                ))
            })?
            .instance
            .read(args);
        match handle {
            ReadValueHandle::Ready(result) => result,
            ReadValueHandle::Pending(_) => {
                Err(DiagError::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "Async data resource reads are not supported in this synchronous context"
                        .to_string(),
                )))
            }
        }
    }

    pub fn list_operations(&self) -> Vec<OperationMetadata> {
        self.operations
            .lock()
            .expect("mutex acquisition failed")
            .values()
            .map(|operation| operation.info.clone())
            .collect()
    }

    pub fn get_operation_info(&self, op_id: &OperationId) -> DiagResult<OperationMetadata> {
        self.operations
            .lock()
            .map_err(|_| DiagError::mutex_poisoned())?
            .get(op_id)
            .map(|operation| operation.info.clone())
            .ok_or_else(|| {
                DiagError::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::ErrorResponse,
                    format!(
                        "Operation with id '{}' not found in entity '{}'",
                        op_id, self.id
                    ),
                ))
            })
    }

    pub fn execute_operation(
        &self,
        op_id: &OperationId,
        timeout: Option<ExecutionTimeout>,
    ) -> DiagResult<ExecutionId> {
        let mut operations = self
            .operations
            .lock()
            .map_err(|_| DiagError::mutex_poisoned())?;
        let operation = operations.get_mut(op_id).ok_or_else(|| {
            DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!(
                    "Operation with id '{}' not found in entity '{}'",
                    op_id, self.id
                ),
            ))
        })?;

        if operation.info.exclusive_execution && !operation.executions.is_empty() {
            return Err(DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::PreconditionNotFulfilled,
                format!(
                    "Operation with id '{}' requires exclusive invocation and already has an active execution!",
                    op_id
                ),
            )));
        }

        let (exec_event_sender, exec_event_receiver) = mpsc::channel(10);
        let exec_id = issue_new_execution_id();
        let exec_control: Box<dyn ExecutionControl> = Box::new(ExecutionControlImpl::new(
            exec_event_receiver,
            exec_id.clone(),
        ));
        let exec_control_for_timeout = if timeout.is_some() {
            Some(exec_event_sender.clone())
        } else {
            None
        };

        let execution_handle = operation.instance.execute(
            ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::UTF8, // UTF8 is used for demonstration purposes here
                user_parameters: None,
                additional_attrs: None,
                proximity_response: None,
            },
            exec_control,
        )?;

        let exec_future = execution_handle.future;
        let exec_future_with_timeout = async move {
            match timeout {
                Some(duration) => {
                    tokio::select! {
                        result = exec_future => result,
                        _ = tokio::time::sleep(duration) => {
                            let _ = exec_control_for_timeout.expect("unexpected None value").send(ExecutionEvent::new(ExecutionEventKind::Stop)).await;
                            Err(DiagError::from_error(sovd::GenericError::from_code(
                                sovd::ErrorCode::ErrorResponse,
                                "execution got stopped due to timeout".to_string(),
                            )))
                        }
                    }
                }
                None => exec_future.await,
            }
        };

        if operation.info.synchronous_execution {
            operation.executions.insert(
                exec_id.clone(),
                ActiveExecution::Running {
                    exec_control: exec_event_sender,
                    join_handle: None,
                },
            );
            drop(operations); // for unlocking the mutex
            let result = futures::executor::block_on(exec_future_with_timeout);
            let mut operations = self
                .operations
                .lock()
                .map_err(|_| DiagError::mutex_poisoned())?;
            let operation = operations.get_mut(op_id).expect("operation must exist");
            if let Some(execution) = operation.executions.get_mut(&exec_id) {
                *execution = ActiveExecution::Completed(result);
            }
        } else {
            operation.executions.insert(
                exec_id.clone(),
                ActiveExecution::Running {
                    exec_control: exec_event_sender,
                    join_handle: Some(tokio::spawn(exec_future_with_timeout)),
                },
            );
        };

        Ok(exec_id)
    }

    pub fn get_execution_control(
        &self,
        op_id: &OperationId,
        exec_id: &ExecutionId,
    ) -> DiagResult<mpsc::Sender<ExecutionEvent>> {
        let operations = self
            .operations
            .lock()
            .map_err(|_| DiagError::mutex_poisoned())?;
        let operation = operations.get(op_id).ok_or_else(|| {
            DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!(
                    "Operation with id '{}' not found in entity '{}'",
                    op_id, self.id
                ),
            ))
        })?;
        let execution = operation.executions.get(exec_id).ok_or_else(|| {
            DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!(
                    "Execution with id '{}' not found for operation '{}' in entity '{}'",
                    exec_id, op_id, self.id
                ),
            ))
        })?;
        execution.get_exec_control()
    }

    pub fn get_execution_result(
        &self,
        op_id: &OperationId,
        exec_id: &ExecutionId,
    ) -> DiagResult<ExecutionResult> {
        let mut operations = self
            .operations
            .lock()
            .map_err(|_| DiagError::mutex_poisoned())?;
        let operation = operations.get_mut(op_id).ok_or_else(|| {
            DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!(
                    "Operation with id '{}' not found in entity '{}'",
                    op_id, self.id
                ),
            ))
        })?;
        let execution = operation.executions.get_mut(exec_id).ok_or_else(|| {
            DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!(
                    "Execution with id '{}' not found for operation '{}' in entity '{}'",
                    exec_id, op_id, self.id
                ),
            ))
        })?;
        execution.try_resolve();
        execution.get_result()
    }

    pub fn remove_execution(&self, op_id: &OperationId, exec_id: &ExecutionId) -> DiagResult<()> {
        let mut operations = self
            .operations
            .lock()
            .map_err(|_| DiagError::mutex_poisoned())?;
        let operation = operations.get_mut(op_id).ok_or_else(|| {
            DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!(
                    "Operation with id '{}' not found in entity '{}'",
                    op_id, self.id
                ),
            ))
        })?;
        let execution = operation.executions.get_mut(exec_id).ok_or_else(|| {
            DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                format!(
                    "Execution with id '{}' not found for operation '{}' in entity '{}'",
                    exec_id, op_id, self.id
                ),
            ))
        })?;
        execution.try_resolve();
        if matches!(execution, ActiveExecution::Running { .. }) {
            return Err(DiagError::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::PreconditionNotFulfilled,
                format!(
                    "Execution with id '{}' for operation '{}' in entity '{}' is still running and must explicitly get stopped prior to removal!",
                    exec_id, op_id, self.id
                ),
            )));
        }
        operation.executions.shift_remove(exec_id);
        Ok(())
    }
}

struct DataResourceHolder {
    metadata: DataResourceMetadata,
    instance: Box<dyn DataResource + Send>,
}

impl DataResourceHolder {
    fn new(
        resource: impl DataResource + Send + 'static,
        resource_metadata: DataResourceMetadata,
    ) -> Self {
        Self {
            metadata: resource_metadata,
            instance: Box::new(resource),
        }
    }
}

struct OperationHolder {
    info: OperationMetadata,
    instance: Box<dyn Operation + Send>,
    executions: IndexMap<ExecutionId, ActiveExecution>,
}

impl OperationHolder {
    fn new(op: impl Operation + Send + 'static, op_metadata: OperationMetadata) -> Self {
        Self {
            info: op_metadata,
            instance: Box::new(op),
            executions: IndexMap::<ExecutionId, ActiveExecution>::new(),
        }
    }
}
