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

//! Upstream SOVD server such as the classic diagnostic adapter (CDA) for the `opensovd-extra`
//! federation. The proxy itself is `opensovd_extra::UpstreamProxy`; this module parses the
//! upstream URL and names the routes the gateway forwards.

use http::Uri;
use opensovd_extra::{ForwardRoute, Upstream, UpstreamProxy};
use std::time::Duration;

const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);

/// The single upstream of the S-CORE gateway. It owns the entities the gateway does not host, the
/// SOVD functions and collections, and vehicle locks.
///
/// `url` is `http://host:port/base`, or `unix:<socket>[:<base>]` for a Unix domain socket, such as
/// `unix:/run/cda.sock:/vehicle/v15`.
// # req-Id: comp_req__sovd_gateway__forwarding, comp_req__sovd_gateway__upstream_config
pub fn upstream(url: &str) -> Result<Upstream, String> {
    let proxy = match url.strip_prefix("unix:") {
        Some(rest) => {
            let (socket, base) = match rest.find(":/") {
                Some(index) => rest.split_at(index),
                None => (rest, ""),
            };
            if socket.is_empty() {
                return Err(format!("upstream {url} names no socket"));
            }
            UpstreamProxy::unix_socket(socket, base.trim_start_matches(':'), CONNECT_TIMEOUT)
        },
        None => {
            let uri: Uri = url
                .parse()
                .map_err(|error| format!("invalid upstream URL {url:?}: {error}"))?;
            if uri.scheme_str() != Some("http") {
                return Err(format!("upstream {url} must use http or unix"));
            }
            UpstreamProxy::new(&uri, CONNECT_TIMEOUT).map_err(|error| error.to_string())?
        },
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

    // # verifies: comp_req__sovd_gateway__upstream_config
    #[test]
    fn only_http_and_unix_upstreams_are_accepted() {
        assert!(upstream("https://cda:20002/vehicle/v15").is_err());
        assert!(upstream("/vehicle/v15").is_err());
        assert!(upstream("unix:").is_err());
        assert!(upstream("http://cda:20002/vehicle/v15").is_ok());
        assert!(upstream("unix:/run/cda.sock:/vehicle/v15").is_ok());
        assert!(upstream("unix:/run/cda.sock").is_ok());
    }
}
