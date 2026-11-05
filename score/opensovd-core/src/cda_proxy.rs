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

//! Reverse proxy that forwards SOVD requests to a downstream CDA instance.
//!
//! Uses the opensovd-client to forward GET requests, avoiding heavy TLS deps.

use opensovd_client::Client;

/// Configuration for a downstream CDA instance.
#[derive(Clone)]
pub struct CdaBackend {
    /// The opensovd client connected to the CDA SOVD endpoint.
    client: Client,
}

impl CdaBackend {
    /// Create a new CDA backend proxy pointing at the given base URL.
    pub fn new(base_url: &str) -> Result<Self, Box<dyn std::error::Error + Send + Sync>> {
        let client = Client::connect(base_url)?;
        Ok(Self { client })
    }

    /// Forward a GET request to the CDA, returning the raw JSON response.
    pub async fn forward(
        &self,
        path: &str,
        query: &[(&str, &str)],
    ) -> Result<serde_json::Value, Box<dyn std::error::Error + Send + Sync>> {
        let result: serde_json::Value = self.client.get(path, query).await?;
        Ok(result)
    }
}

/// Build an Axum router that proxies all requests to the CDA.
pub fn cda_proxy_router(backend: CdaBackend) -> axum::Router {
    use axum::extract::{OriginalUri, State};
    use axum::response::IntoResponse;

    async fn proxy_handler(
        State(backend): State<CdaBackend>,
        OriginalUri(uri): OriginalUri,
    ) -> impl IntoResponse {
        let path = uri.path();

        // Strip the gateway mount prefix (/sovd/v1/cda) to get the CDA-relative path.
        let cda_path = path
            .find("/cda/")
            .map(|pos| &path[pos + 4..])
            .or_else(|| path.find("/cda").map(|pos| &path[pos + 4..]))
            .unwrap_or(path);

        // Forward as a bare path; query params could be added later
        match backend.forward(cda_path, &[]).await {
            Ok(value) => axum::Json(value).into_response(),
            Err(err) => {
                let body = serde_json::json!({
                    "error": format!("CDA proxy error: {err}"),
                });
                (
                    axum::http::StatusCode::BAD_GATEWAY,
                    axum::Json(body),
                )
                    .into_response()
            }
        }
    }

    axum::Router::new()
        .fallback(proxy_handler)
        .with_state(backend)
}
