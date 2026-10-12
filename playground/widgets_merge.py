import re

ALLOW = "tests/mutant_allowlist.txt"

WIDGETS = [
    (17, "ascii fill cap clause is never the binding term, the 95 codepoint range ends 481 slots before the 576 cap"),
    (20, "latin fill cap clause is never the binding term, the 401 codepoint range ends 81 slots before the 576 cap"),
    (23, "viet range end is never binding, the 576 cap truncates the fill ten codepoints before the range end"),
    (57, "empty font path reaches the loader either way and is rejected identically, only the stderr message differs and the return stays false"),
    (61, "atlas texture id is an opaque gpu handle, the default atlas takes id 1 and the custom atlas loads above it so the mutated literal never separates the branch"),
    (61, None),
    (61, None),
    (61, None),
    (61, None),
    (73, "zero id never reaches the guard with a live window, pre window a guisetfont of the unbound font normalizes back to the default inside guigetfont"),
    (73, "default atlas id is 1 and every custom atlas id is above it, the raised bound never changes the bind decision"),
    (91, "stale owned flag is reset at the top of every load before any read and raylib guards unloading the default font copy as a no-op"),
    (99, "count minus one slips into the clamped n and returns the same minus one through the empty loop"),
    (99, "cap minus one clamps n to the same minus one and returns it through the empty loop"),
    (102, "clamp ternary boundary at count equal to cap stores the same value in both arms"),
    (103, "zero fast path boundary, falling through skips the empty loop and returns the same zero"),
    (111, "max scan comparison at equality restores the same maximum, idempotent"),
    (116, "bar value gate at zero, both ternary arms yield zero for the equality case"),
    (129, "zero rows slip the weakened guard into the row bound check which rejects them with the same minus one"),
    (144, "zero cols slip the weakened guard into the column clamp which returns minus one, matching the guard outcome"),
    (153, "column clamp reachable only when the quotient reaches cols, the hit test rejects the right edge so the scaled quotient stays strictly below cols, defensive float guard"),
    (154, "column clamp store is dead code behind the unreachable clamp branch"),
    (156, "row clamp reachable only when the quotient reaches rows, the hit test rejects the bottom edge so the scaled quotient stays strictly below rows, defensive float guard"),
    (157, "row clamp store is dead code behind the unreachable clamp branch"),
    (164, "accuracy lower clamp boundary is idempotent, at zero both arms store zero"),
    (167, "accuracy upper clamp boundary is idempotent, at one both arms store one"),
    (173, "half boundary is the continuous crossing where both branches produce full red and green, identical color bytes"),
]

WIDGETS_EXTRA_61 = [
    "loadfontex returns a populated atlas or a fully zeroed font, both operands flip together so or matches and",
    "glyphcount cannot be zero behind a nonzero atlas id, the widened gate keeps the same outcome",
    "glyphcount equals the 576 codepoint budget on every reachable load, the lowered bound agrees",
    "glyphcount never goes negative, the minus one bound keeps every reachable comparison outcome",
]

MENU = [
    (86, "sizeof on any element of the uniform entry array yields the same element size, count unchanged"),
    (101, "quit branch needs a real gui mouse release inside the rect, no headless path can force the raygui pressed result"),
]

KILLED_WIDGETS = {2, 3, 4, 5, 7, 8, 11, 14, 16, 27, 35, 77}
KILLED_MENU = {27, 30, 31, 32, 54, 55, 56, 57, 58}


def main():
    rows = []
    extra = 0
    for ln, why in WIDGETS:
        if why is None:
            rows.append((ln, WIDGETS_EXTRA_61[extra]))
            extra += 1
        else:
            rows.append((ln, why))
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    existing = {ln.split("\t")[0] for ln in lines}
    new = []
    collisions = []
    for ln, why in rows:
        key = f"src/ui/widgets.c:{ln}"
        if key in existing:
            collisions.append(key)
        else:
            new.append(f"{key}\t{why}")
    for ln, why in MENU:
        key = f"src/ui/screen_menu.c:{ln}"
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

    for log, scope, killed in (
        ("playground/gates/lane_07_widgets.log", "src/ui/widgets.c", KILLED_WIDGETS),
        ("playground/gates/lane_08_screen_menu.log", "src/ui/screen_menu.c", KILLED_MENU),
    ):
        surv = []
        acc = set()
        for line in open(log, encoding="utf-8", errors="replace"):
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
        left_u = []
        left_a = []
        for mid, loc in surv:
            m = re.match(r"^(.+):(\d+)$", loc)
            key = (m.group(1), int(m.group(2)))
            if mid in acc:
                if key not in covered:
                    left_a.append((mid, loc))
            elif mid not in killed and key not in covered:
                left_u.append((mid, loc))
        print(f"{scope} unexplained leftovers: {left_u if left_u else 'none'}")
        print(f"{scope} accepted without rows: {left_a if left_a else 'none'}")


if __name__ == "__main__":
    main()
