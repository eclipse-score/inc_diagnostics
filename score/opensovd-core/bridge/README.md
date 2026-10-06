# ReadDID Provider Prototype

The private binding exposes an existing C++ `ReadDataByIdentifier` handler as a read-only Core data resource.
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

This is an in-process example, not remote application registration. The supplied C++ demo handler returns an asynchronous future;
applications can register their own handler through the C++ registration function, but there is no general Rust-facing registration API yet.
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
