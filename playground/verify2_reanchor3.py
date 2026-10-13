import re
import subprocess
import sys

PATH = "tests/mutant_allowlist.txt"
FILES = ["src/app/script.c", "src/ui/screen_review.c", "src/trainer/session.c"]
HUNK_RE = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


def line_map(rel):
    diff = subprocess.run(["git", "diff", "-U0", "HEAD", "--", rel], capture_output=True,
                          text=True, check=True).stdout.splitlines()
    hunks = []
    for line in diff:
        m = HUNK_RE.match(line)
        if m:
            old_start = int(m.group(1))
            old_count = int(m.group(2) or "1")
            new_start = int(m.group(3))
            new_count = int(m.group(4) or "1")
            hunks.append((old_start, old_count, new_start, new_count))

    def map_line(n):
        delta = 0
        for old_start, old_count, new_start, new_count in hunks:
            if old_count == 0:
                if n >= old_start:
                    delta += new_count
            elif n >= old_start + old_count:
                delta += new_count - old_count
            elif n >= old_start:
                return None
        return n + delta

    return map_line


maps = {rel: line_map(rel) for rel in FILES}

HAND = {
    "src/app/script.c:78": 72,
    "src/app/script.c:430": 430,
    "src/ui/screen_review.c:336": 312,
    "src/ui/screen_review.c:341": 314,
}
DROP = {
    "src/ui/screen_review.c:21",
    "src/ui/screen_review.c:337",
    "src/ui/screen_review.c:338",
}

rows = [l.rstrip("\n") for l in open(PATH, encoding="utf-8") if l.strip() and not l.startswith("#")]
out = []
unmapped = []
dropped = []
for r in rows:
    key = r.split("\t")[0]
    file, _, line = key.rpartition(":")
    n = int(line)
    if key in DROP:
        dropped.append(key)
        continue
    if key in HAND:
        out.append(f"{file}:{HAND[key]}\t" + r.split("\t", 1)[1])
        continue
    if file not in maps:
        out.append(r)
        continue
    m = maps[file](n)
    if m is None:
        unmapped.append(key)
        continue
    out.append(f"{file}:{m}\t" + r.split("\t", 1)[1])

if unmapped:
    sys.exit(f"rows on deleted lines: {unmapped}")

with open(PATH, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out) + "\n")

keys = [r.split("\t")[0] for r in out]
dupes = sorted({k for k in keys if keys.count(k) > 1})
if dupes:
    sys.exit(f"duplicate keys after edit: {dupes}")
shifted = sum(1 for r, o in zip(rows, out) if r.split("\t")[0] != o.split("\t")[0])
print(f"rows {len(rows)} -> {len(out)} shifted {shifted} dropped {sorted(dropped)}")
