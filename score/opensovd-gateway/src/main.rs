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


use opensovd_core::Component;
use opensovd_models::data::DataCategory;
use opensovd_providers::data::{Constant, DataProviderBuilder};
use opensovd_server::{Server, Topology};
use tokio::net::TcpListener;

const DEFAULT_ADDRESS: &str = "127.0.0.1:7690";

fn address() -> String {
    std::env::var("SCORE_GATEWAY_ADDRESS").unwrap_or_else(|_| DEFAULT_ADDRESS.to_owned())
}

async fn topology() -> Result<Topology, Box<dyn std::error::Error>> {
    let provider = DataProviderBuilder::new()
        .read_data(
            "demo.version",
            "Demo Version",
            &DataCategory::IdentData,
            Constant::new("1.0.0")?,
        )
        .read_data(
            "demo.status",
            "Demo Status",
            &DataCategory::CurrentData,
            Constant::new("ready")?,
        )
        .build()?;

    let topology = Topology::new();
    topology
        .write()
        .await
        .add_component(Component::new("score-demo", "SCORE Demo").with_data_provider(provider));
    Ok(topology)
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let address = address();
    let listener = TcpListener::bind(&address).await?;
    let topology = topology().await?;
    let server = Server::builder()
        .base_uri(format!("http://{address}/sovd"))?
        .listener(listener)
        .topology(topology)
        .build()?;
    server.serve().await?;
    Ok(())
}
