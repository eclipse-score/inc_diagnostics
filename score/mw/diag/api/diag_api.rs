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

pub type KeyValueAttributes = ::common::KeyValueAttributes;
pub type ReplyMessageEncoding = ::common::ReplyMessageEncoding;
pub type ReplyMessagePayload = ::common::ReplyMessagePayload;
pub type RequestMessagePayload = ::common::RequestMessagePayload;
pub type DiagnosticReply = ::common::DiagnosticReply;
pub type ByteVector = ::common::ByteVector;
pub type Result<T> = ::common::Result<T>;
pub type Error = ::common::Error;

pub mod sovd {
    pub use common::sovd::*;

    pub use data_resource::DataResource; // for users' convenience

    pub mod data_resource {
        pub use data_resource::sovd::*;
        pub use data_resource::*;
    }

    pub use operation::Operation; // for users' convenience

    pub mod operation {
        pub use operation::*;
    }
}
