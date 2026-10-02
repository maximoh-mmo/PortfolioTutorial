# Local HTTP record fixture

Requires Python 3 only. Start `python server.py --port-file record-port.txt` and use
the written port with `-MHRecordTestBaseUrl=http://127.0.0.1:<port>` when running Unreal
automation `MH.Persistence.HttpRecords`. Stop with Ctrl+C. No external account or
Docker setup is required; storage is in memory and restricted to the local fixture.

Protocol: `/v1/records/<unpadded-base64url-UTF8-namespace>/<unpadded-base64url-UTF8-id>`.
GET returns 200/raw binary or 404; PUT atomically upserts raw application/octet-stream
and returns 204; DELETE is idempotent and returns 204. All requests need
`Authorization: Bearer mh-local-test-only`. Do not use these credentials in production.

The fixture is intentionally not a deployable service. It has no durable database,
per-user authorization, TLS, rate limiting or production concurrency guarantees.
Fault injection roots `/unexpected`, `/unavailable` and `/slow` support the tests.
The host repository documents the full client contract in Docs/Framework/HttpPersistence.md.
