# PostgreSQL provider

`FMHPostgresStore` implements `IMHPersistenceStore` in MHPersistencePostgres. libpq
types stay private; the module has no dependency on Onset or another provider.

## Build configuration

Set `MH_POSTGRES_ROOT` before invoking UnrealBuildTool (and before launching an IDE
that invokes it). The SDK root must contain `include/libpq-fe.h` and its dependent
headers. On Win64 it also needs `lib/libpq.lib` and `bin/libpq.dll` with its dependency
DLLs. The module stages the SDK's DLLs to the target binary output directory.
On Linux it expects `lib/libpq.so`; the host must install matching runtime libraries.
Linux support is declared but has not been compiled or exercised here.

For this checkout in PowerShell, from the repository root:

```powershell
$env:MH_POSTGRES_ROOT = (Resolve-Path Project/Source/Onset/ThirdParty/PostgreSQL).Path
```

The path is supplied by the host and is not hardcoded in the plugin. Extracted plugin
users provide their own SDK and its required redistribution notices. This change does
not copy or redistribute new third-party binaries.

Without an SDK, on unsupported platforms, or for a Client target, the module builds
without libpq and operations report `NotConfigured`. An explicitly supplied malformed
SDK causes a build error. Rebuild after changing the SDK setting; check the build log
and explicitly run integration tests rather than assuming the provider is enabled.

## API and storage

Construct on the game thread with `FMHPostgresConfig`: an explicit libpq connection
string, connection timeout (1-60 seconds, default 5), and statement timeout (1-60000
milliseconds, default 5000). No development credentials or fallback store are supplied
by the provider. Inject connection configuration only in trusted server code.

The provider opens a connection per operation on a worker thread and serializes jobs
per store. It overrides connection-string timeout/options with the configured timeouts,
UTF-8 encoding and a restricted search path. TLS settings remain the caller's choice.
Connection timeouts apply per libpq host; statement/lock timeouts are server-side,
not an end-to-end wall-clock deadline. DNS and network failures can take longer.

It creates `public.mh_records_v1` with text namespace/key columns using `C` collation,
a composite primary key and a non-null `BYTEA` payload. An advisory transaction lock
`(1296584786, 1)` serializes schema creation across provider instances. The connection
role needs permission to create/use this table in the dedicated database. Existing
incompatible schemas fail; no destructive migration is attempted. This v1 table is
independent of Onset's tables and SQLite files.

Record values use `PQexecParams`, with explicit binary bytea input/output. SQL text
never incorporates keys or payloads. An empty payload is an empty bytea, not NULL.
Upserts are atomic; delete is idempotent. Missing records report `NotFound`.
Statement/lock timeouts report `Timeout`; query SQLSTATE authentication/unique failures
map to `Unauthorized`/`Conflict`. Connection failures (including rejected credentials)
report `Unavailable`. Native diagnostic strings are not forwarded or logged.

Cancellation before worker execution skips I/O. Cancellation after I/O starts suppresses
completion but does not interrupt libpq or roll back a committed write. Shutdown rejects
new jobs and cancels queued results. Call `Shutdown()` then wait for `HasPendingIO()` to
be false while the game-thread ticker runs before unloading the module. As with SQLite,
destroying the facade retains worker state until the outstanding operation retires.

## Integration tests

Start the [local fixture](../../Project/Plugins/MHBackend/Tests/PostgreSQL/README.md).
Build with the SDK enabled. Run Unreal automation `MH.Persistence.Postgres` with
`-MHPostgresTest`. Tests deliberately connect only to the documented local test endpoint
on port 55432 with its public test credentials; never point them at production data.
Without the flag the tests log that they were not run. With the flag but without the
SDK they fail explicitly. A passing skipped run is not integration evidence.

Contract coverage: deferred game-thread completion, binary/empty values, UTF-8 and
SQL-shaped keys, namespace/case isolation, overwrite, delete/missing records,
pre-execution cancellation, invalid config/key, authentication failure, shutdown,
destruction and reopening. Records use a unique namespace and are deleted on success.
The timeout test holds the schema advisory lock and verifies server-side timeout
mapping. Run it on a disposable database without concurrent provider tests.

Connection and parameter behavior follow the official
[libpq connection](https://www.postgresql.org/docs/17/libpq-connect.html) and
[command execution](https://www.postgresql.org/docs/17/libpq-exec.html) documentation.

## Verified 2026-10-01

UE 5.8.2 OnsetEditor and Onset Win64 Development builds pass with unity disabled and
the SDK enabled. A separate module-only Win64 game compile passes with the SDK unset.
The full MH automation suite passes all eleven tests against the HTTP and PostgreSQL
fixtures. PostgreSQL contract and forced-timeout tests have no warnings or errors.
SQLiteContract collected an unrelated Unreal background HTTP connectivity warning
(sandbox proxy refused a Google connectivity probe), so the report counts ten clean
successes and one success with warnings. Existing Onset compiler deprecation warnings
remain. No dedicated-server or Linux validation is claimed.
