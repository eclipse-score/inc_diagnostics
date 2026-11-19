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

`//score/opensovd-core:opensovd-gateway` is the SOVD server of S-CORE (ISO 17978-3). It runs the
gateway of opensovd-core (`@opensovd_core//:opensovd-gateway-lib`) with the entities of S-CORE, so
it takes the options and `SOVD_*` environment variables of the reference gateway of opensovd-core
(`opensovd-gateway --help` there). Those that S-CORE uses:

| Variable | Option | Meaning | Default |
|---|---|---|---|
| `SOVD_URL` | `--url` | Address, port and base path to serve on | `http://localhost:7690/sovd` |
| `SOVD_UPSTREAM_URL` | `--upstream-url` | The CDA to forward what the gateway does not host to: `http://<host>:<port>[<base path>]`, e.g. `http://127.0.0.1:20002/vehicle/v15`, or a Unix domain socket `unix:<socket>[:<base path>]`, e.g. `unix:///run/cda.sock:/vehicle/v15` | no forwarding |
| `SOVD_MDNS` | `--mdns` | Announce the gateway via mDNS/DNS-SD with this vehicle identification, e.g. the VIN | no announcement |
| `SOVD_MDNS_HOST` | `--mdns-host` | Host label of the announcement, published as `<host>.local` | derived from the identification |
| `SOVD_MDNS_INTERFACE` | `--mdns-interface` | Comma separated interfaces to announce on, for an unspecified address | all non-loopback interfaces |

With an upstream, the federation of opensovd-core lists the entities of the CDA next to the
gateway's own and forwards to it, over TCP or a Unix domain socket. Authentication, TLS and CORS
are those of opensovd-core as well.

## mDNS/DNS-SD

With `SOVD_MDNS` set, the gateway announces itself as a `_sovd._tcp` service (ISO 17978-3 §5.11),
so that a client on the same network finds it (ISO 17978-2 UC01 "Discover SOVD server(s)"). The
service has the TXT records `identification` (the value of `SOVD_MDNS`) and `accessurl` (the
`SOVD_URL` with the address of the gateway, the parent of `version-info`).

- The gateway has to listen on an address that clients can reach: `0.0.0.0` announces all
  non-loopback addresses, an interface address only that interface. A loopback address is refused
  at startup.
- The announcement starts once the server is set up and follows interface and address changes.
  On SIGTERM (the launch manager stops the gateway with it) or Ctrl-C the gateway sends a goodbye,
  so clients drop it at once.

Find it from another machine on the network:

```sh
dns-sd -B _sovd._tcp                 # macOS
avahi-browse -rt _sovd._tcp          # Linux
```
