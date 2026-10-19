# ReadDID Provider Prototype

The binding exposes existing C++ `ReadDataByIdentifier` handlers as read-only Core data resources.
The example registers `application.version` on `score-diagnostics`, hosted on `score-demo`.
Its HTTP data envelope is `{"value":[49,46,48,46,48]}`: the bytes of `1.0.0`, not serialized JSON bytes.

## Ownership and ABI

- C++ callers register a non-null shared handler with `RegisterReadDataByIdentifier`. The returned reader handle owns it.
- `score_diag_read_start` takes a valid reader, callback, and callback context. The caller must keep the reader alive until start returns.
- Start transfers the context to exactly one callback, including allocation/thread-start failures and saturation. A null request means synchronous failure completion, not an unconsumed context.
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

This is an in-process example, not remote application registration. Applications can supply C++ handler/resource catalogs,
which Rust consumes through the ownership-transfer registration API described below. The original asynchronous demo reader remains
for version and scheduling tests; the sensor catalog uses application-owned source state instead of that demo constructor.
The worker model is bounded per handler, not a production shared executor. Producers must cooperate with stop tokens to release their own work.

Only unrestricted reads are supported. The shim explicitly supplies Default/Physical metadata, locked security and unknown addresses.
It does not populate session/security from HTTP headers or claim to propagate authenticated UDS context.
Core's current `DataError` cannot carry a typed UDS NRC, so NRCs are retained as `UDS NRC 0xXX` in an Internal error message.
A structured cross-protocol error model is an upstream integration requirement, not implemented by this prototype.

WriteDID, routines, fault-lib, process registration, and authenticated context mapping are deferred to the canonical
[migration plan](../../../MIGRATION_PLAN.md).

## Checks

```sh
bazel test --config=score_diag_x86_64_linux_qm //score/opensovd-core:read_did_bridge_test //score/opensovd-core:application_registration_test
```

The C++ tests cover request lifetime, metadata, byte copying and cancellation with a real unresolved promise.
The Rust tests cover NRCs, timeout, dropped reads, saturation and executor responsiveness.
The HTTP test exercises the registered application through the actual example topology.

## Application-Owned Registration

`RegisterApplication` accepts application ID, display name, hosting component ID, and a catalog of
`ReadResourceRegistration` entries. Each entry owns its resource ID/name, encoding and a shared C++ ReadDID handler.
Empty identifiers, missing handlers, unsupported encodings and duplicate resource IDs are rejected.
The component must be registered in Core by the integration owner; there is no external process registration protocol here.

A C++ application factory returns its opaque registration to Rust. `RegisteredApplication::from_registration` consumes
exclusive ownership of that handle; `into_app` copies metadata, clones resource-reader handles, builds the provider,
and releases the catalog. Core's App owns the resulting providers. Removing an App stops future routing; already-started
reads retain their handler state. The C++ library target is visible to SCORE packages so application code can supply its own factories.

Supported prototype encodings:
- Bytes: lossless JSON byte array.
- Temperature: exactly two bytes, signed big-endian centidegrees Celsius, decoded to a numeric degree-Celsius value.
- Availability: exactly one byte, 0 or 1, decoded to a boolean. Other sizes or values are rejected.

The `score-sensor` application is explicitly named a simulator. Its source publishes an initial 21.5 degC sample and can
publish updated or unavailable samples locally. The C++ readers return the current source state; no diagnostic write route
is introduced. Availability means the sample source is usable, not that the whole application or vehicle is healthy.
An unavailable or released source returns false availability and an NRC for temperature, not a fabricated zero reading.
Timestamp-based freshness and platform health sources are not implemented yet.

## Local Walkthrough

Start the server in one terminal (loopback only, no production authorization):

```sh
bazel run --config=score_diag_x86_64_linux_qm //score/opensovd-core:opensovd-gateway
```

Query it from another terminal:

```sh
curl --fail http://127.0.0.1:7690/sovd/v1/apps
curl --fail http://127.0.0.1:7690/sovd/v1/apps/score-sensor/is-located-on
curl --fail http://127.0.0.1:7690/sovd/v1/apps/score-sensor/data
curl --fail 'http://127.0.0.1:7690/sovd/v1/apps/score-sensor/data/temperature.celsius?include-schema=true'
curl --fail http://127.0.0.1:7690/sovd/v1/apps/score-sensor/data/sensor.healthy
```

The temperature read contains `data.value: 21.5`; the availability read contains `data.value: true`.
The integration test publishes -12.5 degC, unavailable and recovered samples, verifies them through HTTP,
then releases the source and removes the App. This proves changing source data and lifecycle behavior, not real sensor hardware.
