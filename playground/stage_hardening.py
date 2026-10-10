import os
import pathlib
import subprocess
import sys
import time

root = pathlib.Path(r"C:\Users\lavantien\dev\github\cubing-trainer")
idx_path = root / ".git" / "ct_hardening_index"


def run(args, env=None, check=True):
    for _ in range(10):
        p = subprocess.run(args, cwd=root, capture_output=True, text=True, env=env)
        if p.returncode == 0 or not check:
            return p
        if "index.lock" in p.stderr:
            time.sleep(0.5)
            continue
        sys.stderr.write(p.stderr)
        raise SystemExit(p.returncode)
    raise SystemExit("index.lock stayed busy")


def blob(path):
    tmp = root / ".git" / "ct_stage_tmp"
    tmp.write_bytes((root / path).read_bytes())
    h = run(["git", "hash-object", "-w", str(tmp)]).stdout.strip()
    tmp.unlink()
    return h


def commit_working(message, paths):
    for _ in range(10):
        parent = run(["git", "rev-parse", "HEAD"]).stdout.strip()
        staged = {p: blob(p) for p in paths}
        env = {**os.environ, "GIT_INDEX_FILE": str(idx_path)}
        run(["git", "read-tree", parent], env=env)
        for path, sha in staged.items():
            run(["git", "update-index", "--add", "--cacheinfo", f"100644,{sha},{path}"], env=env)
        tree = run(["git", "write-tree"], env=env).stdout.strip()
        commit = run(["git", "commit-tree", tree, "-p", parent, "-m", message]).stdout.strip()
        p = run(["git", "update-ref", "refs/heads/main", commit, parent], check=False)
        idx_path.unlink(missing_ok=True)
        if p.returncode == 0:
            return commit
        time.sleep(0.5)
    raise SystemExit("ref race persisted")


if "clean_env" not in run(["git", "diff", "HEAD", "--", "tools/release_verify.py"]).stdout:
    raise SystemExit("working tree no longer carries the hardening edits")
print(commit_working("fix: harden release verification from adversarial findings", [
    "Makefile",
    "tools/release_verify.py",
    "tools/release_verify_selftest.py",
]))
print(commit_working("docs: update spike report for the vendor verify replacement", [
    "playground/spike_link/REPORT.md",
    "playground/stage_hardening.py",
]))
