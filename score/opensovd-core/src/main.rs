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
use opensovd_extra::Federation;
use opensovd_extra_mdns::Announcement;
use opensovd_models::data::DataCategory;
use opensovd_providers::data::{Constant, DataProviderBuilder};
use opensovd_server::{Server, Topology, VendorInfo};
use std::net::SocketAddr;
use std::time::Duration;
use tokio::net::TcpListener;
use tower::layer::util::Identity;
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

/// The mDNS/DNS-SD announcement of the gateway (ISO 17978-3 §5.11), or `None` when
/// `SCORE_GATEWAY_MDNS` is unset or empty. Its value is the vehicle identification, published in
/// the `identification` TXT record, e.g. the VIN. `SCORE_GATEWAY_MDNS_HOST` sets the host label
/// (`<host>.local`), `SCORE_GATEWAY_MDNS_INTERFACE` limits an unspecified address to the named,
/// comma separated interfaces. `env` reads a variable.
fn announcement(addr: SocketAddr, env: impl Fn(&str) -> Option<String>) -> Result<Option<Announcement>, String> {
    let set = |name: &str| env(name).filter(|value| !value.trim().is_empty());
    let Some(identification) = set("SCORE_GATEWAY_MDNS") else {
        return Ok(None);
    };
    if addr.ip().is_loopback() {
        return Err(format!(
            "SCORE_GATEWAY_MDNS cannot announce the loopback address {addr}: set SCORE_GATEWAY_ADDRESS to \
             0.0.0.0 or an interface address"
        ));
    }
    let mut announcement = Announcement::new(addr)
        .identification(identification)
        .access("http", BASE_PATH);
    if let Some(host) = set("SCORE_GATEWAY_MDNS_HOST") {
        announcement = announcement.host(host);
    }
    if let Some(interfaces) = set("SCORE_GATEWAY_MDNS_INTERFACE") {
        announcement = announcement.interfaces(
            interfaces
                .split(',')
                .map(str::trim)
                .filter(|name| !name.is_empty())
                .map(str::to_owned)
                .collect::<Vec<_>>(),
        );
    }
    Ok(Some(announcement))
}

/// Resolves on SIGTERM, which the launch manager sends to stop the gateway, or on Ctrl-C.
async fn shutdown_signal() {
    use tokio::signal::unix::{signal, SignalKind};
    match signal(SignalKind::terminate()) {
        Ok(mut terminate) => {
            tokio::select! {
                _ = terminate.recv() => {},
                _ = tokio::signal::ctrl_c() => {},
            }
        },
        Err(error) => {
            tracing::warn!("Cannot handle SIGTERM: {error}");
            let _ = tokio::signal::ctrl_c().await;
        },
    }
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

type Gateway = Server<VendorInfo, Identity>;

/// The gateway on `listener`. With `upstream_url`, what it does not host is forwarded there and
/// discovery lists the upstream's entities next to its own.
fn gateway(
    listener: TcpListener,
    topology: Topology,
    upstream_url: Option<&str>,
) -> Result<Gateway, Box<dyn std::error::Error>> {
    let address = listener.local_addr()?;
    let mut builder = Server::builder()
        .base_uri(format!("http://{address}{BASE_PATH}"))?
        .listener(listener)
        .topology(topology.clone());
    // An inner layer: only there does a request carry the base path of the server, which the
    // federation needs to tell API requests from others (opensovd-core#260). As an outer layer it
    // would pass every request to the gateway's own routes.
    if let Some(url) = upstream_url {
        tracing::info!(target: "forwarding", "Forwarding to {url}");
        let upstream = upstream::upstream(url)?;
        builder = builder.inner_layer(Federation::new(vec![upstream], topology)?.into_layer());
    }
    Ok(builder.build()?)
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    init_tracing();
    let listener = TcpListener::bind(address()).await?;
    let announcement = announcement(listener.local_addr()?, |name| std::env::var(name).ok())?;
    let server = gateway(listener, topology().await?, upstream_url().as_deref())?;
    // Announced once the server is set up, so a client that finds it can connect. Withdrawn with
    // a goodbye when the gateway stops, so clients do not wait for the records to expire.
    let announcer = match announcement {
        Some(announcement) => Some(announcement.announce().await?),
        None => None,
    };
    let result: Result<(), Box<dyn std::error::Error>> = tokio::select! {
        result = server.serve() => result.map_err(Into::into),
        () = shutdown_signal() => Ok(()),
    };
    if let Some(announcer) = announcer {
        announcer.shutdown().await;
    }
    result
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

    fn env<'a>(vars: &'a [(&'a str, &'a str)]) -> impl Fn(&str) -> Option<String> + 'a {
        move |name| vars.iter().find(|(key, _)| *key == name).map(|(_, value)| (*value).to_owned())
    }

    #[test]
    fn mdns_is_off_without_an_identification() {
        let addr: SocketAddr = "0.0.0.0:7690".parse().expect("address");
        assert!(announcement(addr, env(&[])).expect("no error").is_none());
        assert!(announcement(addr, env(&[("SCORE_GATEWAY_MDNS", " ")])).expect("no error").is_none());
    }

    #[test]
    fn mdns_announces_an_interface_address() {
        let addr: SocketAddr = "0.0.0.0:7690".parse().expect("address");
        let vars = [
            ("SCORE_GATEWAY_MDNS", "SCORE0DEMO0000001"),
            ("SCORE_GATEWAY_MDNS_HOST", "score-pi"),
            ("SCORE_GATEWAY_MDNS_INTERFACE", "eth0, br0"),
        ];
        assert!(announcement(addr, env(&vars)).expect("no error").is_some());
    }

    #[test]
    fn mdns_rejects_the_loopback_address() {
        let addr: SocketAddr = "127.0.0.1:7690".parse().expect("address");
        let error = announcement(addr, env(&[("SCORE_GATEWAY_MDNS", "SCORE0DEMO0000001")])).expect_err("loopback");
        assert!(error.contains("loopback"), "{error}");
    }

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
