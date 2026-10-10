import os
import pathlib
import re
import shutil
import sqlite3
import subprocess
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
USAGE = "usage: python -I tools/release_verify.py <release_dir> <zip> <script_case>"
EXE_REL = "dist/bin/cubing-trainer.exe"
DB_DIR_NAME = "cubing-trainer"
DB_FILENAME = "trainer.db"
EXPECTED_PACK_ROWS = 574
RUN_TIMEOUT_SECS = 30
MIGRATION_NAME_RE = re.compile(r"(\d+)_.*\.sql")


def normalize_entry(name):
    flat = name.replace("\\", "/")
    if flat == "" or flat.endswith("/"):
        return ""
    return flat


def tree_files(root):
    base = pathlib.Path(root)
    return {p.relative_to(base).as_posix() for p in base.rglob("*") if p.is_file()}


def latest_migration_version(migrations_dir):
    best = 0
    for entry in pathlib.Path(migrations_dir).glob("*.sql"):
        m = MIGRATION_NAME_RE.fullmatch(entry.name)
        if m:
            best = max(best, int(m.group(1)))
    return best


def verify_layout(release_dir, migrations_dir, packs_dir, exe_rel):
    failures = []
    exe = pathlib.Path(release_dir) / exe_rel
    if not exe.is_file():
        failures.append(f"missing packaged exe at {exe}")
    got_migrations = tree_files(pathlib.Path(release_dir) / "db" / "migrations")
    want_migrations = tree_files(migrations_dir)
    if not want_migrations:
        failures.append(f"no migrations found under {migrations_dir}")
    elif got_migrations != want_migrations:
        failures.append(f"migrations mismatch: release={sorted(got_migrations)} repo={sorted(want_migrations)}")
    got_packs = tree_files(pathlib.Path(release_dir) / "db" / "seeds" / "packs")
    want_packs = tree_files(packs_dir)
    if not want_packs:
        failures.append(f"no pack seeds found under {packs_dir}")
    elif got_packs != want_packs:
        failures.append(f"pack seeds mismatch: release={sorted(got_packs)} repo={sorted(want_packs)}")
    staged = tree_files(release_dir)
    expected = {exe_rel}
    expected |= {"db/migrations/" + name for name in want_migrations}
    expected |= {"db/seeds/packs/" + name for name in want_packs}
    if staged != expected:
        failures.append(f"staged tree mismatch: extra={sorted(staged - expected)} missing={sorted(expected - staged)}")
    return failures


def verify_zip(zip_path, release_dir, prefix):
    failures = []
    want = {(prefix + "/" + name) if prefix else name for name in tree_files(release_dir)}
    try:
        with zipfile.ZipFile(zip_path) as zf:
            entries = {}
            duplicates = set()
            for info in zf.infolist():
                name = normalize_entry(info.filename)
                if not name:
                    continue
                if name in entries:
                    duplicates.add(name)
                entries[name] = info
            if duplicates:
                failures.append(f"zip holds duplicate entries: {sorted(duplicates)}")
                return failures
            if set(entries) != want:
                extra = sorted(set(entries) - want)
                missing = sorted(want - set(entries))
                failures.append(f"zip content mismatch: extra={extra} missing={missing}")
                return failures
            for name in sorted(want):
                if zf.read(entries[name]) != (pathlib.Path(release_dir) / name.removeprefix(
                    prefix + "/" if prefix else ""
                )).read_bytes():
                    failures.append(f"zip entry {name} differs from the packaged file")
    except (OSError, zipfile.BadZipFile, KeyError) as exc:
        failures.append(f"cannot read zip {zip_path}: {exc}")
    return failures


def verify_db(db_path, expected_version, expected_rows):
    failures = []
    if not pathlib.Path(db_path).is_file():
        return [f"missing database at {db_path}"]
    try:
        con = sqlite3.connect(db_path)
    except sqlite3.Error as exc:
        return [f"cannot open {db_path}: {exc}"]
    try:
        version = con.execute("pragma user_version").fetchone()[0]
        if version != expected_version:
            failures.append(f"user_version: got {version} want {expected_version}")
        rows = con.execute("select count(*) from event_pack").fetchone()[0]
        if rows != expected_rows:
            failures.append(f"event_pack rows: got {rows} want {expected_rows}")
    except sqlite3.Error as exc:
        failures.append(f"database query failed on {db_path}: {exc}")
    finally:
        con.close()
    return failures


def clean_env():
    return {k: v for k, v in os.environ.items() if not k.startswith("CT_")}


def run_exe(exe, cwd, env, args):
    try:
        proc = subprocess.run(
            [exe, *args],
            cwd=cwd,
            env=env,
            capture_output=True,
            text=True,
            timeout=RUN_TIMEOUT_SECS,
        )
        return proc.returncode, proc.stderr, False
    except subprocess.TimeoutExpired:
        return None, "", True


def fresh_home(parent, name):
    home = os.path.join(parent, name)
    shutil.rmtree(home, ignore_errors=True)
    os.makedirs(home, exist_ok=True)
    return home


def main():
    if len(sys.argv) != 4:
        print(USAGE)
        return 2
    release_dir, zip_path, case_path = sys.argv[1], sys.argv[2], sys.argv[3]
    exe = os.path.join(release_dir, *EXE_REL.split("/"))
    migrations_dir = os.path.join(ROOT, "db", "migrations")
    packs_dir = os.path.join(ROOT, "db", "seeds", "packs")
    work = os.path.join(ROOT, "build", "release_verify")
    failures = verify_layout(release_dir, migrations_dir, packs_dir, EXE_REL)
    failures += verify_zip(zip_path, release_dir, os.path.basename(os.path.normpath(release_dir)))
    if failures:
        for failure in failures:
            print(f"release_verify FAIL: {failure}")
        return 1

    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    env = {**clean_env(), "LOCALAPPDATA": fresh_home(work, "run_home")}
    code, stderr, timed_out = run_exe(exe, work, env, ["--headless", "--script", os.path.abspath(case_path)])
    if timed_out:
        print(f"release_verify FAIL: packaged exe timed out after {RUN_TIMEOUT_SECS}s, headless support missing or hanging")
        return 1
    if code != 0:
        print(f"release_verify FAIL: packaged exe exited {code}: {stderr.strip()}")
        return 1
    db_path = os.path.join(env["LOCALAPPDATA"], DB_DIR_NAME, DB_FILENAME)
    failures = verify_db(db_path, latest_migration_version(migrations_dir), EXPECTED_PACK_ROWS)
    failures += fatal_checks(exe, work)
    for failure in failures:
        print(f"release_verify FAIL: {failure}")
    if failures:
        return 1
    print(
        f"release verify passed: layout, zip, migration to "
        f"{latest_migration_version(migrations_dir)}, {EXPECTED_PACK_ROWS} event_pack rows, fatal paths clean"
    )
    return 0


def fatal_checks(exe, work):
    failures = []
    env = {k: v for k, v in clean_env().items() if k != "LOCALAPPDATA"}
    code, stderr, timed_out = run_exe(exe, work, env, [])
    if timed_out or code != 1:
        failures.append(f"unset LOCALAPPDATA: expected exit 1, got {'timeout' if timed_out else code}")
    elif "data directory" not in stderr:
        failures.append(f"unset LOCALAPPDATA: stderr missing the data directory failure: {stderr.strip()}")
    home = fresh_home(work, "fatal_migrations_home")
    env = {**clean_env(), "LOCALAPPDATA": home, "CT_MIGRATIONS_DIR": os.path.join(home, "no_such_migrations")}
    code, stderr, timed_out = run_exe(exe, work, env, [])
    if timed_out or code != 1:
        failures.append(f"missing migrations dir: expected exit 1, got {'timeout' if timed_out else code}")
    elif "migration failed" not in stderr:
        failures.append(f"missing migrations dir: stderr missing the migration failure: {stderr.strip()}")
    return failures


if __name__ == "__main__":
    sys.exit(main())
