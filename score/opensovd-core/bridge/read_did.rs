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
use std::ptr::NonNull;
use std::sync::Arc;
use std::time::Duration;

use async_trait::async_trait;
use opensovd_core::DataError;
use opensovd_providers::data::{ReadableDataResource, Value};
use tokio::sync::oneshot;

unsafe extern "C" {
    fn score_diag_demo_reader_create(delay_ms: u32, nrc: u8) -> *mut c_void;
    #[cfg(test)]
    fn score_diag_reader_clone(reader: *const c_void) -> *mut c_void;
    fn score_diag_reader_release(reader: *mut c_void);
    fn score_diag_read_start(
        reader: *mut c_void,
        context: *mut c_void,
        completion: unsafe extern "C" fn(*mut c_void, u8, u8, *const u8, usize),
    ) -> *mut c_void;
    fn score_diag_read_cancel(request: *mut c_void);
    fn score_diag_read_release(request: *mut c_void);
}

type Reply = Result<Vec<u8>, DataError>;

struct Handler(NonNull<c_void>);

unsafe impl Send for Handler {}
unsafe impl Sync for Handler {}

impl Drop for Handler {
    fn drop(&mut self) {
        unsafe { score_diag_reader_release(self.0.as_ptr()) };
    }
}

struct Request(NonNull<c_void>);

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
    let reply = match status {
        0 if size == 0 => Ok(Vec::new()),
        0 => Ok(unsafe { std::slice::from_raw_parts(data, size) }.to_vec()),
        1 => Err(DataError::Internal(format!("UDS NRC 0x{nrc:02X}"))),
        2 => Err(DataError::Internal("diagnostic read cancelled".to_owned())),
        4 => Err(DataError::Internal("diagnostic reader busy".to_owned())),
        _ => Err(DataError::Internal("diagnostic read failed".to_owned())),
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
    pub unsafe fn from_registration(handle: *mut c_void, timeout: Duration) -> Result<Self, DataError> {
        let handler =
            NonNull::new(handle).ok_or_else(|| DataError::Internal("could not register C++ handler".to_owned()))?;
        Ok(Self {
            handler: Arc::new(Handler(handler)),
            timeout,
        })
    }
}

#[async_trait]
impl ReadableDataResource for ReadDid {
    type Value = Value<Vec<u8>>;

    async fn read(&self) -> Result<Self::Value, DataError> {
        let (sender, receiver) = oneshot::channel::<Reply>();
        let context = Box::into_raw(Box::new(sender)).cast();
        let request = unsafe { score_diag_read_start(self.handler.0.as_ptr(), context, complete) };
        let request = NonNull::new(request).map(Request);
        let result = tokio::time::timeout(self.timeout, receiver)
            .await
            .map_err(|_| DataError::Internal("diagnostic read timed out".to_owned()))?
            .map_err(|_| DataError::Internal("diagnostic completion unavailable".to_owned()))?;
        drop(request);
        result.map(Value::new)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test(flavor = "current_thread")]
    async fn accepts_owned_registration_after_original_handle_is_released() {
        let original = unsafe { score_diag_demo_reader_create(0, 0) };
        assert!(!original.is_null());
        let cloned = unsafe { score_diag_reader_clone(original) };
        unsafe { score_diag_reader_release(original) };
        let reader = unsafe { ReadDid::from_registration(cloned, Duration::from_secs(1)) }.expect("registration");
        assert_eq!(reader.read().await.expect("read").value, b"1.0.0");
        assert!(unsafe { ReadDid::from_registration(std::ptr::null_mut(), Duration::from_secs(1)) }.is_err());
    }

    #[tokio::test(flavor = "current_thread")]
    async fn cpp_handler_returns_bytes_and_nrc() {
        let reader = ReadDid::demo(0, 0, Duration::from_secs(1)).expect("reader");
        assert_eq!(reader.read().await.expect("read").value, b"1.0.0");
        let denied = ReadDid::demo(0, 0x33, Duration::from_secs(1)).expect("denied reader");
        assert!(matches!(denied.read().await, Err(DataError::Internal(error)) if error == "UDS NRC 0x33"));
    }

    #[tokio::test(flavor = "current_thread")]
    async fn delayed_read_does_not_block_runtime() {
        let delayed = ReadDid::demo(200, 0, Duration::from_secs(1)).expect("delayed reader");
        let task = tokio::spawn(async move { delayed.read().await });
        tokio::task::yield_now().await;
        let quick = ReadDid::demo(0, 0, Duration::from_secs(1)).expect("quick reader");
        let result = tokio::time::timeout(Duration::from_millis(100), quick.read()).await;
        assert_eq!(result.expect("runtime responsive").expect("quick read").value, b"1.0.0");
        assert!(!task.is_finished());
        assert_eq!(task.await.expect("task").expect("delayed read").value, b"1.0.0");
    }

    #[tokio::test(flavor = "current_thread")]
    async fn timeout_and_dropped_read_cancel_safely() {
        let reader = ReadDid::demo(200, 0, Duration::from_millis(10)).expect("reader");
        assert!(matches!(reader.read().await, Err(DataError::Internal(error)) if error == "diagnostic read timed out"));
        let delayed = ReadDid::demo(200, 0, Duration::from_secs(1)).expect("delayed reader");
        let task = tokio::spawn(async move { delayed.read().await });
        tokio::task::yield_now().await;
        task.abort();
        assert!(task.await.expect_err("aborted task").is_cancelled());
        let quick = ReadDid::demo(0, 0, Duration::from_secs(1)).expect("quick reader");
        assert_eq!(quick.read().await.expect("subsequent read").value, b"1.0.0");
    }

    #[tokio::test(flavor = "current_thread")]
    async fn saturated_handler_rejects_additional_work() {
        let reader = ReadDid::demo(500, 0, Duration::from_secs(2)).expect("reader");
        let mut tasks = Vec::new();
        for _ in 0..8 {
            let reader = reader.clone();
            tasks.push(tokio::spawn(async move { reader.read().await }));
        }
        tokio::task::yield_now().await;
        assert!(matches!(reader.read().await, Err(DataError::Internal(error)) if error == "diagnostic reader busy"));
        for task in tasks {
            task.abort();
            assert!(task.await.expect_err("aborted read").is_cancelled());
        }
    }
}
