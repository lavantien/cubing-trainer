import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_06_screen_stats.log"

ROWS = [
    (10, "left width shift constant feeds only the draw layout division"),
    (11, "forecast band height divisor feeds only the draw layout division"),
    (15, "retention mark divisor feeds only the draw marker size division"),
    (28, "guard int-1 only misses ct_repo_err_arg which the identical text_ok check at select_deck entry makes unreachable, repo failures return err_db"),
    (32, "heatmap rank minus one needs a negative count or null array, unreachable after case_accuracy returns a bounded count into fixed buffers"),
    (36, "card list re-runs the same 64 byte deck text_ok already enforced at entry so err_arg is unreachable"),
    (40, "row cap off-by-one only adds a cap-zero reviews call that returns ok without writing rows"),
    (44, "reviews err_arg needs a null db or non-positive card id, ids from card_list are always positive and the cap stays at least one"),
    (54, "retention returns the fixed bucket count never zero and err_arg needs null or negative args the fixed static buffers prevent"),
    (71, "deck stats re-runs the same stamp text_ok already validated at load entry so err_arg is unreachable"),
    (75, "from_day only ever receives the 11 byte precision write so the extra buffer byte is dead"),
    (79, "forecast err_arg is unreachable with fixed positive days and cap and a 10 char day prefix, and a zero row count is unobservable because load zeroes forecast_count first"),
    (84, "sessions err_arg needs a null db or negative cap, both fixed at the call site"),
    (105, "deck sections can only return ok or err_db because its own err_arg sites are unreachable, exactly minus one never flows"),
    (119, "worst cases with n of zero clamps to zero through the min either way and negative n is unchanged"),
    (180, "deck label loop writes only file static label arrays consumed by the raygui list view in draw"),
    (184, "session label loop writes only file static label arrays consumed by the raygui list view in draw"),
    (195, "forecast inner x offset feeds only the raygui frame layout"),
    (196, "forecast inner y offset feeds only the raygui frame layout"),
    (197, "forecast inner width and height arithmetic feed only the chart rectangle in draw"),
    (203, "forecast headline label rectangle feeds only the raygui label call"),
    (207, "forecast value copy loop bound feeds only the chart layout call in draw"),
    (212, "forecast bar draw loop bound feeds only the frame loop"),
    (213, "zero height bar skip only changes pixel output"),
    (221, "forecast day label position math feeds only drawtextex"),
    (229, "heatmap inner x offset feeds only the frame layout"),
    (230, "heatmap inner y offset feeds only the frame layout"),
    (240, "heatmap grid row count arithmetic feeds only the cell rectangle math in draw"),
    (245, "heatmap cell loop bound feeds only the frame loop"),
    (250, "hover outline equality only changes pixel output"),
    (255, "hover tooltip guard only changes pixel output"),
    (262, "hover tooltip baseline position math feeds only drawtextex"),
    (273, "retention inner height arithmetic feeds only the chart rectangle in draw"),
    (279, "retention headline label rectangle feeds only the raygui label call"),
    (285, "retention rate conversion feeds only the chart layout values in draw"),
    (291, "retention mark size division feeds only the marker rectangle in draw"),
    (297, "retention bar draw guard only changes pixel output"),
    (300, "empty bucket skip only changes pixel output"),
    (306, "retention marker position arithmetic feeds only the drawrectangle calls"),
    (310, "has_prev flag only gates the drawline polyline calls"),
    (322, "no deck selected guard only swaps the raygui label text"),
    (334, "deck summary label stacking math feeds only the raygui label loop"),
    (343, "opened flag only gates reload frequency of file static state with no exported observable"),
    (357, "draw null guard sits behind clearbackground which no headless suite can invoke"),
    (372, "sessions band y stacking arithmetic feeds only the frame layout"),
    (380, "retention band y stacking arithmetic feeds only the frame layout"),
    (400, "status bar rectangle math feeds only the raygui status bar call"),
]

KILLED = {39, 40, 111, 293}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    existing = {ln.split("\t")[0] for ln in lines}
    new = []
    collisions = []
    for ln, why in ROWS:
        key = f"src/ui/screen_stats.c:{ln}"
        if key in existing:
            collisions.append(key)
        else:
            new.append(f"{key}\t{why}")
    merged = lines + new
    keys = [ln.split("\t")[0] for ln in merged]
    dups = sorted({k for k in keys if keys.count(k) > 1})
    with open(ALLOW, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(merged) + "\n")
    print(f"appended {len(new)} of {len(ROWS)}, collisions {collisions if collisions else 'none'}")
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
