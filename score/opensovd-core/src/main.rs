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

//! The OpenSOVD gateway of S-CORE: the gateway of opensovd-core with the entities of S-CORE.
//!
//! It takes the options and `SOVD_*` environment variables of the reference gateway
//! (`opensovd-gateway --help` of opensovd-core), such as `SOVD_URL`, `SOVD_UPSTREAM_URL` for the CDA
//! and `SOVD_MDNS` for the mDNS/DNS-SD announcement. Upstreams, authentication, TLS, mDNS and
//! shutdown on SIGTERM or Ctrl-C are the ones of opensovd-core.

use opensovd_core::{App, Component, Topology};
use opensovd_models::data::DataCategory;
use opensovd_providers::data::{Constant, DataProviderBuilder};
use std::process::ExitCode;
use std::time::Duration;
use tracing_subscriber::EnvFilter;

#[path = "../bridge/read_did.rs"]
mod read_did;

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
            read_did::Utf8(read_did::ReadDid::demo(0, 0, Duration::from_secs(2))?),
        )
        .build()?;

    let topology = Topology::new();
    {
        let mut registry = topology.write().await;
        registry.add_component(Component::new("score-demo", "SCORE Demo").with_data_provider(provider));
        registry.add_app(
            App::new("score-diagnostics", "SCORE Diagnostics")
                .with_component_id("score-demo")
                .with_data_provider(app_provider),
        );
    }
    Ok(topology)
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> ExitCode {
    let options = opensovd_gateway::cli::parse();
    init_tracing();
    let topology = match topology().await {
        Ok(topology) => topology,
        Err(error) => {
            tracing::error!("Cannot set up the topology: {error}");
            return ExitCode::FAILURE;
        },
    };
    match opensovd_gateway::run(options, topology).await {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            tracing::error!("{error:#}");
            ExitCode::FAILURE
        },
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opensovd_client::Client;

    /// Runs the gateway on a free port of the loopback address with the extra `options`, such as
    /// `--upstream-url`, until the returned task is aborted.
    async fn serve(options: &[&str]) -> (tokio::task::JoinHandle<()>, Client) {
        let address = std::net::TcpListener::bind("127.0.0.1:0")
            .and_then(|listener| listener.local_addr())
            .expect("free port");
        let url = format!("http://{address}/sovd");
        let options = opensovd_gateway::cli::try_parse_from(["opensovd-gateway", "--url", url.as_str()].into_iter().chain(options.iter().copied()))
            .expect("options");
        let topology = topology().await.expect("topology");
        let task = tokio::spawn(async move {
            let _ = opensovd_gateway::run_with_shutdown(options, topology, std::future::pending()).await;
        });
        let client = Client::builder()
            .base_uri(&format!("{url}/v1"))
            .expect("URL")
            .timeout(Duration::from_secs(5))
            .build()
            .expect("client");
        for _ in 0..100 {
            if client.list_apps().send().await.is_ok() {
                break;
            }
            tokio::time::sleep(Duration::from_millis(20)).await;
        }
        (task, client)
    }

    #[tokio::test(flavor = "current_thread")]
    async fn registered_application_is_discoverable_and_serves_data() {
        let (task, client) = serve(&[]).await;
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
        assert_eq!(version.data["value"].as_str(), Some("1.0.0"));
        task.abort();
        let _ = task.await;
    }

    /// A fake CDA on a Unix domain socket: every request gets one component, with an href to the
    /// CDA's own API, as the CDA sends them.
    fn serve_socket_upstream(path: &std::path::Path) -> tokio::task::JoinHandle<()> {
        use tokio::io::{AsyncReadExt, AsyncWriteExt};
        let listener = tokio::net::UnixListener::bind(path).expect("socket");
        tokio::spawn(async move {
            while let Ok((mut stream, _)) = listener.accept().await {
                tokio::spawn(async move {
                    let mut request = Vec::new();
                    let mut buffer = [0u8; 1024];
                    while !request.windows(4).any(|window| window == b"\r\n\r\n") {
                        match stream.read(&mut buffer).await {
                            Ok(0) | Err(_) => return,
                            Ok(read) => request.extend_from_slice(&buffer[..read]),
                        }
                    }
                    let body = r#"{"items":[{"id":"ecu1","name":"ecu1","href":"http://localhost/Vehicle/v15/components/ecu1"}]}"#;
                    let response = format!(
                        "HTTP/1.1 200 OK\r\ncontent-type: application/json\r\ncontent-length: {}\r\nconnection: close\r\n\r\n{body}",
                        body.len()
                    );
                    let _ = stream.write_all(response.as_bytes()).await;
                });
            }
        })
    }

    #[tokio::test(flavor = "current_thread")]
    async fn upstream_entities_are_listed_over_a_unix_socket() {
        let dir = std::env::temp_dir().join(format!("score-gateway-test-{}", std::process::id()));
        std::fs::create_dir_all(&dir).expect("directory");
        let path = dir.join("cda.sock");
        let _ = std::fs::remove_file(&path);
        let cda = serve_socket_upstream(&path);
        let url = format!("unix:{}:/vehicle/v15", path.display());
        let (task, client) = serve(&["--upstream-url", &url]).await;
        let components = client.list_components().send().await.expect("list components");
        let ecu = components.items.iter().find(|component| component.id == "ecu1");
        let ecu = ecu.unwrap_or_else(|| panic!("ecu1 not listed: {components:?}"));
        let href = &ecu.href.0;
        assert!(
            href.starts_with("http://127.0.0.1:") && href.ends_with("/sovd/v1/components/ecu1"),
            "href not rewritten to the gateway: {href}"
        );
        task.abort();
        cda.abort();
        let _ = std::fs::remove_dir_all(&dir);
    }

    #[tokio::test(flavor = "current_thread")]
    async fn unreachable_upstream_keeps_local_entities_available() {
        let unused = std::net::TcpListener::bind("127.0.0.1:0").expect("port");
        let url = format!("http://{}/vehicle/v15", unused.local_addr().expect("address"));
        drop(unused);
        let (task, client) = serve(&["--upstream-url", &url]).await;
        let apps = client.list_apps().send().await.expect("list apps");
        assert!(apps.items.iter().any(|app| app.id == "score-diagnostics"));
        let error = client
            .app("sovd2uds")
            .list_data()
            .send()
            .await
            .expect_err("forwarded request");
        assert!(
            matches!(&error, opensovd_client::Error::ApiError { status, .. } if status.as_u16() == 502),
            "{error:?}"
        );
        assert!(
            !format!("{error:?}").contains(&url),
            "upstream address leaked: {error:?}"
        );
        task.abort();
        let _ = task.await;
    }
}
