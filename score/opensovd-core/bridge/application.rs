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
use std::ffi::{c_char, c_void, CStr};
use std::ptr::NonNull;
use std::time::Duration;

use async_trait::async_trait;
use opensovd_core::{App, DataError};
use opensovd_models::data::DataCategory;
use opensovd_providers::data::{DataProviderBuilder, ReadableDataResource, Value};

use crate::read_did::ReadDid;

unsafe extern "C" {
    fn score_diag_application_id(application: *const c_void) -> *const c_char;
    fn score_diag_application_name(application: *const c_void) -> *const c_char;
    fn score_diag_application_component(application: *const c_void) -> *const c_char;
    fn score_diag_application_resource_count(application: *const c_void) -> usize;
    fn score_diag_application_resource_id(application: *const c_void, index: usize) -> *const c_char;
    fn score_diag_application_resource_name(application: *const c_void, index: usize) -> *const c_char;
    fn score_diag_application_resource_encoding(application: *const c_void, index: usize) -> u8;
    fn score_diag_application_resource_reader(application: *const c_void, index: usize) -> *mut c_void;
    fn score_diag_application_release(application: *mut c_void);
    fn score_diag_sensor_source_create() -> *mut c_void;
    fn score_diag_sensor_source_publish(source: *mut c_void, temperature: i16, available: u8);
    fn score_diag_sensor_source_register(source: *const c_void) -> *mut c_void;
    fn score_diag_sensor_source_release(source: *mut c_void);
}

unsafe fn text(pointer: *const c_char) -> Result<String, DataError> {
    if pointer.is_null() {
        return Err(DataError::Internal("missing application metadata".to_owned()));
    }
    unsafe { CStr::from_ptr(pointer) }
        .to_str()
        .map(str::to_owned)
        .map_err(|_| DataError::Internal("application metadata is not UTF-8".to_owned()))
}

pub struct RegisteredApplication(NonNull<c_void>);

impl Drop for RegisteredApplication {
    fn drop(&mut self) {
        unsafe { score_diag_application_release(self.0.as_ptr()) };
    }
}

impl RegisteredApplication {
    /// Takes exclusive ownership of a C++ application registration.
    ///
    /// # Safety
    /// A non-null handle must be a live registration from RegisterApplication, relinquished by its owner.
    pub unsafe fn from_registration(handle: *mut c_void) -> Result<Self, DataError> {
        NonNull::new(handle)
            .map(Self)
            .ok_or_else(|| DataError::Internal("application registration failed".to_owned()))
    }

    pub fn into_app(self, timeout: Duration) -> Result<App, DataError> {
        let handle = self.0.as_ptr();
        let id = unsafe { text(score_diag_application_id(handle)) }?;
        let name = unsafe { text(score_diag_application_name(handle)) }?;
        let component = unsafe { text(score_diag_application_component(handle)) }?;
        let count = unsafe { score_diag_application_resource_count(handle) };
        let mut provider = DataProviderBuilder::new();
        for index in 0..count {
            let id = unsafe { text(score_diag_application_resource_id(handle, index)) }?;
            let name = unsafe { text(score_diag_application_resource_name(handle, index)) }?;
            let reader =
                unsafe { ReadDid::from_registration(score_diag_application_resource_reader(handle, index), timeout) }?;
            provider = match unsafe { score_diag_application_resource_encoding(handle, index) } {
                0 => provider.read_data(&id, &name, &DataCategory::CurrentData, reader),
                1 => provider.read_data(&id, &name, &DataCategory::CurrentData, Temperature(reader)),
                2 => provider.read_data(&id, &name, &DataCategory::CurrentData, Healthy(reader)),
                _ => return Err(DataError::Internal("unsupported resource encoding".to_owned())),
            };
        }
        let provider = provider
            .build()
            .map_err(|error| DataError::Internal(error.to_string()))?;
        Ok(App::new(id, name, component).with_data_provider(provider))
    }
}

struct Temperature(ReadDid);

#[async_trait]
impl ReadableDataResource for Temperature {
    type Value = Value<f64>;

    async fn read(&self) -> Result<Self::Value, DataError> {
        let bytes = self.0.read().await?.value;
        let bytes: [u8; 2] = bytes
            .try_into()
            .map_err(|_| DataError::Internal("invalid temperature payload".to_owned()))?;
        Ok(Value::new(f64::from(i16::from_be_bytes(bytes)) / 100.0))
    }
}

struct Healthy(ReadDid);

#[async_trait]
impl ReadableDataResource for Healthy {
    type Value = Value<bool>;

    async fn read(&self) -> Result<Self::Value, DataError> {
        match self.0.read().await?.value.as_slice() {
            [0] => Ok(Value::new(false)),
            [1] => Ok(Value::new(true)),
            _ => Err(DataError::Internal("invalid health payload".to_owned())),
        }
    }
}

pub struct SensorSource(NonNull<c_void>);

impl Drop for SensorSource {
    fn drop(&mut self) {
        unsafe { score_diag_sensor_source_release(self.0.as_ptr()) };
    }
}

impl SensorSource {
    pub fn new() -> Result<Self, DataError> {
        NonNull::new(unsafe { score_diag_sensor_source_create() })
            .map(Self)
            .ok_or_else(|| DataError::Internal("sensor source initialization failed".to_owned()))
    }

    pub fn publish(&self, centidegrees: i16, available: bool) {
        unsafe { score_diag_sensor_source_publish(self.0.as_ptr(), centidegrees, u8::from(available)) };
    }

    pub fn register(&self) -> Result<RegisteredApplication, DataError> {
        unsafe { RegisteredApplication::from_registration(score_diag_sensor_source_register(self.0.as_ptr())) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test(flavor = "current_thread")]
    async fn typed_resources_reject_incompatible_payloads() {
        let temperature = Temperature(ReadDid::demo(0, 0, Duration::from_secs(1)).expect("reader"));
        assert!(
            matches!(temperature.read().await, Err(DataError::Internal(error)) if error == "invalid temperature payload")
        );
        let health = Healthy(ReadDid::demo(0, 0, Duration::from_secs(1)).expect("reader"));
        assert!(matches!(health.read().await, Err(DataError::Internal(error)) if error == "invalid health payload"));
        assert!(unsafe { RegisteredApplication::from_registration(std::ptr::null_mut()) }.is_err());
    }
}
