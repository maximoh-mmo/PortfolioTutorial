# Generic persistence

`IMHPersistenceStore` operates on opaque binary records keyed by `FMHRecordKey`:
namespace (1-128 TCHARs) and ID (1-512 TCHARs). Keys are exact and case-sensitive;
control characters are rejected. Providers must preserve empty payloads and binary
bytes, including NUL. The framework does not interpret JSON, player accounts,
character slots, inventory, quests or game schema versions.

Call Load/Put/Delete/Shutdown on the game thread. Completions are always deferred to
that thread through MHCore; handles may cancel from any thread. Load returns NotFound
for an absent key. Put atomically replaces one record (upsert, last writer wins).
Delete is idempotent. Providers serialize operations issued through one store in call
order. There is no multi-record transaction, optimistic revision or cross-store ordering
contract yet. Do not infer those guarantees in game adapters.

Cancellation before a queued operation begins skips it. Cancellation racing with I/O
may suppress a result after a write has committed; it is not rollback. Shutdown cancels
pending results and rejects new operations. Quiesce I/O before unloading provider modules.

## Reference provider

`FMHMemoryStore` is non-durable and lives in MHPersistence. It is a reference for contract
tests and offline prototypes, not a fallback when a durable provider fails. It exercises
key isolation, empty/binary values, upsert/delete semantics, cancellation and teardown.

## SQLite provider

`FMHSQLiteStore` is in MHPersistenceSQLite and privately depends on Unreal SQLiteCore.
Construct it with a dedicated file path in an existing directory. Empty paths and
`:memory:` are rejected; missing directories/open failures return errors. There is no
silent in-memory fallback and no automatic migration of Onset's existing database.

I/O runs on the thread pool. Each store processes one operation at a time, opens a
connection for that operation, binds parameters, finalizes statements and explicitly
closes the database before delivering completion. This favors a simple lifetime model
and correctness over connection-pooling performance. SQLite's busy timeout is 2 seconds.

The provider creates `mh_records_v1(namespace TEXT COLLATE BINARY, record_key TEXT
COLLATE BINARY, payload BLOB)` with a composite primary key. Values are parameter-bound;
empty bytes are stored as an empty BLOB, not SQL NULL. The table name versions the initial
provider schema. Future migrations need explicit compatibility tests; no destructive
schema update is performed by this version.

Shutdown does not block the game thread waiting for an in-flight transaction. Hold the
store, call Shutdown, and poll `HasPendingIO()` until false before deleting its database
or unloading the module. Destroying the facade cancels callbacks; captured worker state
keeps resources alive until the in-flight operation closes them. Engine process shutdown
still requires providers to quiesce while the ticker can dispatch completions.

Tests use unique scratch databases under Project/Saved and remove successful fixtures.
`MH.Persistence.SQLiteContract` covers binary/empty payloads, parameter binding with an
SQL-shaped key, case/namespace isolation, idempotent deletion, missing records, cancellation,
shutdown, configuration/I/O failure, and persistence across a fresh store instance.

The [PostgreSQL provider](PostgreSQL.md) implements the same record contract with a private,
host-supplied libpq SDK. PostgreSQL integration requires a disposable test database;
local fixture connectivity is verified. The [HTTP record provider](HttpPersistence.md)
implements a versioned binary protocol with a local in-memory service fixture. A durable
production implementation of that protocol remains host-owned. The existing Onset
endpoints carry game-specific schemas and cannot be moved unchanged into MH.

A reusable [PostgreSQL Docker fixture](../../Project/Plugins/MHBackend/Tests/PostgreSQL/README.md)
ships with the plugin source, including startup/cleanup commands and a rollback-only SQL
smoke test. Compose configuration and a host-side libpq smoke test pass against PostgreSQL
17.11. The user started the container; Docker engine management remains unavailable from
the coding sandbox. See PostgreSQL.md for the Unreal integration test entry points.
