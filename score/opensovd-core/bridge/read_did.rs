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
use std::ffi::c_void;
use std::fmt;
use std::marker::{PhantomData, PhantomPinned};
use std::ptr::NonNull;
use std::sync::Arc;
use std::time::Duration;

use async_trait::async_trait;
use opensovd_core::DataError;
use opensovd_providers::data::{ReadableDataResource, Value};
use tokio::sync::oneshot;

/// Opaque C++ reader registration (`ScoreDiagReader` in read_did.h).
#[repr(C)]
pub struct ScoreDiagReader {
    _private: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

/// Opaque C++ read request (`ScoreDiagReadRequest` in read_did.h).
#[repr(C)]
pub struct ScoreDiagReadRequest {
    _private: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

/// Mirrors `ScoreDiagReadStatus`; read_did.cpp static_asserts the same values and size.
#[repr(u8)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum ReadStatus {
    Ok = 0,
    Nrc = 1,
    Cancelled = 2,
    Failed = 3,
    Busy = 4,
}

impl ReadStatus {
    fn from_raw(raw: u8) -> Option<Self> {
        [Self::Ok, Self::Nrc, Self::Cancelled, Self::Failed, Self::Busy]
            .into_iter()
            .find(|status| *status as u8 == raw)
    }
}

type ReadCompletion = unsafe extern "C" fn(*mut c_void, u8, u8, *const u8, usize);

unsafe extern "C" {
    fn score_diag_demo_reader_create(delay_ms: u32, nrc: u8) -> *mut ScoreDiagReader;
    #[cfg(test)]
    fn score_diag_reader_clone(reader: *const ScoreDiagReader) -> *mut ScoreDiagReader;
    fn score_diag_reader_release(reader: *mut ScoreDiagReader);
    fn score_diag_read_start(
        reader: *mut ScoreDiagReader,
        context: *mut c_void,
        completion: ReadCompletion,
    ) -> *mut ScoreDiagReadRequest;
    fn score_diag_read_cancel(request: *mut ScoreDiagReadRequest);
    fn score_diag_read_release(request: *mut ScoreDiagReadRequest);
}

/// Failure of a single diagnostic read.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ReadError {
    /// The UDS handler answered with this negative response code.
    Nrc(u8),
    /// The handler already has the maximum number of outstanding reads.
    Busy,
    Cancelled,
    Timeout,
    Failed,
}

/// ISO 14229-1 negative response code for "requestOutOfRange".
const NRC_REQUEST_OUT_OF_RANGE: u8 = 0x31;

impl fmt::Display for ReadError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Nrc(nrc) => write!(f, "UDS NRC 0x{nrc:02X}"),
            Self::Busy => f.write_str("diagnostic reader busy"),
            Self::Cancelled => f.write_str("diagnostic read cancelled"),
            Self::Timeout => f.write_str("diagnostic read timed out"),
            Self::Failed => f.write_str("diagnostic read failed"),
        }
    }
}

impl From<ReadError> for DataError {
    /// Core's `DataError` has no variants for access denied, conflict, busy or timeout, so only
    /// requestOutOfRange gets a distinct status (404). Everything else stays Internal (HTTP 500) until
    /// Core offers a typed provider error; the NRC is kept in the message either way.
    fn from(error: ReadError) -> Self {
        match error {
            ReadError::Nrc(NRC_REQUEST_OUT_OF_RANGE) => DataError::NotFound(error.to_string()),
            _ => DataError::Internal(error.to_string()),
        }
    }
}

type Reply = Result<Vec<u8>, ReadError>;

struct Handler(NonNull<ScoreDiagReader>);

// SAFETY: the handle is immutable from Rust; C++ shares handler state via shared_ptr, counts requests
// atomically and serializes Read dispatch behind a mutex.
unsafe impl Send for Handler {}
unsafe impl Sync for Handler {}

impl Drop for Handler {
    fn drop(&mut self) {
        unsafe { score_diag_reader_release(self.0.as_ptr()) };
    }
}

struct Request(NonNull<ScoreDiagReadRequest>);

// SAFETY: a request has a single owner; cancel and release are never called concurrently.
unsafe impl Send for Request {}

impl Drop for Request {
    fn drop(&mut self) {
        unsafe {
            score_diag_read_cancel(self.0.as_ptr());
            score_diag_read_release(self.0.as_ptr());
        }
    }
}

unsafe extern "C" fn complete(context: *mut c_void, status: u8, nrc: u8, data: *const u8, size: usize) {
    let sender = unsafe { Box::from_raw(context.cast::<oneshot::Sender<Reply>>()) };
    let reply = match ReadStatus::from_raw(status) {
        Some(ReadStatus::Ok) if size == 0 => Ok(Vec::new()),
        Some(ReadStatus::Ok) if data.is_null() => Err(ReadError::Failed),
        Some(ReadStatus::Ok) => Ok(unsafe { std::slice::from_raw_parts(data, size) }.to_vec()),
        Some(ReadStatus::Nrc) => Err(ReadError::Nrc(nrc)),
        Some(ReadStatus::Cancelled) => Err(ReadError::Cancelled),
        Some(ReadStatus::Busy) => Err(ReadError::Busy),
        Some(ReadStatus::Failed) | None => Err(ReadError::Failed),
    };
    let _ = sender.send(reply);
}

#[derive(Clone)]
pub struct ReadDid {
    handler: Arc<Handler>,
    timeout: Duration,
}

impl ReadDid {
    pub fn demo(delay_ms: u32, nrc: u8, timeout: Duration) -> Result<Self, DataError> {
        unsafe { Self::from_registration(score_diag_demo_reader_create(delay_ms, nrc), timeout) }
    }

    /// Takes ownership of a registration returned by the C++ bridge.
    ///
    /// # Safety
    /// A non-null handle must be live and exclusively owned by the caller, which relinquishes it here.
    pub unsafe fn from_registration(handle: *mut ScoreDiagReader, timeout: Duration) -> Result<Self, DataError> {
        let handler =
            NonNull::new(handle).ok_or_else(|| DataError::Internal("could not register C++ handler".to_owned()))?;
        Ok(Self {
            handler: Arc::new(Handler(handler)),
            timeout,
        })
    }

    /// Reads the raw DID payload.
    pub async fn read_bytes(&self) -> Result<Vec<u8>, ReadError> {
        let (sender, receiver) = oneshot::channel::<Reply>();
        let context = Box::into_raw(Box::new(sender)).cast();
        let request = unsafe { score_diag_read_start(self.handler.0.as_ptr(), context, complete) };
        let request = NonNull::new(request).map(Request);
        let result = tokio::time::timeout(self.timeout, receiver)
            .await
            .map_err(|_| ReadError::Timeout)?
            .map_err(|_| ReadError::Failed)?;
        drop(request);
        result
    }
}

/// DID payload that is UTF-8 text, e.g. a version or identification string.
pub struct Utf8(pub ReadDid);

#[async_trait]
impl ReadableDataResource for Utf8 {
    type Value = Value<String>;

    async fn read(&self) -> Result<Self::Value, DataError> {
        utf8_value(self.0.read_bytes().await?)
    }
}

fn utf8_value(bytes: Vec<u8>) -> Result<Value<String>, DataError> {
    String::from_utf8(bytes)
        .map(Value::new)
        .map_err(|_| DataError::Internal("DID payload is not UTF-8".to_owned()))
}

#[cfg(test)]
pub(crate) mod test_support {
    use super::*;

    #[repr(C)]
    pub struct ScoreDiagTestControl {
        _private: [u8; 0],
        _marker: PhantomData<(*mut u8, PhantomPinned)>,
    }

    unsafe extern "C" {
        fn score_diag_test_control_create() -> *mut ScoreDiagTestControl;
        fn score_diag_test_control_register(control: *const ScoreDiagTestControl) -> *mut ScoreDiagReader;
        fn score_diag_test_control_started(control: *const ScoreDiagTestControl) -> usize;
        fn score_diag_test_control_stopped(control: *const ScoreDiagTestControl) -> usize;
        fn score_diag_test_control_complete(control: *mut ScoreDiagTestControl);
        fn score_diag_test_control_release(control: *mut ScoreDiagTestControl);
    }

    /// C++ reader whose reads stay pending until `complete`, so tests wait for state rather than time.
    pub struct Control(NonNull<ScoreDiagTestControl>);

    // SAFETY: the C++ control serializes all access behind a mutex.
    unsafe impl Send for Control {}
    unsafe impl Sync for Control {}

    impl Drop for Control {
        fn drop(&mut self) {
            unsafe { score_diag_test_control_release(self.0.as_ptr()) };
        }
    }

    impl Control {
        pub fn new() -> Self {
            Self(NonNull::new(unsafe { score_diag_test_control_create() }).expect("control"))
        }

        pub fn reader(&self, timeout: Duration) -> ReadDid {
            unsafe { ReadDid::from_registration(score_diag_test_control_register(self.0.as_ptr()), timeout) }
                .expect("controlled reader")
        }

        pub fn started(&self) -> usize {
            unsafe { score_diag_test_control_started(self.0.as_ptr()) }
        }

        pub fn stopped(&self) -> usize {
            unsafe { score_diag_test_control_stopped(self.0.as_ptr()) }
        }

        pub fn complete(&self) {
            unsafe { score_diag_test_control_complete(self.0.as_ptr()) }
        }
    }

    /// Polls a condition driven by C++ worker threads. The deadline only bounds a hung test.
    pub async fn wait_until(condition: impl Fn() -> bool) {
        let deadline = tokio::time::Instant::now() + Duration::from_secs(30);
        while !condition() {
            assert!(tokio::time::Instant::now() < deadline, "condition not reached");
            tokio::time::sleep(Duration::from_millis(1)).await;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::test_support::{wait_until, Control};
    use super::*;

    #[test]
    fn status_values_match_c_header() {
        for (raw, status) in [
            (0, ReadStatus::Ok),
            (1, ReadStatus::Nrc),
            (2, ReadStatus::Cancelled),
            (3, ReadStatus::Failed),
            (4, ReadStatus::Busy),
        ] {
            assert_eq!(ReadStatus::from_raw(raw), Some(status));
        }
        assert_eq!(ReadStatus::from_raw(5), None);
        assert_eq!(std::mem::size_of::<ReadStatus>(), 1);
    }

    // # req-Id: comp_req__uds_data_bridge__nrc_errors
    #[test]
    fn read_errors_keep_nrc_and_map_out_of_range_to_not_found() {
        assert!(matches!(DataError::from(ReadError::Nrc(0x31)), DataError::NotFound(m) if m == "UDS NRC 0x31"));
        assert!(matches!(DataError::from(ReadError::Nrc(0x33)), DataError::Internal(m) if m == "UDS NRC 0x33"));
        assert!(matches!(DataError::from(ReadError::Busy), DataError::Internal(_)));
    }

    #[tokio::test(flavor = "current_thread")]
    async fn accepts_owned_registration_after_original_handle_is_released() {
        let original = unsafe { score_diag_demo_reader_create(0, 0) };
        assert!(!original.is_null());
        let cloned = unsafe { score_diag_reader_clone(original) };
        unsafe { score_diag_reader_release(original) };
        let reader = unsafe { ReadDid::from_registration(cloned, Duration::from_secs(5)) }.expect("registration");
        assert_eq!(reader.read_bytes().await.expect("read"), b"1.0.0");
        assert!(unsafe { ReadDid::from_registration(std::ptr::null_mut(), Duration::from_secs(1)) }.is_err());
    }

    // # req-Id: comp_req__uds_data_bridge__read_handlers, comp_req__uds_data_bridge__nrc_errors
    #[tokio::test(flavor = "current_thread")]
    async fn cpp_handler_returns_bytes_and_nrc() {
        let reader = ReadDid::demo(0, 0, Duration::from_secs(5)).expect("reader");
        assert_eq!(reader.read_bytes().await.expect("read"), b"1.0.0");
        let denied = ReadDid::demo(0, 0x33, Duration::from_secs(5)).expect("denied reader");
        assert_eq!(denied.read_bytes().await, Err(ReadError::Nrc(0x33)));
    }

    // # req-Id: comp_req__uds_data_bridge__utf8_data
    #[tokio::test(flavor = "current_thread")]
    async fn utf8_payload_renders_as_string() {
        let reader = ReadDid::demo(0, 0, Duration::from_secs(5)).expect("reader");
        assert_eq!(Utf8(reader).read().await.expect("utf8").value, "1.0.0");
    }

    // # req-Id: comp_req__uds_data_bridge__utf8_data
    #[test]
    fn non_utf8_payload_is_an_error() {
        assert!(matches!(utf8_value(vec![0xff, 0xfe]), Err(DataError::Internal(_))));
    }

    // # req-Id: comp_req__uds_data_bridge__non_blocking_reads
    #[tokio::test(flavor = "current_thread")]
    async fn pending_read_does_not_block_runtime() {
        let control = Control::new();
        let pending = control.reader(Duration::from_secs(30));
        let task = tokio::spawn(async move { pending.read_bytes().await });
        wait_until(|| control.started() == 1).await;
        let quick = ReadDid::demo(0, 0, Duration::from_secs(5)).expect("quick reader");
        assert_eq!(quick.read_bytes().await.expect("quick read"), b"1.0.0");
        assert!(!task.is_finished());
        control.complete();
        assert_eq!(task.await.expect("task").expect("pending read"), b"ok");
    }

    // # req-Id: comp_req__uds_data_bridge__read_timeout
    #[tokio::test(flavor = "current_thread")]
    async fn timeout_and_dropped_read_cancel_safely() {
        let control = Control::new();
        let reader = control.reader(Duration::from_millis(10));
        assert_eq!(reader.read_bytes().await, Err(ReadError::Timeout));
        wait_until(|| control.stopped() == 1).await;
        let pending = control.reader(Duration::from_secs(30));
        let task = tokio::spawn(async move { pending.read_bytes().await });
        wait_until(|| control.started() == 2).await;
        task.abort();
        assert!(task.await.expect_err("aborted task").is_cancelled());
        wait_until(|| control.stopped() == 2).await;
        let quick = ReadDid::demo(0, 0, Duration::from_secs(5)).expect("quick reader");
        assert_eq!(quick.read_bytes().await.expect("subsequent read"), b"1.0.0");
    }

    // # req-Id: comp_req__uds_data_bridge__read_admission
    #[tokio::test(flavor = "current_thread")]
    async fn saturated_handler_rejects_additional_work() {
        let control = Control::new();
        let reader = control.reader(Duration::from_secs(30));
        let mut tasks = Vec::new();
        for _ in 0..8 {
            let reader = reader.clone();
            tasks.push(tokio::spawn(async move { reader.read_bytes().await }));
        }
        wait_until(|| control.started() == 8).await;
        assert_eq!(reader.read_bytes().await, Err(ReadError::Busy));
        control.complete();
        for task in tasks {
            assert_eq!(task.await.expect("task").expect("read"), b"ok");
        }
    }
}
