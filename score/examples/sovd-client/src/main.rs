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
use tracing_subscriber::EnvFilter;

const DEFAULT_URL: &str = "http://127.0.0.1:7690/sovd/v1";

fn init_tracing() {
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));
    tracing_subscriber::fmt().with_env_filter(filter).init();
}

async fn run(client: &Client) -> Result<(), opensovd_client::Error> {
    let components = client.list_components().send().await?;
    for component in &components.data.items {
        tracing::info!("component: {} ({})", component.id, component.name);
    }

    if let Some(component) = components.data.items.first() {
        let data = client.component(&component.id).list_data().send().await?;
        for item in &data.data.items {
            tracing::info!("  data: {} ({})", item.id, item.name);
        }
    }

    let apps = client.list_apps().send().await?;
    for app in &apps.data.items {
        tracing::info!("app: {} ({})", app.id, app.name);
    }

    let areas = client.list_areas().send().await?;
    for area in &areas.data.items {
        tracing::info!("area: {} ({})", area.id, area.name);
    }

    Ok(())
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    init_tracing();
    let url = std::env::args()
        .nth(1)
        .or_else(|| std::env::var("SOVD_BASE_URL").ok())
        .unwrap_or_else(|| DEFAULT_URL.to_owned());
    let client = Client::connect(&url)?;
    run(&client).await?;
    Ok(())
}
