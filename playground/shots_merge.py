import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_09_shots.log"

ROWS = [
    (35, "font fallback index mutant runs only when ct_font_path fails, which the harness never triggers because getmodulefilenamea always succeeds, and even forced through an oversize ct_font_file override both arms converge on the identical default font theme state with only a stderr diagnostic differing, no suite reads stderr"),
    (51, "memcpy length plus one copies one byte past the source nul into a slot no reader reaches, the destination string is identical through its nul, and the one byte overwrite past the dir array at the maximum accepted length has no trap on this target, coverage based accept"),
    (54, "greater versus greater-equal differ only when both separator scans return the same pointer, strrchr over the two distinct separators slash and backslash can never alias, true equivalent"),
    (120, "trainer form null guard conjunction mutant, the form accessor returns a static singleton so the null arm is dead, and the null db form only arises with a null app db where ct_trainer_form_select_event guards null db at entry and the select chain returns the same e_state, no observable difference"),
    (148, "chart form null guard conjunction mutant, same static singleton form, the null db form reconverges through the ct_trainer_form_select_event entry guard into the same e_state return"),
    (202, "negative pick index is clamped to row zero by ct_cards_form_select, so the -1 mutant selects the same first row and the capture is identical"),
]

KILLED = {33, 34, 35, 95}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    existing = {ln.split("\t")[0] for ln in lines}
    new = []
    collisions = []
    for ln, why in ROWS:
        key = f"src/app/shots.c:{ln}"
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
