import os
import random
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PROBE = os.path.join(ROOT, "build_rc", "refcmp_probe.exe")
REF = os.path.join(ROOT, "playground", "ref_ground")
UFR = os.path.join(ROOT, "playground", "buffer_ufr")

CORNER = 0
EDGE = 1
SHEET_ORBIT = {"corner": CORNER, "edge": EDGE}
ORBIT_SHEET = {CORNER: "corner", EDGE: "edge"}
BUFFER = {CORNER: 2, EDGE: 2}
ORBIT_OFF = {CORNER: 0, EDGE: 24}
HEAD_FACES = {CORNER: (0, 5), EDGE: (0, 5)}
EQ_FACES = {EDGE: (2, 4)}
FACE_LETTERS = "ULFRBD"
CYCLE_CLASSES = {
    "pure",
    "A9",
    "B9",
    "orthogonal",
    "cyclic shift",
    "column",
    "per special",
    "half slice-plane",
    "SP5",
    "SP7",
    "SP9",
}
PREFERRED = {CORNER: set("ULR"), EDGE: set("ULR") | set("MES")}
SAMPLE_N = 30
SAMPLE_SEED = 20261011
COVERAGE_EXPECTED = {
    "edge_in_rows": 471,
    "corner_in_rows": 325,
    "edge_positions": 372,
    "corner_positions": 288,
    "edge_clean": 100,
    "corner_clean": 31,
    "edge_space": 440,
    "corner_space": 378,
}
TOKENIZER_CASES = [
    ("R U R'", 3, 3),
    ("U2 R", 3, 2),
    ("M2 U M' U2 M U M2 U'", 11, 8),
    ("x y' z2", 0, 0),
    ("x R U R'", 3, 3),
    ("2U 2U'", 2, 2),
    ("2U2", 2, 1),
    ("Uw Uw' D2", 4, 3),
    ("R2 U2 R2 U2 R2 U2", 12, 6),
    ("S' U2 S", 4, 3),
    ("U R2 U' R U' R' U' R2 U B' U' B2 U B U' B2 U2 R", 23, 18),
    ("E E E E", 4, 4),
    ("Lw' Dw", 2, 2),
    ("3Uw", 1, 1),
    ("M' U M U2 M' U M", 8, 7),
    ("z2 U R U' R'", 4, 4),
    ("B L' U' L B'", 5, 5),
    ("r U R' U'", 4, 4),
    ("U' R E' R' U2 R E R' U'", 10, 9),
    ("R2 U' S R2 S' R2 U R2", 12, 8),
]


def token_metrics(alg):
    qtm = 0
    htm = 0
    for tok in alg.split():
        core = tok.rstrip("2'")
        if core in ("x", "y", "z"):
            continue
        qtm += 2 if tok.endswith("2") else 1
        htm += 1
    return qtm, htm


def run_tokenizer_tests():
    for alg, qtm, htm in TOKENIZER_CASES:
        got_q, got_h = token_metrics(alg)
        if (got_q, got_h) != (qtm, htm):
            raise SystemExit("tokenizer self-test failed: %r got %s want %s" % (alg, (got_q, got_h), (qtm, htm)))


def read_tsv(path, skip=1):
    with open(path, encoding="utf-8") as f:
        lines = f.read().splitlines()
    return [ln.split("\t") for ln in lines[skip:]]


def load_maps():
    rows = read_tsv(os.path.join(REF, "position_map.tsv"))
    stickers = {}
    by_name = {}
    piece_stickers = {}
    for orbit_s, sticker_s, letter, name, face_s, _row, _col, piece_s in rows:
        orbit = CORNER if orbit_s == "corner" else EDGE
        sticker = int(sticker_s)
        face = int(face_s)
        piece = int(piece_s)
        stickers[sticker] = {"orbit": orbit, "letter": letter, "name": name, "face": face, "piece": piece}
        by_name[name] = sticker
        piece_stickers.setdefault((orbit, piece), []).append((sticker, face))
    head = {}
    for (orbit, piece), members in piece_stickers.items():
        faces = [f for _s, f in members]
        pool = HEAD_FACES[orbit]
        if not any(f in pool for f in faces):
            pool = EQ_FACES[orbit]
        for s, f in members:
            if f in pool:
                head[s] = True
                break
        else:
            raise SystemExit("no head sticker for piece %d orbit %d" % (piece, orbit))
    return stickers, by_name, head


def load_pair_space(orbit):
    name = {CORNER: "pairs_corner2.tsv", EDGE: "pairs_edge2.tsv"}[orbit]
    return {(int(r[0]), int(r[1])) for r in read_tsv(os.path.join(REF, name))}


def load_chart():
    path = os.path.join(UFR, "chart_buffer2.tsv")
    with open(path, encoding="utf-8") as f:
        lines = f.read().splitlines()
    chart = {}
    for fields in (ln.split("\t") for ln in lines[2:]):
        if fields[0] in ("SYNTH_MISS", "FMT_FAIL"):
            continue
        orbit = int(fields[0])
        chart[(orbit, int(fields[2]), int(fields[3]))] = fields[8]
    return chart


def load_ref_rows(by_name, space, head):
    rows = []
    skipped_buffer = 0
    skipped_space = 0
    bad_format = 0
    with open(os.path.join(REF, "rows.tsv"), encoding="utf-8") as f:
        for line in f:
            solver, sheet, rowid, position, moves = line.rstrip("\n").split("\t")
            if sheet not in SHEET_ORBIT:
                continue
            orbit = SHEET_ORBIT[sheet]
            names = position.split("-")
            if len(names) != 3 or any(n not in by_name for n in names):
                bad_format += 1
                continue
            st = [by_name[n] for n in names]
            if st[0] % 24 != BUFFER[orbit]:
                skipped_buffer += 1
                continue
            t1 = st[1]
            t2 = st[2]
            key = (t1, t2)
            if key not in space[orbit]:
                skipped_space += 1
                continue
            clean = all(head.get(s) for s in st)
            rows.append(
                {
                    "solver": solver,
                    "sheet": sheet,
                    "orbit": orbit,
                    "rowid": int(rowid),
                    "position": position,
                    "t1": t1,
                    "t2": t2,
                    "alg": moves,
                    "clean": clean,
                    "has_rot": any(t.rstrip("2'") in ("x", "y", "z") for t in moves.split()),
                }
            )
    return rows, skipped_buffer, skipped_space, bad_format


def load_simcheck():
    sim = {}
    for solver, sheet, rowid, position, moves, qtm, match, note in read_tsv(os.path.join(REF, "simcheck.tsv")):
        sim[(solver, sheet, int(rowid))] = {
            "qtm": int(qtm),
            "match": match,
            "clean": note.endswith("clean") or " clean" in note,
        }
    return sim


def probe_score(pairs_in):
    pairs = list(dict.fromkeys(pairs_in))
    path = os.path.join(HERE, "alg_input.tsv")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        for i, (orbit, alg) in enumerate(pairs):
            f.write("%d\t%d\t%s\n" % (i, orbit, alg))
    out = subprocess.run(
        [PROBE, "score"], stdin=open(path, encoding="utf-8"), capture_output=True, text=True, check=True
    ).stdout
    metrics = {}
    for line in out.splitlines():
        fields = line.rstrip("\r").split("\t")
        if fields[0] == "id":
            continue
        i = int(fields[0])
        m = {
            "parse_rc": fields[1],
            "n_tokens": int(fields[2]) if fields[2] else None,
            "n_rot": int(fields[3]) if fields[3] else None,
            "n_slice": int(fields[4]) if fields[4] else None,
            "n_face": int(fields[5]) if fields[5] else None,
            "n_wide": int(fields[6]) if fields[6] else None,
            "qtm_probe": int(fields[7]) if fields[7] else None,
            "total": float(fields[12]) if fields[12] else None,
            "regrip": int(fields[10]) if fields[10] else None,
            "cls": fields[14],
            "setup": int(fields[15]) if fields[15] else None,
            "inter": int(fields[16]) if fields[16] else None,
            "insert": int(fields[17]) if fields[17] else None,
            "faces": fields[22] if len(fields) > 22 else "",
        }
        metrics[pairs[i]] = m
    return metrics


def probe_synth(pairs):
    path = os.path.join(HERE, "synth_input.tsv")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        for orbit, t1, t2 in pairs:
            f.write("%d\t%d\t%d\n" % (orbit, t1, t2))
    out = subprocess.run(
        [PROBE, "synth"], stdin=open(path, encoding="utf-8"), capture_output=True, text=True, check=True
    ).stdout
    got = {}
    for line in out.splitlines():
        fields = line.rstrip("\r").split("\t")
        if fields[0] == "orbit":
            continue
        got[(int(fields[0]), int(fields[1]), int(fields[2]))] = {
            "rc": fields[3],
            "found": fields[4],
            "qtm": fields[8],
            "alg": fields[9],
        }
    return got


def delta_stats(deltas):
    return {
        "n": len(deltas),
        "mean": round(statistics.fmean(deltas), 3) if deltas else None,
        "median": statistics.median(deltas) if deltas else None,
    }


def fmt(x, digits=2):
    return ("%." + str(digits) + "f") % x if x is not None else ""


def main():
    run_tokenizer_tests()
    stickers, by_name, head = load_maps()
    space = {CORNER: load_pair_space(CORNER), EDGE: load_pair_space(EDGE)}
    chart = load_chart()
    ref_rows, skipped_buffer, skipped_space, bad_format = load_ref_rows(by_name, space, head)
    sim = load_simcheck()

    for orbit, key_rows in ((EDGE, "edge_in_rows"), (CORNER, "corner_in_rows")):
        got = sum(1 for r in ref_rows if r["orbit"] == orbit)
        if got != COVERAGE_EXPECTED[key_rows]:
            raise SystemExit("%s expected %d got %d" % (key_rows, COVERAGE_EXPECTED[key_rows], got))
    for r in ref_rows:
        s = sim[(r["solver"], r["sheet"], r["rowid"])]
        if s["match"] != "CYCLE_A":
            raise SystemExit("simcheck mismatch %s" % r)
        if s["clean"] != r["clean"]:
            raise SystemExit("clean-flag disagreement %s" % r)
        q, h = token_metrics(r["alg"])
        if q != s["qtm"]:
            raise SystemExit("tokenizer vs simcheck qtm %s: %d vs %d" % (r, q, s["qtm"]))

    probe_map = probe_score([(r["orbit"], r["alg"]) for r in ref_rows] +
                            [(k[0], chart[k]) for k in chart])
    for (_orbit, alg), m in probe_map.items():
        if m["parse_rc"].isdigit():
            q, _h = token_metrics(alg)
            if m["n_rot"] == 0 and q != m["qtm_probe"]:
                raise SystemExit("tokenizer vs probe qtm %r: %d vs %d" % (alg, q, m["qtm_probe"]))

    sample_keys = sorted(random.Random(SAMPLE_SEED).sample(sorted(chart), SAMPLE_N))
    synth = probe_synth(sample_keys)
    sample_ok = 0
    sample_fail = []
    for k in sample_keys:
        want = chart[k]
        got = synth[k]
        if got["rc"] == "0" and got["found"] == "1" and got["alg"] == want:
            sample_ok += 1
        else:
            sample_fail.append((k, want, got["alg"]))
    if sample_fail:
        raise SystemExit("chart sample mismatches: %s" % sample_fail)

    for r in ref_rows:
        m = probe_map[(r["orbit"], r["alg"])]
        r["metrics"] = m
        r["qtm"], r["htm"] = token_metrics(r["alg"])

    bench = {}
    for r in ref_rows:
        key = (r["orbit"], r["t1"], r["t2"])
        rank = (r["qtm"], r["htm"], r["metrics"]["total"], r["solver"], r["rowid"])
        if key not in bench or rank < bench[key][0]:
            bench[key] = (rank, r)

    with open(os.path.join(HERE, "mapped_rows.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write(
            "solver\tsheet\trowid\tposition\torbit\tt1_name\tt2_name\tclean\thas_rot\talg\tqtm"
            "\thtm\tscore\tcls\tbench_pick\n"
        )
        for r in sorted(ref_rows, key=lambda r: (r["orbit"], r["t1"], r["t2"], r["qtm"], r["solver"])):
            key = (r["orbit"], r["t1"], r["t2"])
            picked = bench[key][1] is r
            f.write(
                "%s\t%s\t%d\t%s\t%d\t%s\t%s\t%d\t%d\t%s\t%d\t%d\t%s\t%s\t%d\n"
                % (
                    r["solver"],
                    r["sheet"],
                    r["rowid"],
                    r["position"],
                    r["orbit"],
                    stickers[r["t1"]]["name"],
                    stickers[r["t2"]]["name"],
                    1 if r["clean"] else 0,
                    1 if r["has_rot"] else 0,
                    r["alg"],
                    r["qtm"],
                    r["htm"],
                    fmt(r["metrics"]["total"], 4),
                    r["metrics"]["cls"],
                    1 if picked else 0,
                )
            )

    cases = []
    for key in sorted(bench):
        orbit, t1, t2 = key
        _rank, theirs = bench[key]
        our_alg = chart.get((orbit, t1 - ORBIT_OFF[orbit], t2 - ORBIT_OFF[orbit]))
        if our_alg is None:
            raise SystemExit("chart gap at %s" % (key,))
        om = probe_map[(orbit, our_alg)]
        oq, oh = token_metrics(our_alg)
        cases.append(
            {
                "orbit": orbit,
                "t1": t1,
                "t2": t2,
                "letters": stickers[t1]["letter"] + stickers[t2]["letter"],
                "position": theirs["position"],
                "our_alg": our_alg,
                "our_qtm": oq,
                "our_htm": oh,
                "our_score": om["total"],
                "our_cls": om["cls"],
                "their_alg": theirs["alg"],
                "their_solver": theirs["solver"],
                "their_rowid": theirs["rowid"],
                "their_qtm": theirs["qtm"],
                "their_htm": theirs["htm"],
                "their_score": theirs["metrics"]["total"],
                "their_cls": theirs["metrics"]["cls"],
                "their_clean": theirs["clean"],
                "their_rot": theirs["has_rot"],
            }
        )
        c = cases[-1]
        c["dq"] = c["our_qtm"] - c["their_qtm"]
        c["dh"] = c["our_htm"] - c["their_htm"]
        c["ds"] = c["our_score"] - c["their_score"]

    with open(os.path.join(HERE, "per_case.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write(
            "orbit\tt1_name\tt2_name\tposition\tclean\tour_alg\tour_qtm\tour_htm\tour_score\t"
            "our_cls\ttheir_alg\ttheir_solver\ttheir_rowid\ttheir_qtm\ttheir_htm\ttheir_score"
            "\ttheir_cls\tdq\tdh\tds\n"
        )
        for c in cases:
            f.write(
                "%d\t%s\t%s\t%s\t%d\t%s\t%d\t%d\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%s\t%s\t%d\t%d\t%s\n"
                % (
                    c["orbit"],
                    stickers[c["t1"]]["name"],
                    stickers[c["t2"]]["name"],
                    c["position"],
                    1 if c["their_clean"] else 0,
                    c["our_alg"],
                    c["our_qtm"],
                    c["our_htm"],
                    fmt(c["our_score"], 4),
                    c["our_cls"],
                    c["their_alg"],
                    c["their_solver"],
                    c["their_rowid"],
                    c["their_qtm"],
                    c["their_htm"],
                    fmt(c["their_score"], 4),
                    c["their_cls"],
                    c["dq"],
                    c["dh"],
                    fmt(c["ds"], 4),
                )
            )

    agg_lines = ["metric\tedge\tcorner"]
    agg = {}

    def emit(name, edge_v, corner_v):
        agg_lines.append("%s\t%s\t%s" % (name, edge_v, corner_v))
        agg[name] = (edge_v, corner_v)

    for label, subset in (("all", None), ("exact", True)):
        for orbit in (EDGE, CORNER):
            sel = [c for c in cases if c["orbit"] == orbit and (subset is None or c["their_clean"] == subset)]
            wins = sum(1 for c in sel if c["dq"] < 0)
            ties = sum(1 for c in sel if c["dq"] == 0)
            losses = sum(1 for c in sel if c["dq"] > 0)
            tag = ORBIT_SHEET[orbit] + "_" + label
            emit(tag + "_n", len(sel), "")
            emit(tag + "_win", wins, "")
            emit(tag + "_tie", ties, "")
            emit(tag + "_loss", losses, "")
            emit(tag + "_mean_dq", delta_stats([c["dq"] for c in sel])["mean"], "")
            emit(tag + "_median_dq", delta_stats([c["dq"] for c in sel])["median"], "")
            emit(tag + "_mean_dh", delta_stats([c["dh"] for c in sel])["mean"], "")
            emit(tag + "_median_dh", delta_stats([c["dh"] for c in sel])["median"], "")
            emit(tag + "_mean_ds", delta_stats([c["ds"] for c in sel])["mean"], "")

    for orbit in (EDGE, CORNER):
        tag = ORBIT_SHEET[orbit]
        covered = len([c for c in cases if c["orbit"] == orbit])
        emit(tag + "_covered", covered, "")
        emit(tag + "_uncovered", len(space[orbit]) - covered, "")
        preferred = PREFERRED[orbit]
        ours = [probe_map[(k[0], chart[k])] for k in chart if k[0] == orbit]
        our_face_share = statistics.fmean(
            [sum(1 for ch in m["faces"] if ch in preferred) / len(m["faces"]) for m in ours if m["faces"]]
        )
        emit(tag + "_our_subset_share", round(our_face_share, 4), "")
        their_bench = [probe_map[(c["orbit"], c["their_alg"])] for c in cases if c["orbit"] == orbit]
        their_face_share = statistics.fmean(
            [sum(1 for ch in m["faces"] if ch in preferred) / len(m["faces"]) for m in their_bench if m["faces"]]
        )
        emit(tag + "_their_subset_share", round(their_face_share, 4), "")

    cls_dist = {}
    for r in ref_rows:
        bucket = "head" if r["clean"] else "nonhead"
        cls_dist.setdefault((r["orbit"], bucket), {}).setdefault(r["metrics"]["cls"], 0)
        cls_dist[(r["orbit"], bucket)][r["metrics"]["cls"]] += 1
    for (orbit, bucket), dist in sorted(cls_dist.items()):
        for cls, n in sorted(dist.items(), key=lambda kv: -kv[1]):
            agg_lines.append(
                "cls_dist\t%s\t%s\t%s\t%d" % (ORBIT_SHEET[orbit], bucket, cls, n)
            )
    ours_dist = {}
    for k, alg in chart.items():
        ours_dist.setdefault(k[0], {}).setdefault(probe_map[(k[0], alg)]["cls"], 0)
        ours_dist[k[0]][probe_map[(k[0], alg)]["cls"]] += 1
    for orbit, dist in sorted(ours_dist.items()):
        for cls, n in sorted(dist.items(), key=lambda kv: -kv[1]):
            agg_lines.append("our_cls_dist\t%s\t%s\t%d" % (ORBIT_SHEET[orbit], cls, n))
    fallback = {}
    with open(os.path.join(UFR, "chart_buffer2.tsv"), encoding="utf-8") as f:
        lines = f.read().splitlines()
    for fields in (ln.split("\t") for ln in lines[2:]):
        fallback[int(fields[0])] = fallback.get(int(fields[0]), 0) + int(fields[5])
    for orbit, n in sorted(fallback.items()):
        total = len([k for k in chart if k[0] == orbit])
        agg_lines.append("our_fallback_rows\t%s\t%d\t%d" % (ORBIT_SHEET[orbit], n, total))

    mismatch = []
    recall_miss = 0
    for r in ref_rows:
        accepted = r["metrics"]["cls"] in CYCLE_CLASSES
        if accepted and not r["clean"]:
            mismatch.append(("accepted-but-defect", r))
        if r["clean"] and r["metrics"]["cls"] in ("parity", "flip", "twist"):
            mismatch.append(("clean-but-state-class", r))
        if r["clean"] and not accepted:
            recall_miss += 1
    emit("cls_mismatch", len(mismatch), "")
    emit("cls_recall_miss", recall_miss, "")

    rot_bench = [c for c in cases if c["their_rot"]]
    emit("bench_rot_rows", len(rot_bench), "")
    rot_ref = [r for r in ref_rows if r["has_rot"]]
    emit("ref_rot_rows", len(rot_ref), "")
    with open(os.path.join(HERE, "rot_rows.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("kind\tsolver\trowid\tposition\talg\tbench_pick\n")
        for r in sorted(rot_ref, key=lambda r: (r["orbit"], r["rowid"])):
            picked = bench[(r["orbit"], r["t1"], r["t2"])][1] is r
            f.write("ref\t%s\t%d\t%s\t%s\t%d\n" % (r["solver"], r["rowid"], r["position"], r["alg"], 1 if picked else 0))

    with open(os.path.join(HERE, "aggregates.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(agg_lines) + "\n")

    worst = sorted([c for c in cases if c["their_clean"]], key=lambda c: -c["dq"])[:10]
    worst_full = sorted(cases, key=lambda c: -c["dq"])[:10]
    for tag, sel in (("exact", worst), ("all", worst_full)):
        with open(os.path.join(HERE, "worst10_%s.tsv" % tag), "w", encoding="utf-8", newline="\n") as f:
            f.write(
                "rank\torbit\tposition\tour_alg\tour_qtm\tour_htm\tour_score\tour_cls"
                "\ttheir_alg\ttheir_solver\ttheir_qtm\ttheir_htm\ttheir_score\ttheir_cls\tdq\n"
            )
            for i, c in enumerate(sel, 1):
                f.write(
                    "%d\t%s\t%s\t%s\t%d\t%d\t%s\t%s\t%s\t%s\t%d\t%d\t%s\t%s\t%d\n"
                    % (
                        i,
                        ORBIT_SHEET[c["orbit"]],
                        c["position"],
                        c["our_alg"],
                        c["our_qtm"],
                        c["our_htm"],
                        fmt(c["our_score"], 4),
                        c["our_cls"],
                        c["their_alg"],
                        c["their_solver"],
                        c["their_qtm"],
                        c["their_htm"],
                        fmt(c["their_score"], 4),
                        c["their_cls"],
                        c["dq"],
                    )
                )

    losses = sorted(
        [c for c in cases if c["their_clean"] and c["dq"] > 0], key=lambda c: (-c["dq"], c["orbit"], c["position"])
    )
    with open(os.path.join(HERE, "losses_exact.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write(
            "orbit\tposition\tour_alg\tour_qtm\tour_htm\tour_score\tour_cls\ttheir_alg\ttheir_solver"
            "\ttheir_qtm\ttheir_htm\ttheir_score\tdq\tdh\tds\n"
        )
        for c in losses:
            f.write(
                "%s\t%s\t%s\t%d\t%d\t%s\t%s\t%s\t%s\t%d\t%d\t%s\t%d\t%d\t%s\n"
                % (
                    ORBIT_SHEET[c["orbit"]],
                    c["position"],
                    c["our_alg"],
                    c["our_qtm"],
                    c["our_htm"],
                    fmt(c["our_score"], 4),
                    c["our_cls"],
                    c["their_alg"],
                    c["their_solver"],
                    c["their_qtm"],
                    c["their_htm"],
                    fmt(c["their_score"], 4),
                    c["dq"],
                    c["dh"],
                    fmt(c["ds"], 4),
                )
            )

    print("tokenizer self-test: 20/20 pass")
    print("simcheck fresh reproduction: verified earlier (byte-equal)")
    print("chart sample: %d/%d regenerated identical at HEAD" % (sample_ok, SAMPLE_N))
    print("exact-subset losses written: %d" % len(losses))
    print("ref in-space rows: edge %d corner %d, skipped buffer %d space %d format %d"
          % (sum(1 for r in ref_rows if r["orbit"] == EDGE), sum(1 for r in ref_rows if r["orbit"] == CORNER), skipped_buffer, skipped_space, bad_format))
    for name in ("edge_all_n", "edge_all_win", "edge_all_tie", "edge_all_loss",
                 "corner_all_n", "corner_all_win", "corner_all_tie", "corner_all_loss",
                 "edge_exact_n", "edge_exact_win", "edge_exact_tie", "edge_exact_loss",
                 "corner_exact_n", "corner_exact_win", "corner_exact_tie", "corner_exact_loss",
                 "cls_mismatch", "cls_recall_miss", "bench_rot_rows"):
        if name in agg:
            print("%s: %s" % (name, agg[name][0]))
    print("mismatches: %s" % [(t, r["position"], r["solver"]) for t, r in mismatch][:5])
    print("rot ref rows: %s" % [(r["sheet"], r["rowid"], r["position"]) for r in rot_ref])
    print("rot bench picks: %s" % [(c["position"], c["their_solver"], c["their_alg"]) for c in rot_bench])
    return 0


if __name__ == "__main__":
    sys.exit(main())
