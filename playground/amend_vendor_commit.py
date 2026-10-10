import os
import pathlib
import subprocess
import sys
import time

root = pathlib.Path(r"C:\Users\lavantien\dev\github\cubing-trainer")
idx_path = root / ".git" / "ct_amend_index"
OLD_COMMIT = "bbcada022d8ff4bcc0209a02cce0738058312417"
MESSAGE = "feat: verify all vendor archives by sha3-256"
LEFTOVER = (
    "\tif command -v python >/dev/null 2>&1; then python -I -c 'import hashlib,sys;"
    "h=hashlib.sha3_256(open(sys.argv[1],\"rb\").read()).hexdigest();print(h);"
    'sys.exit(0 if h=="$(SQLITE_SHA3)" else 1)\' $(DLDIR)/sqlite.zip && echo sha3-256 verified '
    "> $(DLDIR)/sqlite.verify.log; else $(CC) $(CROSS_LINK) $(CFLAGS) -Ithird_party/sqlite "
    "playground/spike_link/sqlite_version.c $(SQLITE_SRC) -o $(DLDIR)/sqlite_verify.exe && "
    'test "$$($(DLDIR)/sqlite_verify.exe)" = "$(SQLITE_VERSION)" && echo version compile verified '
    "> $(DLDIR)/sqlite.verify.log; fi\n"
)


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


def blob(content):
    tmp = root / ".git" / "ct_stage_tmp"
    tmp.write_bytes(content.encode("utf-8"))
    h = run(["git", "hash-object", "-w", str(tmp)]).stdout.strip()
    tmp.unlink()
    return h


head = run(["git", "rev-parse", "HEAD"]).stdout.strip()
if head != OLD_COMMIT:
    raise SystemExit(f"HEAD moved to {head}, amend lost the race")
parent = run(["git", "rev-parse", OLD_COMMIT + "^"]).stdout.strip()
mk = run(["git", "show", OLD_COMMIT + ":Makefile"]).stdout
if mk.count(LEFTOVER) != 1:
    raise SystemExit(f"leftover line occurs {mk.count(LEFTOVER)} times, expected 1")
mk = mk.replace(LEFTOVER, "")
env = {**os.environ, "GIT_INDEX_FILE": str(idx_path)}
run(["git", "read-tree", OLD_COMMIT], env=env)
run(["git", "update-index", "--cacheinfo", f"100644,{blob(mk)},Makefile"], env=env)
tree = run(["git", "write-tree"], env=env).stdout.strip()
commit = run(["git", "commit-tree", tree, "-p", parent, "-m", MESSAGE]).stdout.strip()
p = run(["git", "update-ref", "refs/heads/main", commit, OLD_COMMIT], check=False)
idx_path.unlink(missing_ok=True)
if p.returncode != 0:
    sys.stderr.write(p.stderr)
    raise SystemExit("ref race: HEAD moved before the amend, use a follow-up commit instead")
print(commit)
