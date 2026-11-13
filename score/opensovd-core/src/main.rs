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
use opensovd_extra::{Federation, FederationLayer};
use opensovd_models::data::DataCategory;
use opensovd_providers::data::{Constant, DataProviderBuilder};
use opensovd_server::{Server, Topology, VendorInfo};
use std::time::Duration;
use tokio::net::TcpListener;
use tower::layer::util::{Identity, Stack};
use tower::util::Either;
use tracing_subscriber::EnvFilter;

#[path = "../bridge/read_did.rs"]
mod read_did;

mod upstream;

const DEFAULT_ADDRESS: &str = "127.0.0.1:7690";
const BASE_PATH: &str = "/sovd";

fn address() -> String {
    std::env::var("SCORE_GATEWAY_ADDRESS").unwrap_or_else(|_| DEFAULT_ADDRESS.to_owned())
}

/// Base URL of the SOVD server the gateway forwards to, such as the CDA at
/// `http://127.0.0.1:20002/vehicle/v15`. Without it the gateway serves only its own entities.
fn upstream_url() -> Option<String> {
    std::env::var("SCORE_UPSTREAM_URL").ok().filter(|url| !url.is_empty())
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

type Gateway = Server<VendorInfo, Stack<Either<FederationLayer, Identity>, Identity>>;

/// The gateway on `listener`. With `upstream_url`, what it does not host is forwarded there and
/// discovery lists the upstream's entities next to its own.
fn gateway(
    listener: TcpListener,
    topology: Topology,
    upstream_url: Option<&str>,
) -> Result<Gateway, Box<dyn std::error::Error>> {
    let address = listener.local_addr()?;
    let federation = match upstream_url {
        Some(url) => {
            tracing::info!(target: "forwarding", "Forwarding to {url}");
            let upstream = upstream::upstream(url)?;
            Some(
                Federation::new(vec![upstream], topology.clone())?
                    .base(BASE_PATH)
                    .into_layer(),
            )
        },
        None => None,
    };
    let server = Server::builder()
        .layer(tower::util::option_layer(federation))
        .base_uri(format!("http://{address}{BASE_PATH}"))?
        .listener(listener)
        .topology(topology)
        .build()?;
    Ok(server)
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    init_tracing();
    let listener = TcpListener::bind(address()).await?;
    let server = gateway(listener, topology().await?, upstream_url().as_deref())?;
    server.serve().await?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use opensovd_client::Client;

    async fn serve(upstream_url: Option<&str>) -> (tokio::task::JoinHandle<()>, Client) {
        let listener = TcpListener::bind("127.0.0.1:0").await.expect("listener");
        let address = listener.local_addr().expect("address");
        let server = gateway(listener, topology().await.expect("topology"), upstream_url).expect("server");
        let task = tokio::spawn(async move {
            let _ = server.serve().await;
        });
        let client = Client::builder()
            .base_uri(&format!("http://{address}/sovd/v1"))
            .expect("URL")
            .timeout(Duration::from_secs(5))
            .build()
            .expect("client");
        (task, client)
    }

    // # req-Id: comp_req__uds_data_bridge__read_handlers, comp_req__uds_data_bridge__utf8_data, comp_req__sovd_gateway__hosted_entities
    #[tokio::test(flavor = "current_thread")]
    async fn registered_application_is_discoverable_and_serves_data() {
        let (task, client) = serve(None).await;
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

    // # req-Id: comp_req__sovd_gateway__upstream_unavailable, comp_req__sovd_gateway__upstream_config
    #[tokio::test(flavor = "current_thread")]
    async fn unreachable_upstream_keeps_local_entities_available() {
        let unused = std::net::TcpListener::bind("127.0.0.1:0").expect("port");
        let url = format!("http://{}/vehicle/v15", unused.local_addr().expect("address"));
        drop(unused);
        let (task, client) = serve(Some(&url)).await;
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
