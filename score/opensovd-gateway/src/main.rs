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


/// SCORE opensovd-gateway binary.
///
/// MVP stub: links opensovd-core and parks the thread indefinitely.
/// Cross-cutting concerns (logging, persistency, config) are wired in later iterations.
fn main() {
    // Stub: keeps the process running without any logic.
    // Replace with async fn main + tokio runtime once SCORE cross-cutting concerns are wired.
    std::thread::park();
}
