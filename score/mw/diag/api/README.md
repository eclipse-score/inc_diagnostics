# Diagnostic API Usage Guide

This directory contains the Rust-facing diagnostic API for score/mw/diag.
The API is intentionally split into a small set of traits and value types so users
can implement diagnostic functionality without coupling their business logic to the runtime internals.

At a high level, the intended usage pattern is:

1. Implement one or more API traits from this directory.
2. Register those implementations on a runtime entity.
3. Let the runtime manage them internally as SOVD-style data resources or operations.
4. Optionally reuse the provided UDS adapters instead of writing the higher-level SOVD-facing wrappers yourself.

The examples in [../examples/examples.rs](../examples/examples.rs) demonstrate some patterns end to end.

## API Surface

The public entry point is the diag_api crate from within the [api](../api/) directory.
It re-exports the common types and groups the user-facing APIs into two main protocol views:

- `diag_api::sovd` for SOVD-oriented data resources and operations
- `diag_api::uds` for UDS-oriented services and the provided adapters

The most relevant building blocks are:

- `DataResource`: read and optionally write diagnostic data values
- `Operation`: full SOVD operation interface with execution control support
- `SimpleOperation`: simplified operation interface only requiring start and stop semantics
- `ReadDataByIdentifier`, `WriteDataByIdentifier`, `RoutineControl`: UDS-specific traits
- `DataResourceAdapter`, `RoutineControlAdapter`, `SimpleOperationAdapter`: bridge types that adapt UDS or simplified implementations to the runtime-facing API

## Common Payload and Error Types

The API supports three payload shapes for requests and replies:

- binary payloads
- JSON payloads
- UTF-8 string payloads

Those are represented by `RequestMessagePayload`, `ReplyMessagePayload`, and `ReplyMessageEncoding`.

Typical reply construction looks like this:

```rust
use diag_api::{DiagnosticReply, ReplyMessagePayload};

let reply = DiagnosticReply {
    message_payload: Some(ReplyMessagePayload::from_string(
        "operation completed".to_string(),
    )),
    additional_attrs: None,
};
```

Errors are returned as `diag_api::Error`. They can wrap either:

- an SOVD-style generic error
- a UDS negative response code

Examples:

```rust
use diag_api::{Error, ReplyMessagePayload};

let sovd_error = Error::from_error(diag_api::sovd::GenericError::from_code(
    diag_api::sovd::ErrorCode::PreconditionNotFulfilled,
    "missing prerequisite".to_string(),
));

let uds_error = Error::from_nrc(diag_api::uds::NegativeResponseCode::ConditionsNotCorrect)
    .with_payload(ReplyMessagePayload::from_byte_vector(vec![0x12, 0x34]));
```

## Implementing a Data Resource

Use `sovd::DataResource` when you want to expose a value under the runtime's data-resource model.

The minimum implementation only needs `read`. The default `write` implementation rejects writes and therefore makes the resource read-only.

```rust
use diag_api::sovd::data_resource::{DataResource, ReadValueArgs, ReadValueReply};
use diag_api::{ReplyMessageEncoding, ReplyMessagePayload, Result as DiagResult};

struct BuildInfoResource {
    version: String,
}

impl DataResource for BuildInfoResource {
    fn read(&self, input: ReadValueArgs) -> DiagResult<ReadValueReply> {
        assert_eq!(input.reply_encoding, ReplyMessageEncoding::UTF8);

        Ok(ReadValueReply {
            id: Some("build_info".to_string()),
            data: ReplyMessagePayload::from_string(self.version.clone()),
            errors: None,
        })
    }
}
```

Implement `write` only when the value is actually writable:

```rust
use diag_api::sovd::data_resource::{DataError, DataResource, ReadValueArgs, ReadValueReply, WriteValueArgs};
use diag_api::{RequestMessagePayload, ReplyMessagePayload, Result as DiagResult};

struct WritableFlag {
    enabled: bool,
}

impl DataResource for WritableFlag {
    fn read(&self, _input: ReadValueArgs) -> DiagResult<ReadValueReply> {
        Ok(ReadValueReply {
            id: Some("feature_flag".to_string()),
            data: ReplyMessagePayload::from_string(self.enabled.to_string()),
            errors: None,
        })
    }

    fn write(&mut self, input: WriteValueArgs) -> Result<(), DataError> {
        match input.user_data {
            Some(RequestMessagePayload::UTF8(value)) if value == "true" => {
                self.enabled = true;
                Ok(())
            }
            Some(RequestMessagePayload::UTF8(value)) if value == "false" => {
                self.enabled = false;
                Ok(())
            }
            _ => Err(DataError::from_error(diag_api::sovd::GenericError::from_code(
                diag_api::sovd::ErrorCode::IncompleteRequest,
                "expected a UTF-8 boolean payload".to_string(),
            ))),
        }
    }
}
```

### Data Resource Metadata

Registration also requires metadata. That metadata is what the runtime returns when clients list available data resources.

```rust
use diag_api::sovd::data_resource::{DataCategory, DataResourceMetadata};

let metadata = DataResourceMetadata {
    id: "build_info".to_string(),
    name: "Build Information".to_string(),
    translation_id: None,
    category: DataCategory::IdentData,
    groups: None,
};
```

## Implementing an Operation

Use `sovd::Operation` when you need full control over execution lifecycle handling.

`execute` receives:

- `ExecuteArguments`, containing requested reply encoding, user payload, additional attributes, and optional proximity proof data
- `ExecutionControl`, which is the channel through which the runtime delivers execution events such as status queries, stop requests, or custom capabilities

`execute` returns an `ExecutionHandle`. That handle contains:

- a future that resolves to the final `ExecutionResult`
- an optional immediate reply that can be sent as the response to the execute request itself

### Synchronous Operation

For short work that can complete directly, returning an `ExecutionHandle` from a closure is the simplest pattern.

```rust
use diag_api::sovd::operation::{ExecuteArguments, ExecutionControl, ExecutionHandle, Operation};
use diag_api::{DiagnosticReply, ReplyMessageEncoding, ReplyMessagePayload, Result as DiagResult};

struct PingOperation;

impl Operation for PingOperation {
    fn execute(
        &mut self,
        input: ExecuteArguments,
        _control: ExecutionControl,
    ) -> DiagResult<ExecutionHandle> {
        assert_eq!(input.reply_encoding, ReplyMessageEncoding::UTF8);

        ExecutionHandle::from_closure(|| {
            Ok(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::from_string(
                    "pong".to_string(),
                )),
                additional_attrs: None,
            })
        })
    }
}
```

### Asynchronous Operation

For longer-running work, return a future and consume execution-control events in parallel.

The relevant intent for the below example is:

- start user work in one future
- process execution control in another future
- use `tokio::select!` so stop or control events can interrupt the operation cleanly
- keep the current execution status in shared state so `ReportStatus` can always return it
- let the user code pause itself in `Interrupted` until the control loop receives `Resume` (just for demonstration puposes here)

Sketch of that pattern:

```rust
use diag_api::sovd::operation::{
    ExecuteArguments, ExecutionControl, ExecutionEventKind, ExecutionHandle, ExecutionResult,
    ExecutionStatus, ExecutionStatusDetails, Operation,
};
use diag_api::{DiagnosticReply, Error, ReplyMessagePayload, Result as DiagResult};
use std::sync::{Arc, Mutex};
use tokio::sync::Notify;

struct AsyncOperation;

impl AsyncOperation {
    async fn user_code(
        _input: ExecuteArguments,
        exec_status: Arc<Mutex<ExecutionStatus>>,
        resume_signal: Arc<Notify>,
    ) -> ExecutionResult {
        {
            let mut status = exec_status.lock().unwrap();
            *status = ExecutionStatus::Interrupted;
        }

        resume_signal.notified().await;

        {
            let mut status = exec_status.lock().unwrap();
            *status = ExecutionStatus::Running;
        }

        Ok(DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string(
                "resumed operation finished".to_string(),
            )),
            additional_attrs: None,
        })
    }

    async fn exec_control(
        mut control: ExecutionControl,
        exec_status: Arc<Mutex<ExecutionStatus>>,
        resume_signal: Arc<Notify>,
    ) {
        let mut last_exec_event_kind = ExecutionEventKind::Resume;
        loop {
            let exec_event = exec_control.next_exec_event().await;
            match exec_event.kind {
                ExecutionEventKind::ControlGone => break,
                ExecutionEventKind::ReportStatus => {
                    let current_status = *exec_status.lock().unwrap();
                    event
                        .status_reporter
                        .put(current_status, ExecutionStatusDetails::none());
                }
                _ => {
                    let mut last_exec_event_kind = exec_event.kind;
                    match last_exec_event_kind {
                        ExecutionEventKind::Resume => {
                            resume_signal.notify_one();
                        }
                        ExecutionEventKind::Stop => {
                            let mut status = exec_status.lock().unwrap();
                            *status = ExecutionStatus::Stopped;
                        }
                        _ => {
                            let mut status = exec_status.lock().unwrap();
                            *status = ExecutionStatus::UnsupportedCapability;
                        }
                    }
                }
            }
        }
    }
}

impl Operation for AsyncOperation {
    fn execute(
        &mut self,
        input: ExecuteArguments,
        control: ExecutionControl,
    ) -> DiagResult<ExecutionHandle> {
        let exec_status = Arc::new(Mutex::new(ExecutionStatus::Running));
        let resume_signal = Arc::new(Notify::new());

        ExecutionHandle::from_future(async move {
            tokio::select! {
                result = Self::user_code(
                    input,
                    Arc::clone(&exec_status),
                    Arc::clone(&resume_signal),
                ) => result,
                _ = Self::exec_control(control, exec_status, resume_signal) => Err(Error::from_error(
                    diag_api::sovd::GenericError::from_code(
                        diag_api::sovd::ErrorCode::ErrorResponse,
                        "execution got stopped".to_string(),
                    ),
                )),
            }
        })
    }
}
```

In this version, `user_code` moves the operation into `Interrupted` immediately,
the control loop reports that shared status whenever `ReportStatus` arrives, and
a `Resume` event wakes the user code so it can complete and return its final reply.

## When to use the SimpleOperation trait

`SimpleOperation` exists for implementations that only need:

- `start()`
- `stop()`
- `completion_percentage()` (optional, for progress reporting)

This is the right abstraction when you do not want to implement the full execution-eventloop yourself.
The adapter handles the runtime control events for you and translates stop requests into `SimpleOperation::stop()` calls.

```rust
use diag_api::sovd::operation::{ExecuteArguments, ExecutionHandle};
use diag_api::sovd::operation::{OperationMetadata};
use diag_api::sovd::operation::{SimpleOperation, SimpleOperationAdapter};
use diag_api::{DiagnosticReply, ReplyMessagePayload, Result as DiagResult};

struct EraseOperation;

impl SimpleOperation for EraseOperation {
    fn start(&mut self, _input: ExecuteArguments) -> DiagResult<ExecutionHandle> {
        ExecutionHandle::from_closure(|| {
            Ok(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::from_string(
                    "erase finished".to_string(),
                )),
                additional_attrs: None,
            })
        })
    }

    fn stop(&mut self, _input: Option<ExecuteArguments>) -> DiagResult<Option<DiagnosticReply>> {
        Ok(Some(DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_string(
                "erase stopped".to_string(),
            )),
            additional_attrs: None,
        }))
    }
}

let operation = SimpleOperationAdapter::from(EraseOperation);
let metadata = OperationMetadata {
    proximity_proof_required: false,
    synchronous_execution: true,
    exclusive_execution: true,
    supported_modes: None,
};
```

The typical reason to choose `SimpleOperation` over `Operation` is that the runtime-level execution event protocol is an implementation detail you do not need.

## UDS Integration

The UDS API exists so users can provide UDS semantics directly and then reuse adapters that project those services into the higher-level SOVD model.

### ReadDataByIdentifier and WriteDataByIdentifier

If you already have a UDS DID implementation, implement the UDS trait and wrap it in `DataResourceAdapter`.

```rust
use diag_api::uds::{DataResourceAdapter, ReadDataByIdentifier};
use diag_api::Result as DiagResult;

struct VinDid;

impl ReadDataByIdentifier for VinDid {
    fn read(&self) -> DiagResult<Vec<u8>> {
        Ok(vec![0xDE, 0xAD, 0xBE, 0xEF])
    }
}

let resource = DataResourceAdapter::from_rdbi(VinDid);
```

Important adapter constraints:

- `ReadDataByIdentifier` only supports binary replies through the adapter
- `WriteDataByIdentifier` expects binary request payloads through the adapter
- unsupported payload encodings are rejected with a diagnostic error

### RoutineControl

If the diagnostic function is naturally a UDS routine, implement `RoutineControl` and wrap it first in `RoutineControlAdapter` and then in `SimpleOperationAdapter`.

This is exactly the intended bridge from UDS RoutineControl to the runtime's operation model.

```rust
use diag_api::uds::{RoutineControl, RoutineControlAdapter, StartRoutine};
use diag_api::sovd::operation::SimpleOperationAdapter;
use diag_api::Result as DiagResult;
use std::sync::Arc;
use tokio::sync::Notify;

struct MyRoutine {
    completion: Arc<Notify>,
}

impl RoutineControl for MyRoutine {
    fn start(&mut self, _input: Option<&[u8]>) -> DiagResult<StartRoutine> {
        let completion = self.completion.clone();
        StartRoutine::from_future_with_reply(
            async move {
                completion.notified().await;
                Ok(Some(vec![0xCA, 0xFE]))
            },
            vec![0xBE, 0xEF],
        )
    }

    fn stop(&mut self, _input: Option<&[u8]>) -> DiagResult<Option<Vec<u8>>> {
        self.completion.notify_one();
        Ok(Some(vec![0xDE, 0xAD]))
    }
}

let operation = SimpleOperationAdapter::from(RoutineControlAdapter::from(MyRoutine {
    completion: Arc::new(Notify::new()),
}));
```

## Registering Implementations at the Runtime

The example test code in [../examples/examples.rs](../examples/examples.rs) shows the expected runtime integration point:

```rust
use diag_api::sovd::data_resource::{DataCategory, DataResourceMetadata};
use diag_api::sovd::operation::OperationMetadata;
use diag_runtime::Runtime;

let runtime = Runtime::new();
let entity = runtime.get_or_create_entity("my_entity".to_string());

entity.register_data_resource(
    BuildInfoResource {
        version: "1.2.3".to_string(),
    },
    "build_info".to_string(),
    DataResourceMetadata {
        id: "build_info".to_string(),
        name: "Build Information".to_string(),
        translation_id: None,
        category: DataCategory::IdentData,
        groups: None,
    },
);

entity.register_operation(
    PingOperation,
    "ping".to_string(),
    OperationMetadata {
        proximity_proof_required: false,
        synchronous_execution: true,
        exclusive_execution: false,
        supported_modes: None,
    },
);
```

That registration step is the boundary between user-owned diagnostic logic and runtime-owned transport, scheduling, and execution management.

## Operation Metadata Semantics

Each registered operation is accompanied by `OperationMetadata`:

- `proximity_proof_required`: execution requires proof of co-location
- `synchronous_execution`: execution is expected to complete synchronously
- `exclusive_execution`: concurrent executions are not allowed
- `supported_modes`: optional mode restrictions accepted by the operation

These fields describe behavior to the runtime and to remote clients.
They are not just documentation fields, so the metadata should match the real execution behavior.

## Execution Lifecycle and Status Reporting

For full `Operation` implementations, the runtime communicates via `ExecutionControl` events.
The currently modeled event kinds include:

- `HandleCustomCapability`
- `ReportStatus`
- `Interrupt`
- `Resume`
- `Reset`
- `Stop`

The intended model is:

1. `execute` creates the operation task.
2. The runtime assigns an execution ID.
3. The runtime may query status at any time.
4. The runtime may issue stop or custom-capability events.
5. The user implementation completes with a final `DiagnosticReply` or `Error`.

When handling `ReportStatus`, fill `ExecutionStatusDetails` with whatever is meaningful for your implementation:

- `last_executed_capability` (mandatory)
- `completion_percentage`
- `event_result`
- `exec_errors`

If you do not need that control-plane detail, prefer to implement the `SimpleOperation` trait
instead and let the respective adapter manage all of it.

## Choosing the right abstraction

Use `DataResource` when:

- the functionality is a value read or write
- there is no execution lifecycle beyond a direct request and reply

Use `Operation` when:

- you need full control over execution events and status transitions
- the runtime must be able to drive or observe a long-running activity
- you need custom capability handling

Use `SimpleOperation` when:

- you need start and stop handling but not a custom execution-control loop
- the adapter behavior matches your lifecycle well enough

Use the UDS traits and adapters when:

- you already have UDS service implementations
- you want the library to translate those into the higher-level SOVD-facing API

## Practical Recommendations

- Keep payload validation at the API boundary. Reject unsupported encodings early.
- Return explicit diagnostic errors instead of panicking for client-driven failures.
- Only mark an operation as synchronous if it really finishes within the execute path.
- Use `SimpleOperationAdapter` for routine-like behavior before writing a custom `Operation` implementation.
- Use the UDS adapters when your underlying diagnostic primitive is already a DID or routine.

## Reference Files

For the concrete implementations which the above guide described, see:

- [data_resource.rs](../api/data_resource.rs)
- [operation.rs](../api/operation.rs)
- [simple_operation.rs](../api/simple_operation.rs)
- [uds.rs](../api/uds.rs)
- [uds_adapters.rs](../api/uds_adapters.rs)
- [../examples/examples.rs](../examples/examples.rs)
