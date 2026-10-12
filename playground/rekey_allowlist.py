import re
import subprocess
import sys

BASE = "d6f3e4f"
HEAD = "HEAD"
ALLOW = "tests/mutant_allowlist.txt"
HUNK_RE = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


def line_map(path):
    diff = subprocess.run(
        ["git", "diff", "-U0", f"{BASE}..{HEAD}", "--", path],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.splitlines()
    mapping = {}
    old_line = 1
    new_line = 1
    for line in diff:
        m = HUNK_RE.match(line)
        if m:
            old_line = int(m.group(1))
            new_line = int(m.group(3))
            continue
        if line.startswith("+++") or line.startswith("---") or line.startswith("diff") or line.startswith("index"):
            continue
        if line.startswith("+"):
            new_line += 1
        elif line.startswith("-"):
            mapping[old_line] = None
            old_line += 1
        else:
            mapping[old_line] = new_line
            old_line += 1
            new_line += 1
    return mapping


def main():
    write = "--write" in sys.argv
    drop = "--drop" in sys.argv
    rows = []
    with open(ALLOW, encoding="utf-8") as handle:
        for raw in handle:
            raw = raw.rstrip("\n")
            if not raw or raw.startswith("#"):
                rows.append((raw, None))
                continue
            key, _, rest = raw.partition("\t")
            rows.append((raw, (key, rest)))
    maps = {}
    unmapped = []
    rewritten = []
    for raw, parsed in rows:
        if parsed is None:
            rewritten.append(raw)
            continue
        key, rest = parsed
        path, _, num = key.rpartition(":")
        if path not in maps:
            maps[path] = line_map(path)
        number = int(num)
        target = maps[path].get(number, number)
        if target is None:
            unmapped.append(raw)
            if not drop:
                rewritten.append(raw)
        else:
            rewritten.append(f"{path}:{target}\t{rest}")
    new_keys = {}
    for line in rewritten:
        if line and not line.startswith("#"):
            key = line.split("\t")[0]
            new_keys[key] = new_keys.get(key, 0) + 1
    dups = sorted(k for k, c in new_keys.items() if c > 1)
    print(f"rows total: {len([r for r in rows if r[1] is not None])}")
    print(f"unmapped (line deleted this wave): {len(unmapped)}")
    print(f"duplicate keys after rekey: {len(dups)}")
    for key in dups:
        print(f"  dup {key}")
    for raw in unmapped:
        print(f"  unmapped {raw.split(chr(9))[0]}")
    if write:
        with open(ALLOW, "w", encoding="utf-8", newline="\n") as handle:
            handle.write("\n".join(rewritten) + "\n")
        print("written")
    return 0


if __name__ == "__main__":
    sys.exit(main())
