import re
import sys
from collections import Counter


def extract(log, scope_rel, out):
    surv = []
    acc = set()
    for line in open(log, encoding="utf-8", errors="replace"):
        m = re.match(r"SURVIVED\s+m(\d+) (\S+) \[([^\]]+)\]", line)
        if m and m.group(2).startswith(scope_rel):
            surv.append((int(m.group(1)), int(m.group(2).split(":")[1]), m.group(3)))
        m = re.match(r"ACCEPTED\s+m(\d+)", line)
        if m:
            acc.add(int(m.group(1)))
    unexplained = [(mid, ln, mut) for mid, ln, mut in surv if mid not in acc]
    with open(out, "w", encoding="utf-8", newline="\n") as handle:
        for mid, ln, mut in unexplained:
            handle.write(f"m{mid:04d}\t{ln}\t{mut}\n")
    lines = Counter(ln for _, ln, _ in unexplained)
    print(f"{scope_rel}: unexplained {len(unexplained)} over {len(lines)} lines")
    for ln, n in sorted(lines.items()):
        print(f"  {ln}: {n}")


if __name__ == "__main__":
    extract(sys.argv[1], sys.argv[2], sys.argv[3])
