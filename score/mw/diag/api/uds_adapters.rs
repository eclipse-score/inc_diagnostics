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
use common::*;
use data_resource::{
    DataResource, ReadValueArgs, ReadValueHandle, ReadValueReply, WriteValueArgs, WriteValueHandle,
};
use operation::{ExecuteArguments, ExecutionHandle};
use simple_operation::SimpleOperation;

/// UDS (Unified Diagnostic Services) data resource adapters.
///
/// Bridges UDS diagnostic services (ReadDataByIdentifier / WriteDataByIdentifier)
/// to the protocol-agnostic [`DataResource`](super::DataResource) trait.

/// A UDS data resource backed by either a read or write service, or both.
pub struct DataResourceAdapter {
    /// Optional read service via ReadDataByIdentifier (0x22).
    rdbi: Option<Box<dyn ::uds::ReadDataByIdentifier + Send + Sync>>,
    /// Optional write service via WriteDataByIdentifier (0x2E).
    wdbi: Option<Box<dyn ::uds::WriteDataByIdentifier + Send + Sync>>,
}

impl DataResourceAdapter {
    #[must_use]
    pub fn new() -> Self {
        Self {
            rdbi: None,
            wdbi: None,
        }
    }

    #[must_use]
    pub fn with_rdbi(
        mut self,
        rdbi: impl ::uds::ReadDataByIdentifier + Send + Sync + 'static,
    ) -> Self {
        self.rdbi = Some(Box::new(rdbi));
        self
    }

    #[must_use]
    pub fn with_wdbi(
        mut self,
        wdbi: impl ::uds::WriteDataByIdentifier + Send + Sync + 'static,
    ) -> Self {
        self.wdbi = Some(Box::new(wdbi));
        self
    }
}

impl DataResource for DataResourceAdapter {
    fn read(&self, input: ReadValueArgs) -> ReadValueHandle {
        let Some(rdbi) = &self.rdbi else {
            return ReadValueHandle::from_error(Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::PreconditionNotFulfilled,
                "No ReadDataByIdentifier service got registered for this data resource!"
                    .to_string(),
            )));
        };
        match input.reply_encoding {
            ReplyMessageEncoding::Binary => match rdbi.read() {
                Ok(data) => ReadValueHandle::ready(ReadValueReply {
                    data: ReplyMessagePayload::from_byte_vector(data.to_vec()),
                    errors: None,
                }),
                Err(e) => ReadValueHandle::from_error(e),
            },
            _ => ReadValueHandle::from_error(Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::PreconditionNotFulfilled,
                "This data resource only supports binary encoding for its reply data!".to_string(),
            ))),
        }
    }

    fn write(&mut self, input: WriteValueArgs) -> WriteValueHandle {
        let Some(wdbi) = &mut self.wdbi else {
            return WriteValueHandle::from_error(sovd::DataError::from_error(
                sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "No WriteDataByIdentifier service got registered for this data resource!"
                        .to_string(),
                ),
            ));
        };
        if let Some(RequestMessagePayload::Binary(data)) = input.user_data {
            match wdbi.write(&data) {
                Ok(()) => WriteValueHandle::ready(),
                Err(e) => WriteValueHandle::from_error(sovd::DataError {
                    path: String::new(),
                    error: match e.code {
                        ErrorCode::SOVD(err) => Some(err),
                        ErrorCode::UDS(nrc) => Some(sovd::GenericError::from_code(
                            sovd::ErrorCode::ErrorResponse,
                            format!("Write operation failed with NRC 0x{:02X}", u8::from(nrc)),
                        )),
                    },
                }),
            }
        } else {
            WriteValueHandle::from_error(sovd::DataError::from_error(
                sovd::GenericError::from_code(
                    sovd::ErrorCode::IncompleteRequest,
                    "This data resource requires binary encoding for its input data!".to_string(),
                ),
            ))
        }
    }
}

/// UDS (Unified Diagnostic Services) routine control adapter.
///
/// Bridges UDS RoutineControl (cf. ISO 14229-1:2020, Service 0x31)
/// to the [`SimpleOperation`](super::SimpleOperation) trait.
pub struct RoutineControlAdapter {
    routine_control: Box<dyn ::uds::RoutineControl + Send>,
}

impl RoutineControlAdapter {
    #[must_use]
    pub fn new(instance: impl ::uds::RoutineControl + Send + 'static) -> Self {
        Self {
            routine_control: Box::new(instance),
        }
    }
}

impl SimpleOperation for RoutineControlAdapter {
    fn start(&mut self, input: ExecuteArguments) -> DiagResult<ExecutionHandle> {
        let byte_input = match input.user_parameters {
            Some(RequestMessagePayload::Binary(data)) => Some(data),
            None => None,
            _ => {
                return Err(Error::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "UDS RoutineControl only supports binary encoding for its input!".to_string(),
                )))
            }
        };
        let start_routine = self.routine_control.start(byte_input.as_deref())?;

        Ok(ExecutionHandle {
            future: Box::pin(async move {
                match start_routine.future.await {
                    Ok(Some(bytes)) => Ok(DiagnosticReply {
                        message_payload: Some(ReplyMessagePayload::from_byte_vector(bytes)),
                        additional_attrs: None,
                    }),
                    Ok(None) => Ok(DiagnosticReply::default()),
                    Err(err) => Err(err),
                }
            }),
            reply: start_routine.reply.map(|bytes| DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::from_byte_vector(bytes)),
                additional_attrs: None,
            }),
        })
    }

    fn stop(&mut self, input: Option<ExecuteArguments>) -> DiagResult<Option<DiagnosticReply>> {
        let byte_input = match input.map(|args| args.user_parameters) {
            Some(Some(RequestMessagePayload::Binary(data))) => Some(data),
            Some(None) | None => None,
            _ => {
                return Err(Error::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "UDS RoutineControl only supports binary encoding for its input!".to_string(),
                )))
            }
        };
        let result = self.routine_control.stop(byte_input.as_deref())?;
        Ok(result.map(|bytes| DiagnosticReply {
            message_payload: Some(ReplyMessagePayload::from_byte_vector(bytes)),
            additional_attrs: None,
        }))
    }

    fn completion_percentage(&self) -> Option<u8> {
        self.routine_control.completion_percentage()
    }
}

/*******************/
/* Unit Tests      */
/*******************/

#[cfg(test)]
mod tests {
    use super::*;
    use data_resource::DataResource;
    use {ByteSlice, ByteVector};

    /// Helper to extract the result from a ReadValueHandle::Ready variant in tests.
    fn unwrap_read_value_handle(handle: ReadValueHandle) -> common::Result<ReadValueReply> {
        match handle {
            ReadValueHandle::Ready(result) => result,
            ReadValueHandle::Pending(_) => panic!("expected Ready, got Pending"),
        }
    }

    /// Helper to extract the result from a WriteValueHandle::Ready variant in tests.
    fn unwrap_write_value_handle(
        handle: WriteValueHandle,
    ) -> std::result::Result<(), sovd::DataError> {
        match handle {
            WriteValueHandle::Ready(result) => result,
            WriteValueHandle::Pending(_) => panic!("expected Ready, got Pending"),
        }
    }

    // ── UDS RDBI / WDBI stub implementations ──────────────────────────

    struct RdbiForTest {
        data: ByteVector,
    }

    impl ::uds::ReadDataByIdentifier for RdbiForTest {
        fn read(&self) -> Result<ByteVector> {
            Ok(self.data.clone())
        }
    }

    struct FailingRdbi;

    impl ::uds::ReadDataByIdentifier for FailingRdbi {
        fn read(&self) -> Result<ByteVector> {
            Err(Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::NotResponding,
                "device not responding".to_string(),
            )))
        }
    }

    struct WdbiForTest {
        written: Option<ByteVector>,
    }

    impl ::uds::WriteDataByIdentifier for WdbiForTest {
        fn write(&mut self, input: ByteSlice) -> Result<()> {
            self.written = Some(input.to_vec());
            Ok(())
        }
    }

    struct FailingWdbi;

    impl ::uds::WriteDataByIdentifier for FailingWdbi {
        fn write(&mut self, _input: ByteSlice) -> Result<()> {
            Err(Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                "write failed".to_string(),
            )))
        }
    }

    // ── UDS DataResourceAdapter::read via RDBI ───────────────────────────────

    #[test]
    fn uds_rdbi_read_returns_binary_payload() {
        let resource = DataResourceAdapter::new().with_rdbi(RdbiForTest {
            data: vec![0xDE, 0xAD],
        });
        let result = unwrap_read_value_handle(
            resource.read(ReadValueArgs::new(ReplyMessageEncoding::Binary)),
        )
        .unwrap();
        assert_eq!(result.data, ReplyMessagePayload::Binary(vec![0xDE, 0xAD]));
        assert!(result.errors.is_none());
    }

    #[test]
    fn uds_rdbi_read_propagates_error() {
        let resource = DataResourceAdapter::new().with_rdbi(FailingRdbi {});
        let err = unwrap_read_value_handle(
            resource.read(ReadValueArgs::new(ReplyMessageEncoding::Binary)),
        )
        .unwrap_err();
        match err.code {
            ErrorCode::SOVD(ref e) => {
                assert_eq!(e.sovd_error, "not-responding");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    // ── UDS DataResourceAdapter::read via WDBI (should fail) ─────────────────

    #[test]
    fn uds_wdbi_read_returns_error() {
        let resource = DataResourceAdapter::new().with_wdbi(WdbiForTest { written: None });
        let err = unwrap_read_value_handle(
            resource.read(ReadValueArgs::new(ReplyMessageEncoding::Binary)),
        )
        .unwrap_err();
        match err.code {
            ErrorCode::SOVD(ref e) => {
                assert_eq!(e.sovd_error, "precondition-not-fulfilled");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    // ── UDS DataResourceAdapter::write via RDBI (should fail) ────────────────

    #[test]
    fn uds_rdbi_write_returns_error() {
        let mut resource = DataResourceAdapter::new().with_rdbi(RdbiForTest { data: vec![0x01] });
        let err = unwrap_write_value_handle(resource.write(WriteValueArgs {
            user_data: Some(RequestMessagePayload::Binary(vec![0x01])),
            user_data_signature: None,
            additional_attrs: None,
        }))
        .unwrap_err();
        assert_eq!(
            err.error.as_ref().unwrap().sovd_error,
            "precondition-not-fulfilled"
        );
    }

    // ── UDS DataResourceAdapter::write via WDBI ──────────────────────────────

    #[test]
    fn uds_wdbi_write_succeeds_with_binary_data() {
        let mut resource = DataResourceAdapter::new().with_wdbi(WdbiForTest { written: None });
        let result = unwrap_write_value_handle(resource.write(WriteValueArgs {
            user_data: Some(RequestMessagePayload::Binary(vec![0xCA, 0xFE])),
            user_data_signature: None,
            additional_attrs: None,
        }));
        assert!(result.is_ok());
    }

    #[test]
    fn uds_wdbi_write_fails_with_non_binary_payload() {
        let mut resource = DataResourceAdapter::new().with_wdbi(WdbiForTest { written: None });
        let err = unwrap_write_value_handle(resource.write(WriteValueArgs {
            user_data: Some(RequestMessagePayload::UTF8("text".to_string())),
            user_data_signature: None,
            additional_attrs: None,
        }))
        .unwrap_err();
        assert_eq!(err.error.as_ref().unwrap().sovd_error, "incomplete-request");
    }

    #[test]
    fn uds_wdbi_write_fails_with_no_user_data() {
        let mut resource = DataResourceAdapter::new().with_wdbi(WdbiForTest { written: None });
        let err = unwrap_write_value_handle(resource.write(WriteValueArgs {
            user_data: None,
            user_data_signature: None,
            additional_attrs: None,
        }))
        .unwrap_err();
        assert_eq!(err.error.as_ref().unwrap().sovd_error, "incomplete-request");
    }

    #[test]
    fn uds_wdbi_write_maps_underlying_error_to_data_error() {
        let mut resource = DataResourceAdapter::new().with_wdbi(FailingWdbi {});
        let err = unwrap_write_value_handle(resource.write(WriteValueArgs {
            user_data: Some(RequestMessagePayload::Binary(vec![0x01])),
            user_data_signature: None,
            additional_attrs: None,
        }))
        .unwrap_err();
        assert_eq!(err.error.as_ref().unwrap().sovd_error, "error-response");
    }

    // ── UDS DataResourceAdapter::read with non-binary encoding ───────────────

    #[test]
    fn uds_rdbi_read_rejects_non_binary_encoding() {
        let resource = DataResourceAdapter::new().with_rdbi(RdbiForTest { data: vec![0x01] });
        let err =
            unwrap_read_value_handle(resource.read(ReadValueArgs::new(ReplyMessageEncoding::UTF8)))
                .unwrap_err();
        match err.code {
            ErrorCode::SOVD(ref e) => {
                assert_eq!(e.sovd_error, "precondition-not-fulfilled");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    // ── UDS DataResourceAdapter with both RDBI and WDBI ──────────────────────

    #[test]
    fn uds_combined_adapter_supports_read_and_write() {
        let mut resource = DataResourceAdapter::new()
            .with_rdbi(RdbiForTest {
                data: vec![0xAB, 0xCD],
            })
            .with_wdbi(WdbiForTest { written: None });

        let read_result = unwrap_read_value_handle(
            resource.read(ReadValueArgs::new(ReplyMessageEncoding::Binary)),
        )
        .unwrap();
        assert_eq!(
            read_result.data,
            ReplyMessagePayload::Binary(vec![0xAB, 0xCD])
        );

        let write_result = unwrap_write_value_handle(resource.write(WriteValueArgs {
            user_data: Some(RequestMessagePayload::Binary(vec![0xEF])),
            user_data_signature: None,
            additional_attrs: None,
        }));
        assert!(write_result.is_ok());
    }

    // ── UDS RoutineControl stub implementations ─────────────────────────

    struct RoutineControlForTest {
        start_reply: Option<ByteVector>,
        start_result: Option<ByteVector>,
        stop_result: Option<ByteVector>,
        completion: Option<u8>,
    }

    impl ::uds::RoutineControl for RoutineControlForTest {
        fn start(&mut self, _input: Option<ByteSlice>) -> DiagResult<::uds::StartRoutine> {
            let result = self.start_result.clone();
            let reply = self.start_reply.clone();
            ::uds::StartRoutine::from_future(async move { Ok(result) }, reply)
        }

        fn stop(&mut self, _input: Option<ByteSlice>) -> DiagResult<Option<ByteVector>> {
            Ok(self.stop_result.clone())
        }

        fn completion_percentage(&self) -> Option<u8> {
            self.completion
        }
    }

    struct FailingRoutineControl;

    impl ::uds::RoutineControl for FailingRoutineControl {
        fn start(&mut self, _input: Option<ByteSlice>) -> DiagResult<::uds::StartRoutine> {
            Err(Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::NotResponding,
                "routine start failed".to_string(),
            )))
        }

        fn stop(&mut self, _input: Option<ByteSlice>) -> DiagResult<Option<ByteVector>> {
            Err(Error::from_error(sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                "routine stop failed".to_string(),
            )))
        }
    }

    // ── RoutineControlAdapter::start ─────────────────────────────────────

    #[tokio::test]
    async fn routine_start_with_binary_input_succeeds() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: Some(vec![0xCA, 0xFE]),
            stop_result: None,
            completion: None,
        });
        let handle = adapter
            .start(ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::Binary,
                user_parameters: Some(RequestMessagePayload::Binary(vec![0x01, 0x02])),
                additional_attrs: None,
                proximity_response: None,
            })
            .unwrap();
        assert!(handle.reply.is_none());
        let result = handle.future.await.unwrap();
        assert_eq!(
            result.message_payload,
            Some(ReplyMessagePayload::Binary(vec![0xCA, 0xFE]))
        );
    }

    #[tokio::test]
    async fn routine_start_with_no_input_succeeds() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: Some(vec![0xAB]),
            stop_result: None,
            completion: None,
        });
        let handle = adapter
            .start(ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::Binary,
                user_parameters: None,
                additional_attrs: None,
                proximity_response: None,
            })
            .unwrap();
        let result = handle.future.await.unwrap();
        assert_eq!(
            result.message_payload,
            Some(ReplyMessagePayload::Binary(vec![0xAB]))
        );
    }

    #[tokio::test]
    async fn routine_start_with_reply_returns_immediate_reply() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: Some(vec![0xBE, 0xEF]),
            start_result: Some(vec![0xCA, 0xFE]),
            stop_result: None,
            completion: None,
        });
        let handle = adapter
            .start(ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::Binary,
                user_parameters: None,
                additional_attrs: None,
                proximity_response: None,
            })
            .unwrap();
        assert_eq!(
            handle.reply,
            Some(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::Binary(vec![0xBE, 0xEF])),
                additional_attrs: None,
            })
        );
        let result = handle.future.await.unwrap();
        assert_eq!(
            result.message_payload,
            Some(ReplyMessagePayload::Binary(vec![0xCA, 0xFE]))
        );
    }

    #[tokio::test]
    async fn routine_start_with_none_result_returns_default_reply() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: None,
            completion: None,
        });
        let handle = adapter
            .start(ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::Binary,
                user_parameters: None,
                additional_attrs: None,
                proximity_response: None,
            })
            .unwrap();
        let result = handle.future.await.unwrap();
        assert_eq!(result, DiagnosticReply::default());
    }

    #[test]
    fn routine_start_rejects_non_binary_input() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: None,
            completion: None,
        });
        let result = adapter.start(ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::Binary,
            user_parameters: Some(RequestMessagePayload::UTF8("text".to_string())),
            additional_attrs: None,
            proximity_response: None,
        });
        match result {
            Err(ref err) => match err.code {
                ErrorCode::SOVD(ref e) => {
                    assert_eq!(e.sovd_error, "precondition-not-fulfilled");
                }
                _ => panic!("expected SOVD error code"),
            },
            Ok(_) => panic!("expected error"),
        }
    }

    #[test]
    fn routine_start_propagates_error_from_routine_control() {
        let mut adapter = RoutineControlAdapter::new(FailingRoutineControl);
        let result = adapter.start(ExecuteArguments {
            reply_encoding: ReplyMessageEncoding::Binary,
            user_parameters: None,
            additional_attrs: None,
            proximity_response: None,
        });
        match result {
            Err(ref err) => match err.code {
                ErrorCode::SOVD(ref e) => {
                    assert_eq!(e.sovd_error, "not-responding");
                }
                _ => panic!("expected SOVD error code"),
            },
            Ok(_) => panic!("expected error"),
        }
    }

    // ── RoutineControlAdapter::stop ──────────────────────────────────────

    #[test]
    fn routine_stop_with_binary_input_succeeds() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: Some(vec![0xDE, 0xAD]),
            completion: None,
        });
        let result = adapter
            .stop(Some(ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::Binary,
                user_parameters: Some(RequestMessagePayload::Binary(vec![0x03])),
                additional_attrs: None,
                proximity_response: None,
            }))
            .unwrap();
        assert_eq!(
            result,
            Some(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::Binary(vec![0xDE, 0xAD])),
                additional_attrs: None,
            })
        );
    }

    #[test]
    fn routine_stop_with_no_args_succeeds() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: Some(vec![0xFF]),
            completion: None,
        });
        let result = adapter.stop(None).unwrap();
        assert_eq!(
            result,
            Some(DiagnosticReply {
                message_payload: Some(ReplyMessagePayload::Binary(vec![0xFF])),
                additional_attrs: None,
            })
        );
    }

    #[test]
    fn routine_stop_with_no_user_parameters_succeeds() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: None,
            completion: None,
        });
        let result = adapter
            .stop(Some(ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::Binary,
                user_parameters: None,
                additional_attrs: None,
                proximity_response: None,
            }))
            .unwrap();
        assert!(result.is_none());
    }

    #[test]
    fn routine_stop_returns_none_when_routine_returns_none() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: None,
            completion: None,
        });
        let result = adapter.stop(None).unwrap();
        assert!(result.is_none());
    }

    #[test]
    fn routine_stop_rejects_non_binary_input() {
        let mut adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: None,
            completion: None,
        });
        let err = adapter
            .stop(Some(ExecuteArguments {
                reply_encoding: ReplyMessageEncoding::Binary,
                user_parameters: Some(RequestMessagePayload::UTF8("text".to_string())),
                additional_attrs: None,
                proximity_response: None,
            }))
            .unwrap_err();
        match err.code {
            ErrorCode::SOVD(ref e) => {
                assert_eq!(e.sovd_error, "precondition-not-fulfilled");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    #[test]
    fn routine_stop_propagates_error_from_routine_control() {
        let mut adapter = RoutineControlAdapter::new(FailingRoutineControl);
        let err = adapter.stop(None).unwrap_err();
        match err.code {
            ErrorCode::SOVD(ref e) => {
                assert_eq!(e.sovd_error, "error-response");
            }
            _ => panic!("expected SOVD error code"),
        }
    }

    // ── RoutineControlAdapter::completion_percentage ─────────────────────

    #[test]
    fn routine_completion_percentage_returns_value() {
        let adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: None,
            completion: Some(42),
        });
        assert_eq!(adapter.completion_percentage(), Some(42));
    }

    #[test]
    fn routine_completion_percentage_returns_none_when_not_available() {
        let adapter = RoutineControlAdapter::new(RoutineControlForTest {
            start_reply: None,
            start_result: None,
            stop_result: None,
            completion: None,
        });
        assert_eq!(adapter.completion_percentage(), None);
    }
}
