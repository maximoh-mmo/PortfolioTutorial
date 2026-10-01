# Async HTTP transport

`MHBackendHttp` exposes `FMHHttpClient`, `FMHHttpRequest` and `FMHHttpResponse`.
Unreal HTTP interfaces and its module dependency stay private. Construct, send,
shutdown and destroy the client on the game thread. Request handles can cancel
from any thread; all user callbacks and transport cleanup run on the game thread.

A successful `TMHResult<FMHHttpResponse>` means HTTP delivered a response, including
4xx/5xx. Inspect `IsSuccessStatus()` or `GetStatusError()` before treating the service
operation as successful. This retains error bodies and headers for protocol adapters.
Connection/start failures return Unavailable, transport timeouts return Timeout, and
cancellation returns Cancelled. HTTP status helpers map 401/403 to Unauthorized,
404 to NotFound, 409 to Conflict, 408/504 to Timeout, 429/other 5xx to Unavailable,
other 4xx to InvalidArgument, and remaining non-2xx to Internal.

Requests use an explicit timeout (30 seconds by default); no framework retry policy
is applied. Content-Type defaults to application/octet-stream when unspecified.
Pass application/json explicitly for JSON. The engine handles redirects according
to its HTTP implementation. Bodies are buffered; this API is for backend messages,
not unbounded downloads or streaming. Response headers retain duplicate header lines.
Never log raw authorization headers, response bodies or token-bearing URLs.

Basic validation rejects invalid schemes, empty authorities, embedded URL credentials,
control characters, invalid header/verb tokens and nonpositive/nonfinite timeouts.
HTTP and HTTPS are supported; production deployments should configure HTTPS.
Validation failures are deferred through the same MHCore completion path as I/O.

Cancellation first resolves MHCore's terminal gate, then its game-thread cleanup hook
removes the pending operation, unbinds the HTTP callback and cancels the engine request
before delivering the cancelled result. Late results cannot deliver twice. Cancellation
cannot undo a server-side write. Shutdown immediately cancels owned operations,
rejects new sends and is idempotent. Destroying the client calls Shutdown; do this while
the engine ticker and HTTP module are still running. Callbacks must use weak owners.

Tests: MH.Backend.Http.StatusMapping and Lifecycle use a private operation seam to
exercise duplicate/late callbacks, failed start, input validation, worker cancellation,
shutdown, destruction and received error responses. Loopback uses real Unreal HTTP.

To run the loopback test:

1. Start `python Scripts/Framework/http_fixture.py --port-file <scratch-port-file>`.
2. Read its assigned loopback port from that file.
3. Run UnrealEditor-Cmd with Onset.uproject, `-unattended -NullRHI -nosound`,
   `-ExecCmds="Automation RunTests MH."`, `-TestExit="Automation Test Queue Empty"`,
   and `-MHHttpTestBaseUrl=http://127.0.0.1:<port>`.
4. Stop the fixture after the test run. Without that argument, the loopback test
   reports that its fixture is absent; that is not evidence of an integration pass.

The fixture tests GET, POST binary echo/header forwarding, 404, timeout, cancellation
and deliberate connection closure. It binds only to 127.0.0.1 and does not use external
services. Existing Onset FHttpStore is unchanged; its migration remains a later step.
