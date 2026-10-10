import importlib.util
import os
import shutil
import sqlite3
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTDIR = os.path.join(ROOT, "build", "release_selftest")

EXE_REL = "dist/bin/cubing-trainer.exe"
MIGRATIONS = ["001_init.sql", "002_next.sql"]
PACKS = ["a.tsv", "b.tsv"]
EXPECTED_ROWS = 3


def load_module():
    spec = importlib.util.spec_from_file_location(
        "release_verify", os.path.join(ROOT, "tools", "release_verify.py")
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(data)


def build_tree(base):
    release = os.path.join(base, "release")
    repo_migrations = os.path.join(base, "repo_migrations")
    repo_packs = os.path.join(base, "repo_packs")
    write(os.path.join(release, *EXE_REL.split("/")), b"exe bytes")
    for name in MIGRATIONS:
        write(os.path.join(release, "db", "migrations", name), b"-- migration")
        write(os.path.join(repo_migrations, name), b"-- migration")
    for name in PACKS:
        write(os.path.join(release, "db", "seeds", "packs", name), b"x\ty\n")
        write(os.path.join(repo_packs, name), b"x\ty\n")
    return release, repo_migrations, repo_packs


def build_zip(path, release, extra=None, backslash=True, prefix=None):
    sep = "\\" if backslash else "/"
    parts = ([prefix] if prefix else []) + EXE_REL.split("/")
    with zipfile.ZipFile(path, "w") as zf:
        zf.write(os.path.join(release, *EXE_REL.split("/")), sep.join(parts))
        for name in MIGRATIONS:
            zf.write(
                os.path.join(release, "db", "migrations", name),
                sep.join(([prefix] if prefix else []) + ["db", "migrations", name]),
            )
        for name in PACKS:
            zf.write(
                os.path.join(release, "db", "seeds", "packs", name),
                sep.join(([prefix] if prefix else []) + ["db", "seeds", "packs", name]),
            )
        if extra is not None:
            zf.writestr(extra, b"stowaway")


def build_db(path, version, rows):
    con = sqlite3.connect(path)
    con.execute("pragma user_version = " + str(version))
    con.execute("create table event_pack (id integer primary key)")
    con.executemany("insert into event_pack values (?)", [(i,) for i in range(rows)])
    con.commit()
    con.close()


def expect(name, got, want):
    status = "PASS" if got == want else "FAIL"
    print(f"{status} {name}: got {got!r} want {want!r}")
    return got == want


def main():
    rv = load_module()
    os.makedirs(OUTDIR, exist_ok=True)
    for entry in os.listdir(OUTDIR):
        target = os.path.join(OUTDIR, entry)
        if os.path.isdir(target):
            shutil.rmtree(target)
        else:
            os.remove(target)
    results = []

    case = os.path.join(OUTDIR, "case1")
    os.makedirs(case, exist_ok=True)
    release, migrations, packs = build_tree(case)
    zip_path = os.path.join(case, "out.zip")
    build_zip(zip_path, release)
    db_path = os.path.join(case, "trainer.db")
    build_db(db_path, 2, EXPECTED_ROWS)

    results.append(expect("normalize_backslash", rv.normalize_entry("bin\\cubing-trainer.exe"), "bin/cubing-trainer.exe"))
    results.append(expect("normalize_dir_entry", rv.normalize_entry("db/migrations/"), ""))
    results.append(expect("layout_ok", rv.verify_layout(release, migrations, packs, EXE_REL), []))
    results.append(expect("zip_ok", rv.verify_zip(zip_path, release, ""), []))
    results.append(expect("db_ok", rv.verify_db(db_path, 2, EXPECTED_ROWS), []))

    os.remove(os.path.join(release, "db", "migrations", MIGRATIONS[1]))
    layout_failures = rv.verify_layout(release, migrations, packs, EXE_REL)
    results.append(expect("layout_missing_migration", len(layout_failures), 1))
    results.append(expect("layout_missing_names_file", MIGRATIONS[1] in layout_failures[0], True))

    results.append(expect("zip_missing_file", len(rv.verify_zip(zip_path, release, "")), 1))

    release2, migrations, packs = build_tree(case)
    zip2 = os.path.join(case, "out2.zip")
    build_zip(zip2, release2, extra="stowaway.txt")
    stowaway_failures = rv.verify_zip(zip2, release2, "")
    results.append(expect("zip_extra_entry", len(stowaway_failures), 1))
    results.append(expect("zip_extra_names_it", "stowaway.txt" in stowaway_failures[0], True))

    zip3 = os.path.join(case, "out3.zip")
    build_zip(zip3, release2, backslash=False)
    results.append(expect("zip_forward_slash_ok", rv.verify_zip(zip3, release2, ""), []))

    zip4 = os.path.join(case, "out4.zip")
    build_zip(zip4, release2, prefix="release")
    results.append(expect("zip_prefix_mismatch", len(rv.verify_zip(zip4, release2, "")), 1))
    results.append(expect("zip_prefix_ok", rv.verify_zip(zip4, release2, "release"), []))

    db2 = os.path.join(case, "trainer2.db")
    build_db(db2, 1, EXPECTED_ROWS)
    version_failures = rv.verify_db(db2, 2, EXPECTED_ROWS)
    results.append(expect("db_version_fail", len(version_failures), 1))
    results.append(expect("db_version_message", "user_version" in version_failures[0], True))

    db3 = os.path.join(case, "trainer3.db")
    build_db(db3, 2, EXPECTED_ROWS - 1)
    rows_failures = rv.verify_db(db3, 2, EXPECTED_ROWS)
    results.append(expect("db_rows_fail", len(rows_failures), 1))
    results.append(expect("db_rows_message", "event_pack" in rows_failures[0], True))

    results.append(expect("db_missing_file", len(rv.verify_db(os.path.join(case, "no.db"), 2, EXPECTED_ROWS)), 1))

    passed = sum(results)
    total = len(results)
    print(f"release_verify selftest: {passed}/{total} cases met expectations")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
