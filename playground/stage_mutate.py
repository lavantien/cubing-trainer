import os
import pathlib
import subprocess
import sys
import time

root = pathlib.Path(r"C:\Users\lavantien\dev\github\cubing-trainer")
idx_path = root / ".git" / "ct_mutate_index"


def run(args, env=None, check=True):
    for attempt in range(10):
        p = subprocess.run(args, cwd=root, capture_output=True, text=True, env=env)
        if p.returncode == 0 or not check:
            return p
        if "index.lock" in p.stderr:
            time.sleep(0.5)
            continue
        sys.stderr.write(p.stderr)
        raise SystemExit(p.returncode)
    raise SystemExit("index.lock stayed busy")


def head(path):
    return run(["git", "show", "HEAD:" + path]).stdout


def work(path):
    return (root / path).read_text(encoding="utf-8")


def stage_content(path, content):
    tmp = root / ".git" / "ct_stage_tmp"
    tmp.write_bytes(content.encode("utf-8"))
    h = run(["git", "hash-object", "-w", str(tmp)]).stdout.strip()
    tmp.unlink()
    return h


def isolated_commit(message, files):
    env = {**os.environ, "GIT_INDEX_FILE": str(idx_path)}
    parent = run(["git", "rev-parse", "HEAD"]).stdout.strip()
    run(["git", "read-tree", parent], env=env)
    for path, blob in files:
        run(["git", "update-index", "--add", "--cacheinfo", f"100644,{blob},{path}"], env=env)
    tree = run(["git", "write-tree"], env=env).stdout.strip()
    commit = run(["git", "commit-tree", tree, "-p", parent, "-m", message]).stdout.strip()
    run(["git", "update-ref", "refs/heads/main", commit, parent])
    idx_path.unlink(missing_ok=True)
    return commit


mk = head("Makefile")
mk = mk.replace(
    "TEST_CFLAGS := $(CFLAGS) -g -fsanitize=undefined -fsanitize-trap=all\n",
    "TEST_CFLAGS := $(CFLAGS) -g -fsanitize=undefined -fsanitize-trap=all\nDEPFLAGS := -MMD -MP\n",
)
mk = mk.replace(
    "$(CC) $(CROSS_COMPILE) $(CFLAGS) -c $< -o $@",
    "$(CC) $(CROSS_COMPILE) $(CFLAGS) $(DEPFLAGS) -c $< -o $@",
)
mk = mk.replace(
    "$(CC) $(CROSS_COMPILE) $(TEST_CFLAGS) -c $< -o $@",
    "$(CC) $(CROSS_COMPILE) $(TEST_CFLAGS) $(DEPFLAGS) -c $< -o $@",
)
mk = mk.replace(
    "$(CC) $(CROSS_COMPILE) $(CFLAGS) $(RAYGUI_CFLAGS) -c $< -o $@",
    "$(CC) $(CROSS_COMPILE) $(CFLAGS) $(DEPFLAGS) $(RAYGUI_CFLAGS) -c $< -o $@",
)
mk = mk.replace(
    "SQLITE_OBJ := $(OBJDIR)/third_party/sqlite/sqlite3.o\n",
    "SQLITE_OBJ := $(OBJDIR)/third_party/sqlite/sqlite3.o\n"
    "DEPFILES := $(LIB_OBJ:.o=.d) $(LIB_TOBJ:.o=.d) $(TEST_OBJ:.o=.d) $(RAYLIB_OBJ:.o=.d) $(RAYGUI_OBJ:.o=.d)\n"
    "DEPFILES += $(SQLITE_OBJ:.o=.d)\n-include $(DEPFILES)\n",
)
print(isolated_commit("build: track header dependencies in object rules", [("Makefile", stage_content("Makefile", mk))]))

mk = mk.replace(
    "CT_FUZZ_SUITES += fuzz_cube_invariants fuzz_cube_mirror\n",
    "CT_FUZZ_SUITES += fuzz_cube_invariants fuzz_cube_mirror\n"
    "CT_FUZZ_SUITES += fuzz_srs_walk fuzz_srs_elapsed fuzz_srs_grade_space\n"
    "CT_FUZZ_SUITES += fuzz_db_card_strings fuzz_db_review_rows fuzz_db_settings_keys\n"
    "CT_FUZZ_SUITES += fuzz_config_keys fuzz_config_moves\n",
)
print(isolated_commit("build: add srs db and config fuzz suites to fuzz target", [("Makefile", stage_content("Makefile", mk))]))

if mk != work("Makefile"):
    for i, (a, b) in enumerate(zip(mk.splitlines(), work("Makefile").splitlines())):
        if a != b:
            print(f"first diff at line {i + 1}: staged={a!r} final={b!r}")
            break
    raise SystemExit("staged Makefile diverges from working Makefile")
print("staging ok")
