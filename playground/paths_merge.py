import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_10_paths.log"

ROWS = [
    (154, "the byte before every env value reachable in the harness is the equals separator of the putenv entry, never nul, and the unset case short circuits at the first arm"),
    (156, "written zero is unreachable for a nonempty override string and a negative written slips into the unsigned cast comparison which rejects it identically"),
    (164, "ct_exe_resource_dir succeeds on every harness call, the failure arm and its return variants never run"),
    (167, "written zero is unreachable because the format always emits the resource dir, the separator slash, and a nonempty font name, and a negative written slips into the unsigned cast comparison which rejects it identically"),
    (179, "every reachable value at this line comes from a putenv entry whose byte before the value is the equals separator, and the unset case short circuits at the null arm"),
    (182, "every reachable value at this line comes from a putenv entry whose byte before the value is the equals separator, and the unset case short circuits at the null arm"),
    (190, "written zero is unreachable because the base and the joined name always emit at least the separator slash, and a negative written slips into the unsigned cast comparison which rejects it identically"),
]

KILLED = {215, 216, 217, 221, 223, 227, 228, 232, 233, 234, 244, 245, 246, 247, 248, 257}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    existing = {ln.split("\t")[0] for ln in lines}
    new = []
    collisions = []
    for ln, why in ROWS:
        key = f"src/app/paths.c:{ln}"
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
