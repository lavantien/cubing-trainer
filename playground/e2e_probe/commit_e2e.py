import os
import pathlib
import subprocess
import sys
import time

root = pathlib.Path(r"C:\Users\lavantien\dev\github\cubing-trainer")
idx_path = root / ".git" / "ct_e2e_index"


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


def stage_content(content):
    tmp = root / ".git" / "ct_e2e_stage_tmp"
    tmp.write_bytes(content.encode("utf-8"))
    h = run(["git", "hash-object", "-w", str(tmp)]).stdout.strip()
    tmp.unlink()
    return h


def apply_once(text, old, new, label):
    if text.count(old) != 1:
        raise SystemExit(f"{label}: expected exactly one match, got {text.count(old)}")
    return text.replace(old, new)


mk = run(["git", "show", "HEAD:Makefile"]).stdout
mk = apply_once(
    mk,
    "E2E_DIR := tests/e2e\n",
    "E2E_DIR := tests/e2e\nE2E_RUN_DIR := $(BUILDDIR)/e2e\n",
    "makefile e2e run dir",
)
mk = apply_once(
    mk,
    """\t@for case in $(E2E_DIR)/*.txt; do \\
\t\techo "e2e: $$case"; \\
\t\t$(APP_BIN) --headless --script "$$case" || exit 1; \\
\tdone""",
    """\t@for case in $(E2E_DIR)/*.txt; do \\
\t\tname=$$(basename "$$case" .txt); \\
\t\tsandbox=$(E2E_RUN_DIR)/$$name; \\
\t\techo "e2e: $$case"; \\
\t\trm -rf "$$sandbox" && mkdir -p "$$sandbox" || exit 1; \\
\t\tCT_DATA_DIR="$$sandbox" $(APP_BIN) --headless --script "$$case" || exit 1; \\
\tdone""",
    "makefile e2e recipe",
)

rd = run(["git", "show", "HEAD:README.md"]).stdout
rd = apply_once(
    rd,
    "The e2e script runner is the one pending piece in this set: while tests/e2e holds no script "
    "cases, make e2e exits 1 with a message naming the pending runner, so the gate fails honestly.",
    "make e2e drives the real app binary the same way: each tests/e2e/*.txt case runs "
    "cubing-trainer.exe --headless --script with CT_DATA_DIR pointed at a fresh "
    "build/e2e/<case> sandbox, so no case ever touches the real %LOCALAPPDATA% database, and any "
    "nonzero case exit fails the target.",
    "readme build paragraph",
)
rd = apply_once(
    rd,
    "src/app/main.c app.h app_loop.c headless.h headless.c paths.h paths.c",
    "src/app/main.c app.h app_loop.c headless.h headless.c paths.h paths.c script.h script.c "
    "script_expect.c",
    "readme module tree",
)
rd = apply_once(
    rd,
    "an unset LOCALAPPDATA or uncreatable directory aborts startup instead of running db-less "
    "(79ffeb5). The migrations directory",
    "an unset LOCALAPPDATA or uncreatable directory aborts startup instead of running db-less "
    "(79ffeb5). The CT_DATA_DIR environment override takes precedence over LOCALAPPDATA and is "
    "how make e2e sandboxes each case. The migrations directory",
    "readme data dir override",
)
rd = apply_once(
    rd,
    "startup seeding of the registered event packs (f0e8db9). Queued: the e2e script runner.",
    "startup seeding of the registered event packs (f0e8db9), and the e2e script runner with 5 "
    "cases over the real exe.",
    "readme sync line",
)
work_rd = (root / "README.md").read_text(encoding="utf-8")
work_start = work_rd.index("E2E: make e2e runs every tests/e2e/*.txt")
work_end = work_rd.index("\n\nVisual regression:", work_start)
e2e_new = work_rd[work_start:work_end]
e2e_old_start = rd.index("E2E: make e2e runs every tests/e2e/*.txt")
e2e_old_end = rd.index("\n\nVisual regression:", e2e_old_start)
rd = rd[:e2e_old_start] + e2e_new + rd[e2e_old_end:]

env = {**os.environ, "GIT_INDEX_FILE": str(idx_path)}
parent = run(["git", "rev-parse", "HEAD"]).stdout.strip()
run(["git", "read-tree", parent], env=env)
paths = [
    "src/app/script.h",
    "src/app/script.c",
    "src/app/script_internal.h",
    "src/app/script_expect.c",
    "src/app/main.c",
    "src/app/paths.c",
    "src/config/config.h",
    "tests/unit_e2e_script.c",
    "tests/fuzz_script.c",
    "tests/e2e/review_loop.txt",
    "tests/e2e/cfop_cards.txt",
    "tests/e2e/settings_merge.txt",
    "tests/e2e/trainer_generate.txt",
    "tests/e2e/startup_idempotent.txt",
    "playground/e2e_probe/main.c",
    "playground/e2e_probe/parse_probe.c",
    "playground/e2e_probe/commit_e2e.py",
]
for path in paths:
    blob = stage_content((root / path).read_text(encoding="utf-8"))
    run(["git", "update-index", "--add", "--cacheinfo", f"100644,{blob},{path}"], env=env)
for path, content in [("Makefile", mk), ("README.md", rd)]:
    blob = stage_content(content)
    run(["git", "update-index", "--add", "--cacheinfo", f"100644,{blob},{path}"], env=env)
tree = run(["git", "write-tree"], env=env).stdout.strip()
message = (
    "feat: run e2e script cases through the real app binary\n"
    "\n"
    "cubing-trainer.exe --headless --script <case> parses a script into a\n"
    "command queue and consumes it one command per step through the same\n"
    "model APIs the screens use, then asserts final DB state. make e2e\n"
    "sandboxes each case under build/e2e through the new CT_DATA_DIR\n"
    "override, so no case touches the real data directory."
)
commit = run(["git", "commit-tree", tree, "-p", parent, "-m", message]).stdout.strip()
run(["git", "update-ref", "refs/heads/main", commit, parent])
idx_path.unlink(missing_ok=True)
print(commit)
