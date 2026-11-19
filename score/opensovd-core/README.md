<!--
*******************************************************************************
Copyright (c) 2026 Contributors to the Eclipse Foundation

See the NOTICE file(s) distributed with this work for additional
information regarding copyright ownership.

This program and the accompanying materials are made available under the
terms of the Apache License Version 2.0 which is available at
https://www.apache.org/licenses/LICENSE-2.0

SPDX-License-Identifier: Apache-2.0
*******************************************************************************
-->

# S-CORE OpenSOVD gateway

`//score/opensovd-core:opensovd-gateway` is the SOVD server of S-CORE (ISO 17978-3), built on the
crates of `opensovd_core`. It serves its own entities and, with `SCORE_UPSTREAM_URL`, forwards to
the classic diagnostic adapter (CDA) with the federation and the upstream proxy of opensovd-core
(`@opensovd_core//:opensovd-http-proxy`), over TCP or a Unix domain socket. It is configured by
environment variables:

| Variable | Meaning | Default |
|---|---|---|
| `SCORE_GATEWAY_ADDRESS` | Address and port to listen on | `127.0.0.1:7690` |
| `SCORE_UPSTREAM_URL` | The CDA to forward to: `http://<host>:<port>[<base path>]`, e.g. `http://127.0.0.1:20002/vehicle/v15`, or a Unix domain socket `unix:<socket>[:<base path>]`, e.g. `unix:///run/cda.sock:/vehicle/v15` | no forwarding |
| `SCORE_GATEWAY_MDNS` | Announce the gateway via mDNS/DNS-SD with this vehicle identification, e.g. the VIN | no announcement |
| `SCORE_GATEWAY_MDNS_HOST` | Host label of the announcement, published as `<host>.local` | derived from the identification |
| `SCORE_GATEWAY_MDNS_INTERFACE` | Comma separated interfaces to announce on, for an unspecified address | all non-loopback interfaces |

## mDNS/DNS-SD

With `SCORE_GATEWAY_MDNS` set, the gateway announces itself as a `_sovd._tcp` service (ISO 17978-3
§5.11), so that a client on the same network finds it (ISO 17978-2 UC01 "Discover SOVD server(s)").
The service has the TXT records `identification` (the value of `SCORE_GATEWAY_MDNS`) and
`accessurl` (`http://<address>:<port>/sovd`, the parent of `version-info`). It uses the announcer of
`opensovd-extra` (`@opensovd_core//:opensovd-mdns`).

- The gateway has to listen on an address that clients can reach: `0.0.0.0` announces all
  non-loopback IPv4 addresses, an interface address only that interface. A loopback address is
  refused at startup.
- The announcement starts once the server is set up and follows interface and address changes.
  On SIGTERM (the launch manager stops the gateway with it) or Ctrl-C the gateway sends a goodbye,
  so clients drop it at once.

Find it from another machine on the network:

```sh
dns-sd -B _sovd._tcp                 # macOS
avahi-browse -rt _sovd._tcp          # Linux
```
