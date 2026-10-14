import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_04_screen_review.log"

ROWS = [
    (12, "speed table first entry, consumed only by the gui speed callback no headless suite triggers"),
    (17, "slider height divisor, drawn slider geometry only"),
    (19, "play button count, consumed only by the draw-side play button row"),
    (62, "scramble loop overrun applies parser frame residue one slot past the written inverses, it decodes as a zero turn on every suite alg so the extra apply is the identity"),
    (113, "edge probe conjunction flip, every miss memsets the row image empty so both disjuncts collapse to the same empty copy"),
    (144, "local card fill byte, the failure path never reads the local and the repo get memsets every field"),
    (157, "weakened parse clamp, every reachable parse error code is minus two or lower so the shifted bound still clamps to zero"),
    (174, "weakened parse clamp in reveal, zero maps to itself and every error code stays below the shifted bound"),
    (175, "minus one clamp replacement in reveal mirrors the original empty replay, the inverse helper rejects a negative count so no scramble or push happens either way"),
    (223, "speed min clamp boundary is idempotent, at the minimum both arms store the same value"),
    (235, "weakened negative scrub clamp maps zero to itself"),
    (238, "upper scrub clamp boundary is idempotent, at move_count both arms store the same value"),
    (243, "weakened parse clamp in scrub maps zero to itself and every error code stays below the shifted bound"),
    (244, "minus one clamp replacement in scrub mirrors the original empty replay"),
    (283, "any flag flip only matters for a digitless field, the wall clock stamp always carries digits"),
    (284, "weakened digit bound drops trailing nines from the day seed, a nine free stamp parses identically"),
    (289, "separator guard conjunction flip only matters for a digitless field, the stamp always parses"),
    (308, "scheme settings probe, write-only scheme state read only by the draw path"),
    (317, "open form memset fill, the subsequent form begin memsets the same struct wholesale"),
    (322, "camera square distance, draw-side camera geometry only"),
    (323, "camera depth sum, draw-side camera geometry only"),
    (325, "camera half width factors, draw-side camera geometry only"),
    (326, "camera aspect factors, draw-side camera geometry only"),
    (327, "camera diagonal divisor, draw-side camera geometry only"),
    (328, "camera pan factors, draw-side camera geometry only"),
    (329, "camera position offsets, draw-side camera geometry only"),
    (330, "camera target offset, draw-side camera geometry only"),
    (336, "year holder init, overwritten on every scan success and unused on failure"),
    (337, "month holder init, overwritten on every scan success and unused on failure"),
    (338, "day holder init, overwritten on every scan success and unused on failure"),
    (341, "auto-open limit seed, minus one takes the same unlimited queue path as zero"),
    (358, "opened reset in the escape branch, raylib input branch no headless suite can trigger"),
    (399, "button row width split, draw path only"),
    (400, "button row loop bound, draw path only"),
    (415, "play callback dispatch index, gui event branch no headless suite can trigger"),
    (428, "null guard after the background clear, draw path only"),
    (435, "pane width layout, draw path only"),
    (438, "right pane x layout, draw path only"),
    (439, "right pane width layout, draw path only"),
    (443, "scissor pane bounds, draw path only"),
    (444, "text pane background rect, draw path only"),
    (459, "empty card branch, draw path only"),
    (462, "status bar rect, draw path only"),
    (468, "memo image pick, drawn title text only"),
    (470, "memo color pick, drawn title color only"),
    (472, "memo title y advance, draw layout only"),
    (477, "reveal gate, mutant only flips when the reveal button paints, firing the button needs live mouse input that no suite injects, so no model state ever changes"),
    (478, "reveal button width division feeds only the button rectangle, pixel geometry with no model effect"),
    (479, "reveal button rectangle arithmetic feeds only the gui hit box, pixel geometry with no model effect"),
    (498, "slider height divisor, drawn slider geometry only"),
    (501, "scrub slider gate, gui slider return branch no headless suite can trigger"),
    (506, "status bar rect, draw path only"),
]

KILLED = {55, 58, 59, 60, 61, 62, 83, 85, 87, 45, 47, 166, 167, 172, 173, 174,
          182, 185, 186, 189, 225, 236}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    by_key = {}
    order = []
    for ln in lines:
        key = ln.split("\t")[0]
        if key not in by_key:
            order.append(key)
        by_key[key] = ln
    replaced = []
    for ln, why in ROWS:
        key = f"src/ui/screen_review.c:{ln}"
        row = f"{key}\t{why}"
        if key in by_key:
            if by_key[key] != row:
                replaced.append(key)
            by_key[key] = row
        else:
            order.append(key)
            by_key[key] = row
    merged = [by_key[k] for k in order]
    keys = [ln.split("\t")[0] for ln in merged]
    dups = sorted({k for k in keys if keys.count(k) > 1})
    with open(ALLOW, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(merged) + "\n")
    print(f"total {len(merged)}, replaced stale {len(replaced)}: {replaced if replaced else 'none'}")
    print(f"dup keys: {dups if dups else 'none'}")

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
