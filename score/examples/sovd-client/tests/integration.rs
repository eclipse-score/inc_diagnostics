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

use opensovd_client::Client;
use opensovd_core::Component;
use opensovd_models::data::DataCategory;
use opensovd_providers::data::{Constant, DataProviderBuilder};
use opensovd_server::{Server, Topology};
use tokio::net::TcpListener;
use tokio::time::{Duration, sleep};

async fn demo_topology() -> Topology {
    let provider = DataProviderBuilder::new()
        .read_data(
            "demo.version",
            "Demo Version",
            &DataCategory::IdentData,
            Constant::new("1.0.0").expect("constant"),
        )
        .build()
        .expect("data provider");
    let topology = Topology::new();
    topology
        .write()
        .await
        .add_component(Component::new("score-demo", "SCORE Demo").with_data_provider(provider));
    topology
}

#[tokio::test(flavor = "current_thread")]
async fn client_lists_demo_component() {
    let listener = TcpListener::bind("127.0.0.1:0").await.expect("listener");
    let address = listener.local_addr().expect("local address");
    let server = Server::builder()
        .base_uri(format!("http://{address}/sovd"))
        .expect("base URI")
        .listener(listener)
        .topology(demo_topology().await)
        .build()
        .expect("server");
    let port = address.port();

    tokio::spawn(async move {
        server.serve().await.expect("server task");
    });

    let url = format!("http://127.0.0.1:{port}/sovd/v1");
    let mut last_error = None;
    for _ in 0..40 {
        match Client::connect(&url)
            .expect("client")
            .list_components()
            .send()
            .await
        {
            Ok(response) => {
                assert!(response.data.items.iter().any(|c| c.id == "score-demo"));
                return;
            }
            Err(error) => last_error = Some(error),
        }
        sleep(Duration::from_millis(25)).await;
    }
    panic!("server did not become reachable: {last_error:?}");
}
