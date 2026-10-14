import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_03_screen_settings.log"

ROWS = [
    (55, "hex reject sentinel is sign only, callers test below zero so minus two rejects identically"),
    (91, "errno preset to a non erange value never equals erange and strtol leaves it untouched on success, the range check decides identically"),
    (110, "zero fill fallback is dead, the compiled in default palette is valid six digit hex on every face"),
    (131, "scheme at the second guard is always at least 1, the skip path saw a positive default id and the seeded path rereads the default the seed just guaranteed"),
    (192, "holder probe start at minus one can only reassign holder the minus one sentinel it already holds, every real match at 0 to 23 overwrites it"),
    (255, "retention lower clamp boundary is idempotent, at the minimum both arms store the same value"),
    (257, "retention upper clamp boundary is idempotent, at the maximum both arms store the same value"),
    (272, "move seconds lower clamp boundary is idempotent, at the minimum both arms store the same value"),
    (302, "loop and capacity guards never engage, the format writes at most 9 face chars into the 32 byte settings buffer and the extra twelfth iteration looks up a face char of nul"),
    (395, "letter edit buffer bound, consumed only by the draw-side letter text box no headless suite exercises"),
    (403, "label cell size, consumed only by the draw-side label grid no headless suite exercises"),
    (432, "cell grid column arithmetic, consumed only by the draw path button layout no headless suite exercises"),
    (434, "cell rect composition, consumed only by the draw path button layout no headless suite exercises"),
    (443, "errno reset in the draw-side text apply helper, only the erange comparison reads it and no headless suite calls the helper"),
    (446, "validation disjunct in the draw-side text apply helper, no headless suite calls the helper"),
    (455, "label orbit loop bound, the label grid is write-only outside the draw path"),
    (456, "label orbit ternary, the label grid is write-only outside the draw path"),
    (458, "label sticker loop bound, the label grid is write-only outside the draw path"),
    (468, "label width layout, draw path only, no headless suite exercises it"),
    (474, "retention edit toggle, gui event state no headless suite can trigger"),
    (475, "retention edit close branch, gui event branch no headless suite can trigger"),
    (487, "seconds text box rect, draw path only, no headless suite exercises it"),
    (490, "seconds edit toggle, gui event state no headless suite can trigger"),
    (496, "idle seconds canonical refill, draw path only, no headless suite exercises it"),
    (501, "moves group y offset, draw layout only, no headless suite exercises it"),
    (506, "move cell width divisor, draw layout only"),
    (507, "move cell height divisor, draw layout only"),
    (516, "toggle state show bit, drawn toggle only, no headless suite exercises the draw path"),
    (517, "toggle label buffer bound, drawn toggle text only"),
    (518, "toggle gap divisor, draw layout only"),
    (519, "toggle rect x placement, draw layout only"),
    (520, "toggle rect y placement and size, draw layout only"),
    (522, "toggle callback guard, gui event branch no headless suite can trigger"),
    (529, "orbit grid cell width divisor, draw layout only"),
    (530, "orbit grid cell height divisor, draw layout only"),
    (541, "orbit array ternary inside the button callback, gui event branch no headless suite can trigger"),
    (542, "letter buffer stash in the button callback, gui event state no headless suite can trigger"),
    (546, "selected outline branch, drawn highlight only"),
    (549, "duplicate outline guard, drawn highlight only"),
    (572, "letters hint label rect, draw layout only"),
    (583, "edge grid placement, draw layout only"),
    (590, "half width divisor, draw layout only"),
    (595, "corner buffer group rect and grid placement, draw layout only"),
    (599, "edge buffer group rect and grid placement, draw layout only"),
    (602, "color band y offset, draw layout only"),
    (605, "swatch inset divisor, draw layout only"),
    (606, "face width divisor and inset, draw layout only"),
    (607, "inner y offset, draw layout only"),
    (613, "face label buffer and name index, drawn face label only"),
    (621, "swatch color compose, drawn swatch only"),
    (627, "value box y offset, draw layout only"),
    (628, "channel loop bound, drawn value box count only"),
    (629, "channel index compose, draw layout only"),
    (630, "edit bit pick, gui edit state no headless suite can trigger"),
    (631, "idle channel mirror, gui edit state no headless suite can trigger"),
    (633, "value box rect, draw layout only"),
    (634, "value box commit branch, gui event branch no headless suite can trigger"),
    (635, "edit flag toggle, gui event state no headless suite can trigger"),
    (636, "stage color call inside the commit branch, gui event branch no headless suite can trigger"),
    (646, "tab clamp lower boundary is idempotent, a tab at the general bound rewrites the same value"),
    (649, "tab clamp upper boundary is idempotent, a tab at the bld bound rewrites the same value"),
    (659, "reopen condition on static screen state, no public accessor reads the form between updates and no headless suite swaps dbs across update calls"),
    (660, "opened latch write, static screen state read only by the update path itself"),
    (665, "letter buffer seed from the staged map, draw-side edit buffer only"),
    (673, "null guard after the background clear, the flipped polarity only skips the draw body for a live app"),
    (681, "button y offset, draw layout only"),
    (682, "button width divisor, draw layout only"),
    (684, "content y offset, draw layout only"),
    (685, "content width inset, draw layout only"),
    (686, "content height, draw layout only"),
    (691, "tab range clamp, draw-side tab state only, the boundary values rewrite the same tab and the count arm falls into the general draw either way"),
    (707, "save call inside the save button callback, gui event branch no headless suite can trigger"),
    (709, "merge rejected counter inside the save callback, gui event branch no headless suite can trigger"),
    (710, "rejected counter reset inside the save callback, gui event branch no headless suite can trigger"),
    (713, "cancel call inside the cancel button callback, gui event branch no headless suite can trigger"),
    (716, "rejected status branch, drawn status bar text only"),
    (722, "status bar rect, draw layout only"),
]

KILLED = {118, 162, 252}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    existing_keys = {ln.split("\t")[0] for ln in lines if "\t" in ln}
    new = []
    skipped = []
    for ln, why in ROWS:
        key = f"src/ui/screen_settings.c:{ln}"
        if key in existing_keys:
            skipped.append(key)
        else:
            new.append(f"{key}\t{why}")
    merged = lines + new
    keys = [ln.split("\t")[0] for ln in merged if "\t" in ln]
    dups = sorted({k for k in keys if keys.count(k) > 1})
    with open(ALLOW, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(merged) + "\n")
    print(f"appended {len(new)} rows, skipped collisions {len(skipped)}: {skipped if skipped else 'none'}")
    print(f"total {len(merged)}, dup keys: {dups if dups else 'none'}")

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
