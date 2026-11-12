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

//! Proxy to an upstream SOVD server such as the classic diagnostic adapter (CDA), for the
//! `opensovd-extra` federation. Adapted from `opensovd-cli/gateway/src/upstream.rs` in opensovd-core,
//! which is part of the gateway binary and cannot be reused as a library.
//!
//! The federation hands the proxy what the gateway does not serve itself, with the URI relative to
//! the API version. The proxy sends it below the upstream's base URL and streams the answer back
//! without hop-by-hop headers. References to the upstream's own API in `Location` and in JSON
//! bodies are rewritten to the gateway, so clients never bypass it.

use http::header::{self, HeaderMap, HeaderName};
use http::response::Parts;
use http::{HeaderValue, Request, Response, StatusCode, Uri};
use http_body_util::{BodyExt, Full, Limited};
use hyper_util::client::legacy::connect::HttpConnector;
use hyper_util::client::legacy::Client;
use hyper_util::rt::TokioExecutor;
use opensovd_extra::{ForwardRoute, ServerUri, Upstream};
use opensovd_models::error::GenericError;
use opensovd_server::Body;
use std::convert::Infallible;
use std::time::Duration;

const JSON_LIMIT: usize = 64 * 1024 * 1024;
const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);

const HOP_BY_HOP: [HeaderName; 7] = [
    header::CONNECTION,
    HeaderName::from_static("keep-alive"),
    header::PROXY_AUTHENTICATE,
    header::PROXY_AUTHORIZATION,
    header::TE,
    header::TRAILER,
    header::UPGRADE,
];

/// The single upstream of the S-CORE gateway. It owns the entities the gateway does not host, the
/// SOVD functions and collections, and vehicle locks.
pub fn upstream(url: &str) -> Result<Upstream, String> {
    let url: Uri = url
        .parse()
        .map_err(|error| format!("invalid upstream URL {url:?}: {error}"))?;
    let proxy = UpstreamProxy::new(&url)?;
    Ok(Upstream::new(proxy.into_service()).routes([
        ForwardRoute::Entities,
        ForwardRoute::Locks,
        ForwardRoute::Functions,
        ForwardRoute::Collections,
        ForwardRoute::CustomCollections,
    ]))
}

#[derive(Clone)]
struct UpstreamProxy {
    /// Scheme and authority of the upstream.
    origin: Uri,
    /// Path of the upstream's API without a trailing `/`, such as `/vehicle/v15`.
    base: String,
    client: Client<HttpConnector, Body>,
}

impl UpstreamProxy {
    fn new(url: &Uri) -> Result<Self, String> {
        if url.scheme_str() != Some("http") {
            return Err(format!("upstream {url} must use http"));
        }
        let authority = url.authority().ok_or_else(|| format!("no host in {url}"))?;
        let origin = Uri::builder()
            .scheme("http")
            .authority(authority.clone())
            .path_and_query("/")
            .build()
            .map_err(|error| error.to_string())?;
        let mut connector = HttpConnector::new();
        connector.set_connect_timeout(Some(CONNECT_TIMEOUT));
        connector.set_nodelay(true);
        Ok(Self {
            origin,
            base: url.path().trim_end_matches('/').to_owned(),
            client: Client::builder(TokioExecutor::new()).build(connector),
        })
    }

    fn into_service(
        self,
    ) -> impl tower::Service<
        Request<Body>,
        Response = Response<Body>,
        Error = Infallible,
        Future = impl std::future::Future<Output = Result<Response<Body>, Infallible>> + Send,
    > + Clone
           + Send
           + Sync
           + 'static {
        tower::service_fn(move |request: Request<Body>| {
            let proxy = self.clone();
            async move { Ok(proxy.call(request).await) }
        })
    }

    /// `uri` below the upstream's base URL.
    fn target(&self, uri: &Uri) -> Option<Uri> {
        let path = format!("{}{}", self.base, uri.path().trim_end_matches('/'));
        let path = if path.is_empty() { "/" } else { &path };
        let query = uri.query().map_or_else(String::new, |query| format!("?{query}"));
        let mut parts = self.origin.clone().into_parts();
        parts.path_and_query = Some(format!("{path}{query}").parse().ok()?);
        Uri::from_parts(parts).ok()
    }

    async fn call(&self, request: Request<Body>) -> Response<Body> {
        let (mut parts, body) = request.into_parts();
        let Some(target) = self.target(&parts.uri) else {
            return bad_gateway("The request has no valid upstream URI");
        };
        let server = parts.extensions.get::<ServerUri>().cloned();
        tracing::debug!(target: "forwarding", "{} {target}", parts.method);
        parts.uri = target;
        strip_hop_by_hop_headers(&mut parts.headers);
        parts.headers.remove(header::HOST);

        match self.client.request(Request::from_parts(parts, body)).await {
            Ok(response) => {
                let (mut parts, body) = response.into_parts();
                strip_hop_by_hop_headers(&mut parts.headers);
                let body = Body::new(body);
                match server {
                    Some(ServerUri(server)) => self.point_at(&server, parts, body).await,
                    None => Response::from_parts(parts, body),
                }
            },
            Err(error) => {
                tracing::warn!(target: "forwarding", error = %error, "Upstream unreachable");
                bad_gateway("The upstream is unreachable")
            },
        }
    }

    /// Makes references to the upstream in `Location` and JSON bodies refer to `server`.
    async fn point_at(&self, server: &str, mut parts: Parts, body: Body) -> Response<Body> {
        let location = parts
            .headers
            .get(header::LOCATION)
            .and_then(|location| location.to_str().ok())
            .and_then(|location| self.at_server(location, server))
            .and_then(|location| HeaderValue::try_from(location).ok());
        if let Some(location) = location {
            parts.headers.insert(header::LOCATION, location);
        }
        if !is_json(&parts.headers) {
            return Response::from_parts(parts, body);
        }
        let body = match Limited::new(body, JSON_LIMIT).collect().await {
            Ok(body) => body.to_bytes(),
            Err(error) => {
                tracing::warn!(target: "forwarding", error = %error, "Invalid upstream response");
                return bad_gateway("The upstream sent an invalid response");
            },
        };
        let rewritten = std::str::from_utf8(&body)
            .ok()
            .and_then(|text| self.at_server(text, server));
        let body = match rewritten {
            Some(rewritten) => {
                parts.headers.remove(header::CONTENT_LENGTH);
                bytes::Bytes::from(rewritten)
            },
            None => body,
        };
        Response::from_parts(parts, Body::new(Full::new(body)))
    }

    /// `text` with absolute URLs of the upstream API replaced by `server`, and bare references to
    /// its base path by the path of `server`. `None` if nothing refers to the upstream.
    fn at_server(&self, text: &str, server: &str) -> Option<String> {
        let authority = self.origin.authority().map_or("", |authority| authority.as_str());
        let reference = if self.base.is_empty() {
            authority
        } else {
            self.base.as_str()
        };
        let server_path = server
            .split_once("://")
            .and_then(|(_, rest)| rest.find('/').and_then(|path| rest.get(path..)))
            .unwrap_or_default();

        let mut rewritten = String::new();
        let mut copied = 0;
        for start in matches_ignoring_case(text, reference) {
            let end = start.saturating_add(reference.len());
            let (Some(before), Some(after)) = (text.get(..start), text.get(end..)) else {
                continue;
            };
            if start < copied || after.starts_with(is_path_char) {
                continue;
            }
            let (from, target) = match url_start(before) {
                Some(url) => (url, server),
                None if self.base.is_empty() => continue,
                None if before.ends_with(|c| is_path_char(c) || c == '/') => continue,
                None => (start, server_path),
            };
            rewritten.push_str(text.get(copied..from)?);
            rewritten.push_str(target);
            copied = end;
        }
        if copied == 0 {
            return None;
        }
        rewritten.push_str(text.get(copied..)?);
        Some(rewritten)
    }
}

fn matches_ignoring_case<'a>(text: &'a str, needle: &'a str) -> impl Iterator<Item = usize> + 'a {
    text.as_bytes()
        .windows(needle.len().max(1))
        .enumerate()
        .filter(|(_, window)| window.eq_ignore_ascii_case(needle.as_bytes()))
        .map(|(offset, _)| offset)
}

fn is_path_char(c: char) -> bool {
    c.is_ascii_alphanumeric() || c == '-' || c == '_'
}

/// Start of the `http://` or `https://` URL that `before` ends in, if it ends in a scheme and host.
fn url_start(before: &str) -> Option<usize> {
    let (scheme, host) = before.rsplit_once("://")?;
    if !host
        .chars()
        .all(|c| is_path_char(c) || matches!(c, '.' | ':' | '[' | ']'))
    {
        return None;
    }
    ["https", "http"]
        .iter()
        .find_map(|name| scheme.strip_suffix(name))
        .map(str::len)
}

fn is_json(headers: &HeaderMap) -> bool {
    headers
        .get(header::CONTENT_TYPE)
        .and_then(|content_type| content_type.to_str().ok())
        .and_then(|content_type| content_type.split(';').next())
        .is_some_and(|mime| mime.trim() == "application/json" || mime.trim().ends_with("+json"))
}

fn strip_hop_by_hop_headers(headers: &mut HeaderMap) {
    let named: Vec<HeaderName> = headers
        .get_all(header::CONNECTION)
        .iter()
        .filter_map(|value| value.to_str().ok())
        .flat_map(|value| value.split(','))
        .filter_map(|name| HeaderName::try_from(name.trim()).ok())
        .collect();
    for name in HOP_BY_HOP.iter().chain(&named) {
        headers.remove(name);
    }
}

fn bad_gateway(message: &str) -> Response<Body> {
    let error = GenericError::with_vendor_code("upstream-unreachable", message);
    let body = serde_json::to_vec(&error).unwrap_or_default();
    let mut response = Response::new(Body::new(Full::new(bytes::Bytes::from(body))));
    *response.status_mut() = StatusCode::BAD_GATEWAY;
    response
        .headers_mut()
        .insert(header::CONTENT_TYPE, HeaderValue::from_static("application/json"));
    response
}

#[cfg(test)]
mod tests {
    use super::*;

    fn proxy(url: &str) -> UpstreamProxy {
        UpstreamProxy::new(&url.parse().expect("URL")).expect("proxy")
    }

    // # req-Id: comp_req__sovd_gateway__reference_rewrite
    #[test]
    fn references_to_the_cda_refer_to_the_gateway() {
        let proxy = proxy("http://cda:20002/vehicle/v15");
        let server = "http://gw:7690/sovd/v1";
        assert_eq!(
            proxy.at_server(r#"{"href":"http://cda:20002/vehicle/v15/apps/sovd2uds"}"#, server),
            Some(r#"{"href":"http://gw:7690/sovd/v1/apps/sovd2uds"}"#.to_owned())
        );
        assert_eq!(
            proxy.at_server(r#"{"message":"/vehicle/v15/apps/x not found"}"#, server),
            Some(r#"{"message":"/sovd/v1/apps/x not found"}"#.to_owned())
        );
        assert_eq!(proxy.at_server(r#"{"href":"/vehicle/v150"}"#, server), None);
        assert_eq!(proxy.at_server(r#"{"id":"sovd2uds"}"#, server), None);
    }

    // # req-Id: comp_req__sovd_gateway__forwarding
    #[test]
    fn requests_go_below_the_upstream_base() {
        let proxy = proxy("http://cda:20002/vehicle/v15/");
        let target = proxy.target(&"/apps/sovd2uds/data?x=1".parse().expect("URI"));
        assert_eq!(
            target.map(|uri| uri.to_string()),
            Some("http://cda:20002/vehicle/v15/apps/sovd2uds/data?x=1".to_owned())
        );
    }

    // # req-Id: comp_req__sovd_gateway__upstream_config
    #[test]
    fn only_plain_http_upstreams_are_accepted() {
        assert!(upstream("https://cda:20002/vehicle/v15").is_err());
        assert!(upstream("/vehicle/v15").is_err());
        assert!(upstream("http://cda:20002/vehicle/v15").is_ok());
    }

    // # req-Id: comp_req__sovd_gateway__forwarding
    #[test]
    fn hop_by_hop_headers_are_dropped() {
        let mut headers = HeaderMap::new();
        headers.insert(header::CONNECTION, HeaderValue::from_static("x-private"));
        headers.insert("x-private", HeaderValue::from_static("1"));
        headers.insert(header::TE, HeaderValue::from_static("trailers"));
        headers.insert(header::ACCEPT, HeaderValue::from_static("application/json"));
        strip_hop_by_hop_headers(&mut headers);
        assert_eq!(headers.len(), 1);
        assert!(headers.contains_key(header::ACCEPT));
    }
}
