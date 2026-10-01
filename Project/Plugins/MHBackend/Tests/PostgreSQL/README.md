# Local PostgreSQL test fixture

This directory travels with MHBackend source. Requires Docker with Linux containers
and Docker Compose supporting `up --wait`. No host PostgreSQL installation is needed.
Docker is development tooling only; the plugin does not launch Docker at runtime.

From this directory, in PowerShell or a POSIX shell:

```sh
docker compose up -d --wait --wait-timeout 90
docker compose exec -T -e PGPASSWORD=mh_local_test_only postgres psql -h 127.0.0.1 -U mh_test -d mh_test -v ON_ERROR_STOP=1 -f /tests/smoke.sql
docker compose ps
```

The smoke test uses a TCP/password connection inside the container and rolls back
its temporary table. It verifies basic SQL, binary/empty values and case-sensitive
keys. It does not yet test the Unreal PostgreSQL provider, which remains pending,
or connectivity from the host. Provider integration tests must use the host endpoint:

```text
host=127.0.0.1 port=55432 dbname=mh_test user=mh_test password=mh_local_test_only connect_timeout=5
```

Port 55432 avoids the normal local PostgreSQL port. Override `MH_POSTGRES_TEST_PORT`
in your shell if occupied, and use that port in the connection string. The published
port binds only to loopback. These public test credentials are exclusively for this
disposable fixture. Do not use them for a shared or production database.

Data lives in container tmpfs: stopping/recreating the container discards it. No
existing Onset database or host data directory is mounted. Remove this fixture with:

```sh
docker compose down
```

The default Compose project name is `mh-backend-postgres-test`. For parallel test
runs, supply distinct `docker compose -p <name>` project names on every command and
distinct host ports. PostgreSQL is pinned to major 17; the image tag receives patch
updates. See the [official image documentation](https://hub.docker.com/_/postgres).

If Docker reports an engine-pipe permission error, run these commands from a terminal
with Docker access. A running Docker Desktop alone does not grant sandbox access.

Validation on 2026-10-01: Compose configuration parsed successfully. Container startup
and SQL execution could not be verified from the coding sandbox because Docker engine
pipe access was denied; the permission tool could not represent the pipe path.
