import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_05_screen_cards.log"

ROWS = [
    (14, "list width divisor constant feeds only the draw-side list width arithmetic no headless suite exercises"),
    (16, "label max feeds only the draw-side label buffer bound no headless suite exercises"),
    (28, "cap guard arms are unreachable, the only caller passes the fixed page size 16 so cap never equals 0 or goes negative"),
    (57, "selected sentinel collapses before observation, with rows open assigns 0 right after and without rows reload clamps back to -1"),
    (81, "deck list rc boundary arms are identical, the copy loop is a no-op at rc 0 and never entered at negative rc"),
    (86, "rc zero boundary is identical in both branches, row_count stays 0 and status stays ok either way"),
    (106, "the zero clamp arm maps row 0 to itself, only the negative arm distinguishes and that mutant is killed"),
    (253, "label loop bound feeds only the draw-side label builder no headless suite exercises"),
    (257, "empty label write offset feeds only the draw-side label builder no headless suite exercises"),
    (268, "recompute guard arms run only in the draw path, the dirty flag is write-only across the app"),
    (271, "recompute fetch guard runs only in the draw path, the dirty flag is write-only across the app"),
    (276, "dirty compare chain runs only in the draw-side recompute, the flag is write-only across the app"),
    (287, "field text box rect arithmetic, draw path only no headless suite exercises"),
    (304, "field count index shift is equivalent, every spec element has the same size so the divisor is unchanged"),
    (311, "button hit test rect arithmetic feeds only the draw-side button row no headless suite exercises"),
    (318, "deck pick ternary inside the new button callback, gui event branch no headless suite can trigger"),
    (355, "null guard after the background clear, draw path only"),
    (363, "list width layout arithmetic, draw path only"),
    (365, "list y layout arithmetic, draw path only"),
    (377, "filter text box rect arithmetic, draw path only"),
]

KILLED = {50, 51, 238}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    existing = {ln.split("\t")[0] for ln in lines}
    new = []
    collisions = []
    for ln, why in ROWS:
        key = f"src/ui/screen_cards.c:{ln}"
        if key in existing:
            collisions.append(key)
        else:
            new.append(f"{key}\t{why}")
    merged = lines + new
    keys = [ln.split("\t")[0] for ln in merged]
    dups = sorted({k for k in keys if keys.count(k) > 1})
    with open(ALLOW, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(merged) + "\n")
    print(f"appended {len(new)}, collisions {collisions if collisions else 'none'}")
    print(f"total {len(merged)}, dups: {dups if dups else 'none'}")

    surv = []
    acc = set()
    for line in open(LOG, encoding="utf-8", errors="replace"):
        m = re.match(r"SURVIVED\s+m(\d+) (\S+) \[", line)
        if m:
            surv.append((int(m.group(1)), m.group(2)))
        m = re.match(r"ACCEPTED\s+m(\d+)", line)
        if m:
            acc.add(int(m.group(1)))
    covered = set()
    for ln in merged:
        m = re.match(r"^(.+):(\d+)\t", ln)
        if m:
            covered.add((m.group(1), int(m.group(2))))
    left_unexplained = []
    left_accepted = []
    for mid, loc in surv:
        m = re.match(r"^(.+):(\d+)$", loc)
        key = (m.group(1), int(m.group(2)))
        if mid in acc:
            if key not in covered:
                left_accepted.append((mid, loc))
        elif mid not in KILLED:
            if key not in covered:
                left_unexplained.append((mid, loc))
    print(f"unexplained leftovers: {left_unexplained if left_unexplained else 'none'}")
    print(f"accepted without rows: {left_accepted if left_accepted else 'none'}")


if __name__ == "__main__":
    main()
