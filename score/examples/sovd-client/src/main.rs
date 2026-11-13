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
use std::time::Duration;
use tracing_subscriber::EnvFilter;

const DEFAULT_URL: &str = "http://127.0.0.1:7690/sovd/v1";

fn connect(url: &str) -> Result<Client, opensovd_client::BuilderError> {
    Client::builder().base_uri(url)?.timeout(Duration::from_secs(5)).build()
}

fn init_tracing() {
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));
    tracing_subscriber::fmt().with_env_filter(filter).init();
}

fn is_unsupported_collection(error: &opensovd_client::Error) -> bool {
    matches!(error, opensovd_client::Error::ApiError { status, .. } if status.as_u16() == 404)
}

async fn run(client: &Client) -> Result<(), opensovd_client::Error> {
    let components = client.list_components().send().await?;
    for component in &components.items {
        tracing::info!("component: {} ({})", component.id, component.name);
    }

    if let Some(component) = components.items.first() {
        match client.component(&component.id).list_data().send().await {
            Ok(data) => {
                for item in &data.items {
                    tracing::info!("  data: {} ({})", item.id, item.name);
                }
            },
            Err(error) if is_unsupported_collection(&error) => {},
            Err(error) => return Err(error),
        }
    }

    let apps = client.list_apps().send().await?;
    for app in &apps.items {
        tracing::info!("app: {} ({})", app.id, app.name);
    }

    match client.list_areas().send().await {
        Ok(areas) => {
            for area in &areas.items {
                tracing::info!("area: {} ({})", area.id, area.name);
            }
        },
        Err(error) if is_unsupported_collection(&error) => {},
        Err(error) => return Err(error),
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use opensovd_core::Component;
    use opensovd_server::{Server, Topology};
    use tokio::net::TcpListener;

    #[tokio::test(flavor = "current_thread")]
    async fn discovery_continues_without_component_data() {
        let listener = TcpListener::bind("127.0.0.1:0").await.expect("listener");
        let address = listener.local_addr().expect("address");
        let topology = Topology::new();
        topology
            .write()
            .await
            .add_component(Component::new("no-data", "No Data"));
        let server = Server::builder()
            .base_uri(format!("http://{address}/sovd"))
            .expect("base URI")
            .listener(listener)
            .topology(topology)
            .build()
            .expect("server");
        let server_task = tokio::spawn(async move { server.serve().await });
        let client = connect(&format!("http://{address}/sovd/v1")).expect("client");
        let result = run(&client).await;
        server_task.abort();
        let _ = server_task.await;
        assert!(result.is_ok(), "unsupported data must not abort discovery: {result:?}");
    }

    #[tokio::test(flavor = "current_thread")]
    async fn configured_client_times_out_against_unresponsive_peer() {
        let listener = TcpListener::bind("127.0.0.1:0").await.expect("listener");
        let address = listener.local_addr().expect("address");
        let peer = tokio::spawn(async move {
            let (_connection, _) = listener.accept().await.expect("accept connection");
            std::future::pending::<()>().await;
        });
        let client = connect(&format!("http://{address}/sovd/v1")).expect("client");
        let result = tokio::time::timeout(Duration::from_secs(6), run(&client)).await;
        peer.abort();
        let _ = peer.await;
        assert!(matches!(result, Ok(Err(opensovd_client::Error::Timeout(_)))));
    }

    #[test]
    fn only_not_found_marks_a_collection_unsupported() {
        let not_found = opensovd_client::Error::ApiError {
            status: "404".parse().expect("status"),
            details: None,
        };
        let server_error = opensovd_client::Error::ApiError {
            status: "500".parse().expect("status"),
            details: None,
        };
        let timeout = opensovd_client::Error::Timeout(std::time::Duration::from_secs(1));
        assert!(is_unsupported_collection(&not_found));
        assert!(!is_unsupported_collection(&server_error));
        assert!(!is_unsupported_collection(&timeout));
    }
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    init_tracing();
    let url = std::env::args()
        .nth(1)
        .or_else(|| std::env::var("SOVD_BASE_URL").ok())
        .unwrap_or_else(|| DEFAULT_URL.to_owned());
    let client = connect(&url)?;
    run(&client).await?;
    Ok(())
}
