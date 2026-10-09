import pathlib
import re
import shutil
import subprocess
import sys

SQLITE3_EXE = "sqlite3"
INTEGRITY_OK = "ok"
MIGRATION_NAME_RE = re.compile(r"(\d+)_.*\.sql")
USAGE = "usage: python -I tools/db_verify.py <schema.sql> <db> <migrations_dir>"


def run_sql(exe, db, sql):
    proc = subprocess.run(
        [exe, "-batch", db], input=sql, text=True, capture_output=True
    )
    if proc.returncode != 0:
        raise RuntimeError(f"sqlite3 exited {proc.returncode}: {proc.stderr.strip()}")
    return proc.stdout


def latest_migration_version(migrations_dir):
    best = 0
    for entry in pathlib.Path(migrations_dir).glob("*.sql"):
        m = MIGRATION_NAME_RE.fullmatch(entry.name)
        if m:
            best = max(best, int(m.group(1)))
    return best


def main():
    if len(sys.argv) != 4:
        print(USAGE)
        return 2
    schema_sql_path, db_path, migrations_dir = sys.argv[1], sys.argv[2], sys.argv[3]
    exe = shutil.which(SQLITE3_EXE)
    if exe is None:
        print(f"db_verify: {SQLITE3_EXE} CLI not found on PATH")
        return 2
    failures = []
    try:
        integrity = run_sql(exe, db_path, "PRAGMA integrity_check;\n")
        got = [line for line in integrity.splitlines() if line]
        if got != [INTEGRITY_OK]:
            failures.append(f"integrity_check: {got}")

        version = run_sql(exe, db_path, "PRAGMA user_version;\n").strip()
        expected = str(latest_migration_version(migrations_dir))
        if version != expected:
            failures.append(f"user_version: got {version} want {expected}")

        schema_sql = pathlib.Path(schema_sql_path).read_text()
        rows = [line for line in run_sql(exe, db_path, schema_sql).splitlines() if line]
        passed = [line for line in rows if line.startswith("ok ")]
        failed = [line for line in rows if not line.startswith("ok ")]
        if not passed:
            failures.append(f"schema assertions produced no passing rows: {rows}")
        failures.extend(failed)
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"db_verify: {exc}")
        return 2
    for failure in failures:
        print(f"db_verify FAIL: {failure}")
    if failures:
        print(f"db verify failed: {len(failures)} failures")
        return 1
    print(f"db verify passed: integrity ok, user_version {version}, {len(passed)} schema asserts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
