# ReadDID Provider Prototype

The binding exposes existing C++ `ReadDataByIdentifier` handlers as read-only Core data resources.
The example registers `application.version` on `score-diagnostics`, hosted on `score-demo`.
It uses the UTF-8 encoding, so its HTTP data envelope is `{"value":"1.0.0"}`.

## Ownership and ABI

- C++ callers register a non-null shared handler with `RegisterReadDataByIdentifier`. The returned reader handle owns it.
- `score_diag_read_start` takes a valid reader, callback, and callback context. The caller must keep the reader alive until start returns.
- Start transfers the context to exactly one callback, including allocation/thread-start failures and saturation. A null request means synchronous failure completion, not an unconsumed context.
- Reader and request handles are distinct opaque types on both sides (`#[repr(C)]` zero-sized structs in Rust), so they cannot be swapped.
- `ScoreDiagReadStatus` is a `uint8_t` with fixed values; Rust mirrors it as a `#[repr(u8)]` enum, read_did.cpp `static_assert`s size and values, and unknown values map to FAILED.
- The callback receives OK, NRC, CANCELLED, FAILED, or BUSY. Data is borrowed only for callback duration; Rust copies it before returning. C++ errors that are not NRCs become FAILED.
- A request retains shared handler state independently of the registration handle. Releasing registration prevents further use of that handle but does not invalidate started reads.
- Cancel and release are separate operations. The caller must not use a released handle; cancel/release on one request must not race each other. Rust's uniquely owned request guard cancels before release on every exit path.
- A bounded worker waits on the interruptible C++ future, never on the Tokio executor. At most eight requests may be outstanding per handler; additional starts complete BUSY without a new worker.
- The handler's Read dispatch is serialized. Read must return its future promptly; the bridge cannot interrupt a handler blocked inside Read itself. Future waiting is interruptible even if the eventual producer ignores cancellation.
- C++ catches application exceptions before callback completion. The callback must not throw. Rust's callback consumes its boxed channel sender once and does not unwrap fallible operations; Rust panics must not unwind across C.

The Rust Handler Send/Sync implementations rely on immutable opaque handles, C++ shared ownership, atomic request accounting,
and the dispatch mutex. A Request has a single owner and is movable between threads; it is deliberately not Sync.
The boxed channel sender survives dropped receivers until the worker completes cancellation, so late completion cannot access freed Rust state.

## Scope and Limitations

This is an in-process example, not remote application registration. The asynchronous demo reader serves
`application.version` and the scheduling tests.
The Bazel integration is root-only/dev-only and is not exported to downstream modules.
The worker model is bounded per handler, not a production shared executor. Producers must cooperate with stop tokens to release their own work.

Only unrestricted reads are supported. The shim explicitly supplies Default/Physical metadata, locked security and unknown addresses.
It does not populate session/security from HTTP headers or claim to propagate authenticated UDS context.
Reads fail with a typed `ReadError` (NRC, busy, cancelled, timeout, failed), which is converted to Core's `DataError`.
Core's current `DataError` has no variants for access denied, conflict, busy or timeout, so only NRC 0x31
(requestOutOfRange) becomes `NotFound` (HTTP 404). All other failures become `Internal` (HTTP 500) with the message
`UDS NRC 0xXX` or the failure kind. Mapping e.g. 0x33 to 403, 0x22 to 409, 0x21/busy to 503 and timeout to 504
needs a typed provider error in Core; see issue #57.
The gateway installs a subscriber from OpenSOVD's crate index so upstream HTTP 500 details are retained in logs, with filtering controlled by `RUST_LOG`.

Application registration, application-owned data sources, value codecs, WriteDID, routines, fault-lib, process registration, and authenticated context
mapping are tracked in [Use cases: v1.0+](https://github.com/eclipse-score/inc_diagnostics/issues/67).

## Checks

```sh
bazel test --config=score_diag_x86_64_linux //score/opensovd-core:read_did_bridge_test //score/opensovd-core:application_registration_test
```

The C++ tests cover request lifetime, metadata, byte copying and cancellation with a real unresolved promise.
The Rust tests cover NRCs, encodings, timeout, dropped reads, saturation and executor responsiveness. Pending reads use a
test-only C++ reader (`read_did_test_support`) that completes on request, so the tests wait for observed state, not time.
The HTTP tests exercise the registered application through the actual example topology, alone and with an
unreachable upstream.

## Local Walkthrough

Start the gateway in one terminal (loopback only, no production authorization). With `SCORE_UPSTREAM_URL` it
forwards everything it does not host, for example to a CDA, and lists the CDA's entities next to its own:

```sh
SCORE_UPSTREAM_URL=http://127.0.0.1:20002/vehicle/v15 \
  bazel run --config=score_diag_x86_64_linux //score/opensovd-core:opensovd-gateway
```

Query it from another terminal:

```sh
curl --fail http://127.0.0.1:7690/sovd/v1/apps
curl --fail http://127.0.0.1:7690/sovd/v1/apps/score-diagnostics/data/application.version
```

The version read contains `data.value: "1.0.0"`. `//score/opensovd-cda:integration_test` starts a CDA and the
gateway, discovers the CDA's apps through the gateway and reads their data through it.
