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

//! The upstream of the S-CORE gateway, such as the classic diagnostic adapter (CDA), for the
//! `opensovd-extra` federation.
//!
//! The proxy is the one of opensovd-core, `opensovd_extra::UpstreamProxy`: it sends what the
//! gateway does not serve below the upstream's base URL, streams the answer back without
//! hop-by-hop headers, and rewrites the upstream's references to its own API in `Location` and in
//! JSON bodies to the gateway, so clients never bypass it. This module only reads the URL of the
//! upstream, in the forms that `--upstream-url` of the reference gateway takes:
//!
//! - `http://<host>:<port>[<base path>]` over TCP, such as `http://127.0.0.1:20002/vehicle/v15`;
//! - `unix:<socket>[:<base path>]` over a Unix domain socket, such as
//!   `unix:///run/cda.sock:/vehicle/v15`. The socket is an absolute path or, on Linux, `@name` for
//!   an abstract socket. The base path starts at the first `:/`.

use http::Uri;
use opensovd_extra::{ForwardRoute, Upstream, UpstreamProxy};
use std::time::Duration;

/// How long connecting to the upstream may take. It does not limit the response.
const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);

/// Where the upstream listens.
#[derive(Debug, PartialEq, Eq)]
enum Endpoint {
    Tcp(Uri),
    UnixSocket { socket: String, base: String },
}

fn endpoint(url: &str) -> Result<Endpoint, String> {
    if let Some(rest) = url.strip_prefix("unix:") {
        let rest = rest.strip_prefix("//").unwrap_or(rest);
        let (socket, base) = rest
            .split_once(":/")
            .map_or((rest, String::new()), |(socket, base)| (socket, format!("/{base}")));
        let abstract_name = cfg!(target_os = "linux") && socket.starts_with('@');
        if socket.len() < 2 || !(socket.starts_with('/') || abstract_name) {
            return Err(format!(
                "upstream {url} must name a socket by its absolute path, or on Linux as @name"
            ));
        }
        if base.contains(['?', '#']) {
            return Err(format!("upstream {url} must not have a query"));
        }
        return Ok(Endpoint::UnixSocket {
            socket: socket.to_owned(),
            base,
        });
    }
    let uri: Uri = url
        .parse()
        .map_err(|error| format!("invalid upstream URL {url:?}: {error}"))?;
    if uri.scheme_str() != Some("http") {
        return Err(format!("upstream {url} must use http or unix"));
    }
    if uri.authority().is_none() {
        return Err(format!("no host in {url}"));
    }
    if uri.query().is_some() {
        return Err(format!("upstream {url} must not have a query"));
    }
    Ok(Endpoint::Tcp(uri))
}

/// The single upstream of the S-CORE gateway. It owns the entities the gateway does not host, the
/// SOVD functions and collections, and vehicle locks.
pub fn upstream(url: &str) -> Result<Upstream, String> {
    let proxy = match endpoint(url)? {
        Endpoint::Tcp(uri) => UpstreamProxy::new(&uri, CONNECT_TIMEOUT).map_err(|error| format!("{error:#}"))?,
        Endpoint::UnixSocket { socket, base } => UpstreamProxy::unix_socket(&socket, &base, CONNECT_TIMEOUT),
    };
    Ok(Upstream::new(proxy.into_service()).routes([
        ForwardRoute::Entities,
        ForwardRoute::Locks,
        ForwardRoute::Functions,
        ForwardRoute::Collections,
        ForwardRoute::CustomCollections,
    ]))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn socket(socket: &str, base: &str) -> Endpoint {
        Endpoint::UnixSocket {
            socket: socket.to_owned(),
            base: base.to_owned(),
        }
    }

    #[test]
    fn tcp_and_socket_upstreams_are_read() {
        assert_eq!(
            endpoint("http://127.0.0.1:20002/vehicle/v15"),
            Ok(Endpoint::Tcp("http://127.0.0.1:20002/vehicle/v15".parse().expect("URI")))
        );
        assert_eq!(endpoint("unix:///run/cda.sock:/vehicle/v15"), Ok(socket("/run/cda.sock", "/vehicle/v15")));
        assert_eq!(endpoint("unix:/run/cda.sock"), Ok(socket("/run/cda.sock", "")));
        assert_eq!(endpoint("unix:/run/a:b.sock:/v15"), Ok(socket("/run/a:b.sock", "/v15")));
        if cfg!(target_os = "linux") {
            assert_eq!(endpoint("unix:@cda:/vehicle/v15"), Ok(socket("@cda", "/vehicle/v15")));
        }
    }

    #[test]
    fn only_http_and_socket_upstreams_are_accepted() {
        for url in [
            "https://cda:20002/vehicle/v15",
            "ftp://cda/vehicle/v15",
            "/vehicle/v15",
            "http://cda:20002/vehicle/v15?x=1",
            "unix:run/cda.sock:/vehicle/v15",
            "unix:/",
            "unix:/run/cda.sock:/vehicle/v15?x=1",
        ] {
            assert!(endpoint(url).is_err(), "{url} accepted");
        }
    }
}
