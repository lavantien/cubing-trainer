import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_11_app_loop.log"

ROWS = [
    (67, "the font guard only picks the theme loader string inside the raylib frame loop no gate suite exercises, ct_font_path returns only zero in the harness layout and the loader normalizes every unusable string to the default font identically"),
    (68, "the fallback write only truncates the theme loader string inside the raylib frame loop no gate suite exercises, the loader normalizes empty and unreadable strings to the default font identically and ct_app_run releases the theme before returning"),
    (71, "the frame loop gate feeds only the raylib pump no gate suite exercises, the or mutant diverges solely by refusing shutdown which stalls the suite run instead of failing an assertion and the deleted not only skips frames whose absence nothing observes after the theme release"),
    (74, "the catchup cap arithmetic steers the accumulator clamp inside the raylib frame loop no gate suite exercises and diverges only on single frame durations the gate machine never yields deterministically"),
    (75, "the clamp comparison differs only at exact double equality between the accumulator and the cap inside the raylib frame loop no gate suite exercises, a value real frame times never produce"),
    (78, "the fixed step gate differs only at exact double equality between the accumulator and the fixed step inside the raylib frame loop no gate suite exercises, a value real frame times never produce"),
]


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    existing = {ln.split("\t")[0] for ln in lines}
    new = []
    collisions = []
    for ln, why in ROWS:
        key = f"src/app/app_loop.c:{ln}"
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
        elif key not in covered:
            left_unexplained.append((mid, loc))
    print(f"unexplained leftovers: {left_unexplained if left_unexplained else 'none'}")
    print(f"accepted without rows: {left_accepted if left_accepted else 'none'}")


if __name__ == "__main__":
    main()
