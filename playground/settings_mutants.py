import re
from collections import Counter

LOG = "playground/gates/lane_03_screen_settings.log"
OUT = "playground/settings_mutants.txt"


def main():
    surv = []
    acc = set()
    for line in open(LOG, encoding="utf-8", errors="replace"):
        m = re.match(r"SURVIVED\s+m(\d+) src/ui/screen_settings\.c:(\d+) \[([^\]]+)\]", line)
        if m:
            surv.append((int(m.group(1)), int(m.group(2)), m.group(3)))
        m = re.match(r"ACCEPTED\s+m(\d+)", line)
        if m:
            acc.add(int(m.group(1)))
    unexplained = [(mid, ln, mut) for mid, ln, mut in surv if mid not in acc]
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        for mid, ln, mut in unexplained:
            handle.write(f"m{mid:04d}\t{ln}\t{mut}\n")
    lines = Counter(ln for _, ln, _ in unexplained)
    print(f"unexplained {len(unexplained)} over {len(lines)} lines")
    print("line clusters:")
    for ln, n in sorted(lines.items()):
        print(f"  {ln}: {n}")


if __name__ == "__main__":
    main()
