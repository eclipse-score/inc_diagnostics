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
use common::{ByteSlice, ByteVector};
use std::future::Future;
use std::pin::Pin;

/// UDS ReadDataByIdentifier service (cf. ISO 14229-1:2020, Service 0x22).
pub trait ReadDataByIdentifier {
    /// Read raw bytes for the data identifier.
    fn read(&self) -> DiagResult<ByteVector>;
}

/// UDS WriteDataByIdentifier service (cf. ISO 14229-1:2020, Service 0x2E).
pub trait WriteDataByIdentifier {
    /// Write raw bytes for the data identifier.
    fn write(&mut self, input: ByteSlice) -> DiagResult<()>;
}

/// UDS RoutineControl service (cf. ISO 14229-1:2020, Service 0x31).
/// NOTE: request routine results (sub-function 0x03) will get handled
///       implicitly by the diag runtime via `ExecutionEvent::ReportStatus`.
pub trait RoutineControl {
    /// Start a routine (sub-function 0x01).
    fn start(&mut self, input: Option<ByteSlice>) -> DiagResult<StartRoutine>;

    /// Stop a routine (sub-function 0x02).
    fn stop(&mut self, input: Option<ByteSlice>) -> DiagResult<Option<ByteVector>>;

    /// Optionally provide the current completion percentage
    fn completion_percentage(&self) -> Option<u8> {
        None
    }
}

/// Returned by `RoutineControl::start` and contains a future (which optionally produces
/// a `ByteVector` as execution result), along with an optional `ByteVector` which
/// shall get used as reply to the `RoutineControl::start` request.
#[must_use]
pub struct StartRoutine {
    pub future: Pin<Box<dyn Future<Output = DiagResult<Option<ByteVector>>> + Send>>,
    pub reply: Option<ByteVector>,
}

impl StartRoutine {
    pub fn from_closure<Func>(func: Func, reply: Option<ByteVector>) -> DiagResult<Self>
    where
        Func: FnOnce() -> DiagResult<Option<ByteVector>> + Send + 'static,
    {
        Ok(Self {
            future: Box::pin(async move { func() }),
            reply: reply,
        })
    }

    pub fn from_future<Fut>(future: Fut, reply: Option<ByteVector>) -> DiagResult<Self>
    where
        Fut: Future<Output = DiagResult<Option<ByteVector>>> + Send + 'static,
    {
        Ok(Self {
            future: Box::pin(future),
            reply: reply,
        })
    }

    pub fn from_nrc(nrc: ::common::uds::NegativeResponseCode) -> DiagResult<Self> {
        Err(::common::Error::from_nrc(nrc))
    }
}
