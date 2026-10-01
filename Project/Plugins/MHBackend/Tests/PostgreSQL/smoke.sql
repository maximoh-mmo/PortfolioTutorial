\set ON_ERROR_STOP on
BEGIN;
CREATE TEMP TABLE mh_fixture_probe (
    namespace TEXT COLLATE "C" NOT NULL,
    record_key TEXT COLLATE "C" NOT NULL,
    payload BYTEA NOT NULL,
    PRIMARY KEY (namespace, record_key)
);
INSERT INTO mh_fixture_probe VALUES ('test', 'Key', decode('00ff01', 'hex'));
INSERT INTO mh_fixture_probe VALUES ('test', 'key', decode('', 'hex'));
DO $$
BEGIN
    IF (SELECT count(*) FROM mh_fixture_probe) <> 2 THEN
        RAISE EXCEPTION 'Case-sensitive keys failed';
    END IF;
    IF (SELECT encode(payload, 'hex') FROM mh_fixture_probe WHERE record_key = 'Key') <> '00ff01' THEN
        RAISE EXCEPTION 'Binary payload round trip failed';
    END IF;
    IF (SELECT octet_length(payload) FROM mh_fixture_probe WHERE record_key = 'key') <> 0 THEN
        RAISE EXCEPTION 'Empty payload round trip failed';
    END IF;
END $$;
ROLLBACK;
SELECT 'MH PostgreSQL fixture smoke test passed' AS result;

