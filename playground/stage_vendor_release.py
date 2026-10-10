import os
import pathlib
import subprocess
import sys
import time

root = pathlib.Path(r"C:\Users\lavantien\dev\github\cubing-trainer")
idx_path = root / ".git" / "ct_vendor_index"


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


def head(path):
    return run(["git", "show", "HEAD:" + path]).stdout


def work(path):
    return (root / path).read_text(encoding="utf-8")


def blob(content):
    tmp = root / ".git" / "ct_stage_tmp"
    tmp.write_bytes(content.encode("utf-8"))
    h = run(["git", "hash-object", "-w", str(tmp)]).stdout.strip()
    tmp.unlink()
    return h


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: anchor occurs {count} times, expected 1")
    return text.replace(old, new)


def commit_files(message, entries):
    for _ in range(10):
        parent = run(["git", "rev-parse", "HEAD"]).stdout.strip()
        env = {**os.environ, "GIT_INDEX_FILE": str(idx_path)}
        run(["git", "read-tree", parent], env=env)
        for path, content in entries:
            run(
                ["git", "update-index", "--add", "--cacheinfo", f"100644,{blob(content)},{path}"],
                env=env,
            )
        tree = run(["git", "write-tree"], env=env).stdout.strip()
        commit = run(["git", "commit-tree", tree, "-p", parent, "-m", message]).stdout.strip()
        p = run(["git", "update-ref", "refs/heads/main", commit, parent], check=False)
        idx_path.unlink(missing_ok=True)
        if p.returncode == 0:
            return commit
        time.sleep(0.5)
    raise SystemExit("ref race persisted")


def note_divergence(path, staged):
    if staged == work(path):
        return
    staged_lines = staged.splitlines()
    working_lines = work(path).splitlines()
    drift = sum(1 for pair in zip(staged_lines, working_lines) if pair[0] != pair[1])
    drift += abs(len(staged_lines) - len(working_lines))
    print(f"note: {path} working tree carries {drift} lines of unstaged foreign edits")


mk = replace_once(
    head("Makefile"),
    "tar.exe -xf $(DLDIR)/sqlite.zip -C third_party/sqlite --strip-components=1\n",
    "rm -rf $(DLDIR)/sqlite_x\n"
    "\tpython -I -m zipfile -e $(DLDIR)/sqlite.zip $(DLDIR)/sqlite_x\n"
    "\tmv $(DLDIR)/sqlite_x/sqlite-amalgamation-$(SQLITE_CODE)/* third_party/sqlite/\n"
    "\trm -rf $(DLDIR)/sqlite_x\n",
    "zip extraction fix",
)
print(commit_files("fix: extract the sqlite zip through python since gnu tar cannot read zip", [("Makefile", mk)]))

mk = replace_once(
    mk,
    "SQLITE_SHA3 := 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e\n",
    "SQLITE_SHA3 := 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e\n"
    "RAYLIB_SHA3 := 87fadf1148a547fda4a40dd9bcef059787085deb70515ef80a45839e27188cac\n"
    "RAYGUI_SHA3 := 723b0b86b71a5df7163ee48fcdf4afd0e44abfcf547326f848161b6c5682d6b7\n",
    "hash constants",
)
mk = replace_once(
    mk,
    "vendor: $(DLDIR)/raylib.stamp $(DLDIR)/raygui.stamp $(DLDIR)/sqlite.stamp\n",
    "vendor: $(DLDIR)/raylib.stamp $(DLDIR)/raygui.stamp $(DLDIR)/sqlite.stamp\n"
    "\tpython -I tools/vendor_verify.py $(DLDIR)/raylib.tar.gz $(RAYLIB_SHA3)\n"
    "\tpython -I tools/vendor_verify.py $(DLDIR)/raygui.tar.gz $(RAYGUI_SHA3)\n"
    "\tpython -I tools/vendor_verify.py $(DLDIR)/sqlite.zip $(SQLITE_SHA3)\n"
    "\n"
    "vendor-verify-selftest:\n"
    "\tpython -I tools/vendor_verify_selftest.py\n",
    "vendor target",
)
mk = replace_once(
    mk,
    "\tcurl.exe -fsL -o $(DLDIR)/raylib.tar.gz $(RAYLIB_URL)\n\tmkdir -p third_party/raylib\n",
    "\tcurl.exe -fsL -o $(DLDIR)/raylib.tar.gz $(RAYLIB_URL)\n"
    "\tpython -I tools/vendor_verify.py $(DLDIR)/raylib.tar.gz $(RAYLIB_SHA3)\n"
    "\tmkdir -p third_party/raylib\n",
    "raylib stamp verify",
)
mk = replace_once(
    mk,
    "\tcurl.exe -fsL -o $(DLDIR)/raygui.tar.gz $(RAYGUI_URL)\n\tmkdir -p third_party/raygui\n",
    "\tcurl.exe -fsL -o $(DLDIR)/raygui.tar.gz $(RAYGUI_URL)\n"
    "\tpython -I tools/vendor_verify.py $(DLDIR)/raygui.tar.gz $(RAYGUI_SHA3)\n"
    "\tmkdir -p third_party/raygui\n",
    "raygui stamp verify",
)
mk = replace_once(
    mk,
    "\tcurl.exe -fsL -o $(DLDIR)/sqlite.zip $(SQLITE_URL)\n\tmkdir -p third_party/sqlite\n",
    "\tcurl.exe -fsL -o $(DLDIR)/sqlite.zip $(SQLITE_URL)\n"
    "\tpython -I tools/vendor_verify.py $(DLDIR)/sqlite.zip $(SQLITE_SHA3)\n"
    "\tmkdir -p third_party/sqlite\n",
    "sqlite stamp verify",
)
mk = replace_once(
    mk,
    ".PHONY: all build vendor test diagrams",
    ".PHONY: all build vendor vendor-verify-selftest test diagrams",
    "phony list",
)
note_divergence("Makefile", mk)

readme = head("README.md")
readme = replace_once(
    readme,
    "sqlite3 CLI 3.53.4, and node with npx and mmdc (mermaid-cli). No MSYS2, no pkg-config.",
    "sqlite3 CLI 3.53.4, python 3 for the vendor, db-verify, and visual targets, and node with npx and mmdc (mermaid-cli). No MSYS2, no pkg-config.",
    "prerequisites",
)
readme = replace_once(
    readme,
    "Run `make vendor` first. It HEAD-checks each URL, downloads, verifies, and extracts the pinned third party sources under third_party/: the raylib 6.0 tarball, the raygui 5.0 header-only release, and the SQLite 3.53.4 amalgamation zip verified by SHA3-256.",
    "Run `make vendor` first. It HEAD-checks each URL, downloads, extracts the pinned third party sources under third_party/, and verifies every archive by SHA3-256 through tools/vendor_verify.py: the raylib 6.0 and raygui 5.0 tag tarballs and the SQLite 3.53.4 amalgamation zip. Each stamp recipe verifies before extracting and the vendor target re-verifies the kept archives in build/vendor on every run, so a corrupted or truncated download fails the target and needs build/vendor removed to re-download. The zip extracts through python's zipfile module instead of tar because GNU tar on the make shell cannot read zip while the Windows bsdtar can, which made the old tar line depend on which tar won PATH order.",
    "vendor paragraph",
)
readme = replace_once(
    readme,
    "| vendor | download, HEAD-check, verify, and extract raylib 6.0, raygui 5.0, and the sqlite amalgamation into third_party/ |",
    "| vendor | download, HEAD-check, SHA3-256 verify, and extract raylib 6.0, raygui 5.0, and the sqlite amalgamation into third_party/, re-verifying the kept archives every run |\n"
    "| vendor-verify-selftest | exercise the archive hash checker against synthetic matches, corruptions, and malformed hashes |",
    "vendor target row",
)
readme = replace_once(
    readme,
    "Current records, URLs HEAD-checked 2026-10-09:",
    "Current records, URLs HEAD-checked 2026-10-09, vendor archives re-downloaded and re-verified 2026-10-10:",
    "records intro",
)
readme = replace_once(
    readme,
    "| raylib 6.0 source tarball | https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz | reachable, verified live |\n"
    "| raygui 5.0 source tarball | https://github.com/raysan5/raygui/archive/refs/tags/5.0.tar.gz | reachable |\n"
    "| sqlite 3.53.4 amalgamation zip | https://sqlite.org/2026/sqlite-amalgamation-3530400.zip | SHA3-256 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e |",
    "| raylib 6.0 source tarball | https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz | SHA3-256 87fadf1148a547fda4a40dd9bcef059787085deb70515ef80a45839e27188cac, derived from the live tag archive 2026-10-10, no published checksum upstream |\n"
    "| raygui 5.0 source tarball | https://github.com/raysan5/raygui/archive/refs/tags/5.0.tar.gz | SHA3-256 723b0b86b71a5df7163ee48fcdf4afd0e44abfcf547326f848161b6c5682d6b7, derived from the live tag archive 2026-10-10, no published checksum upstream |\n"
    "| sqlite 3.53.4 amalgamation zip | https://sqlite.org/2026/sqlite-amalgamation-3530400.zip | SHA3-256 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e, published by sqlite.org for the then-current release and re-derived from the live artifact 2026-10-10 |",
    "source records rows",
)
note_divergence("README.md", readme)

print(
    commit_files(
        "feat: verify all vendor archives by sha3-256",
        [
            ("Makefile", mk),
            ("README.md", readme),
            ("tools/vendor_verify.py", work("tools/vendor_verify.py")),
            ("tools/vendor_verify_selftest.py", work("tools/vendor_verify_selftest.py")),
            ("playground/stage_vendor_release.py", work("playground/stage_vendor_release.py")),
        ],
    )
)
