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

use opensovd_core::{App, Component};
use opensovd_models::data::DataCategory;
use opensovd_providers::data::{Constant, DataProviderBuilder};
use opensovd_server::{Server, Topology};
use std::time::Duration;
use tokio::net::TcpListener;
use tracing_subscriber::EnvFilter;

#[path = "../bridge/read_did.rs"]
mod read_did;

const DEFAULT_ADDRESS: &str = "127.0.0.1:7690";

fn address() -> String {
    std::env::var("SCORE_GATEWAY_ADDRESS").unwrap_or_else(|_| DEFAULT_ADDRESS.to_owned())
}

fn init_tracing() {
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));
    tracing_subscriber::fmt().with_env_filter(filter).init();
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

    let app_provider = DataProviderBuilder::new()
        .read_data(
            "application.version",
            "Application Version",
            &DataCategory::IdentData,
            read_did::ReadDid::demo(0, 0, Duration::from_secs(2))?,
        )
        .build()?;

    let topology = Topology::new();
    {
        let mut registry = topology.write().await;
        registry
            .add_component(Component::new("score-demo", "SCORE Demo").with_data_provider(provider));
        registry.add_app(
            App::new("score-diagnostics", "SCORE Diagnostics")
                .with_component_id("score-demo")
                .with_data_provider(app_provider),
        );
    }
    Ok(topology)
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    init_tracing();
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

#[cfg(test)]
mod tests {
    use super::*;
    use opensovd_client::Client;
    use std::io::{self, Write};
    use std::sync::{Arc, Mutex};

    struct LogWriter(Arc<Mutex<Vec<u8>>>);

    impl Write for LogWriter {
        fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
            self.0.lock().expect("log buffer").extend_from_slice(bytes);
            Ok(bytes.len())
        }

        fn flush(&mut self) -> io::Result<()> {
            Ok(())
        }
    }

    #[tokio::test(flavor = "current_thread")]
    async fn upstream_internal_errors_are_logged() {
        let output = Arc::new(Mutex::new(Vec::new()));
        let buffer = output.clone();
        let subscriber = tracing_subscriber::fmt()
            .with_writer(move || LogWriter(buffer.clone()))
            .without_time()
            .finish();
        let _guard = tracing::subscriber::set_default(subscriber);
        let listener = TcpListener::bind("127.0.0.1:0").await.expect("listener");
        let address = listener.local_addr().expect("address");
        let provider = DataProviderBuilder::new()
            .read_data(
                "denied",
                "Denied",
                &DataCategory::IdentData,
                read_did::ReadDid::demo(0, 0x33, Duration::from_secs(1)).expect("reader"),
            )
            .build()
            .expect("provider");
        let topology = Topology::new();
        topology
            .write()
            .await
            .add_component(Component::new("denied", "Denied").with_data_provider(provider));
        let server = Server::builder()
            .base_uri(format!("http://{address}/sovd"))
            .expect("base URI")
            .listener(listener)
            .topology(topology)
            .build()
            .expect("server");
        let server_task = tokio::spawn(async move { server.serve().await });
        let client = Client::builder()
            .base_uri(format!("http://{address}/sovd/v1"))
            .expect("URL")
            .timeout(Duration::from_secs(1))
            .build()
            .expect("client");
        let result = client
            .component("denied")
            .data("denied")
            .read()
            .send()
            .await;
        server_task.abort();
        let _ = server_task.await;
        assert!(
            matches!(result, Err(opensovd_client::Error::ApiError { status, .. }) if status.as_u16() == 500)
        );
        let logs =
            String::from_utf8(output.lock().expect("log buffer").clone()).expect("UTF-8 logs");
        assert!(
            logs.contains("UDS NRC 0x33"),
            "internal error detail missing: {logs}"
        );
    }

    #[tokio::test(flavor = "current_thread")]
    async fn registered_application_is_discoverable_and_serves_data() {
        let listener = TcpListener::bind("127.0.0.1:0").await.expect("listener");
        let address = listener.local_addr().expect("address");
        let server = Server::builder()
            .base_uri(format!("http://{address}/sovd"))
            .expect("base URI")
            .listener(listener)
            .topology(topology().await.expect("topology"))
            .build()
            .expect("server");
        let server_task = tokio::spawn(async move { server.serve().await });
        let url = format!("http://{address}/sovd/v1");
        let client = Client::builder()
            .base_uri(&url)
            .expect("URL")
            .timeout(Duration::from_secs(1))
            .build()
            .expect("client");
        let apps = client.list_apps().send().await.expect("list apps");
        assert!(apps.items.iter().any(|app| app.id == "score-diagnostics"));
        let app = client.app("score-diagnostics");
        let location = app.located_on().await.expect("app location");
        assert!(location.is_some_and(|component| component.id == "score-demo"));
        let version = app
            .data("application.version")
            .read()
            .send()
            .await
            .expect("app version");
        let bytes = version.data["value"].as_array().expect("byte array");
        let bytes: Vec<u8> = bytes
            .iter()
            .map(|value| value.as_u64().expect("byte") as u8)
            .collect();
        assert_eq!(bytes, b"1.0.0");
        server_task.abort();
        let _ = server_task.await;
    }
}
