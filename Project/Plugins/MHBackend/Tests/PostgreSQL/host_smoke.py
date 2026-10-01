"""Run the disposable fixture's SQL smoke test over host TCP using libpq.

Requires Python 3 and a libpq shared library (no Python packages).
"""
import argparse
import ctypes
import os
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--libpq", required=True, type=Path)
    parser.add_argument("--port", type=int, default=55432)
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("port must be between 1 and 65535")
    library = args.libpq.resolve(strict=True)
    dll_directory = os.add_dll_directory(str(library.parent)) if os.name == "nt" else None
    try:
        pq = ctypes.CDLL(str(library))
        for name, restype, argtypes in (
            ("PQconnectdb", ctypes.c_void_p, [ctypes.c_char_p]),
            ("PQstatus", ctypes.c_int, [ctypes.c_void_p]),
            ("PQexec", ctypes.c_void_p, [ctypes.c_void_p, ctypes.c_char_p]),
            ("PQresultStatus", ctypes.c_int, [ctypes.c_void_p]),
            ("PQclear", None, [ctypes.c_void_p]),
            ("PQfinish", None, [ctypes.c_void_p]),
            ("PQserverVersion", ctypes.c_int, [ctypes.c_void_p]),
        ):
            function = getattr(pq, name)
            function.restype = restype
            function.argtypes = argtypes
        # Intentionally restricted to the documented local disposable fixture.
        connection = pq.PQconnectdb(
            (f"host=127.0.0.1 port={args.port} dbname=mh_test user=mh_test "
             "password=mh_local_test_only connect_timeout=5 sslmode=disable "
             "options='-c statement_timeout=5000'").encode()
        )
        if not connection:
            raise RuntimeError("libpq could not allocate a connection")
        try:
            if pq.PQstatus(connection) != 0:
                raise RuntimeError("Fixture connection failed; check container health and published port")
            sql = Path(__file__).with_name("smoke.sql").read_text(encoding="utf-8")
            # ON_ERROR_STOP is a psql command, not SQL. PQexec stops on SQL errors.
            sql = "\n".join(line for line in sql.splitlines() if not line.startswith("\\"))
            result = pq.PQexec(connection, sql.encode())
            if not result:
                raise RuntimeError("Fixture SQL returned no result")
            try:
                if pq.PQresultStatus(result) != 2:  # PGRES_TUPLES_OK from final SELECT
                    raise RuntimeError("Fixture SQL smoke test failed")
            finally:
                pq.PQclear(result)
            print(f"PASS: host TCP/password connection, SQL smoke test; server version {pq.PQserverVersion(connection)}")
        finally:
            # Also rolls back if execution failed before the explicit ROLLBACK.
            pq.PQfinish(connection)
    finally:
        if dll_directory:
            dll_directory.close()


if __name__ == "__main__":
    main()
