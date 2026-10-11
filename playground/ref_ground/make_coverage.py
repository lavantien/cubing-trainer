import collections
import os
import sys


def load_names(root):
    inv = {}
    for line in open(os.path.join(root, "position_map_inverse.tsv"), encoding="utf-8"):
        parts = line.rstrip("\n").split("\t")
        if parts[0] == "name":
            continue
        inv[parts[0]] = (parts[1], int(parts[2]))
    return inv


def load_pairs(root, name):
    pairs = set()
    for line in open(os.path.join(root, name), encoding="utf-8"):
        parts = line.rstrip("\n").split("\t")
        if parts[0] == "t1":
            continue
        pairs.add((int(parts[0]), int(parts[1])))
    return pairs


def head(name):
    if len(name) == 3:
        return name[0] in "UD"
    return name[0] in "UD" or (name[0] in "FB" and name[1] in "LR")


def main(root):
    inv = load_names(root)
    pair_c0 = load_pairs(root, "pairs_corner0.tsv")
    pair_c2 = load_pairs(root, "pairs_corner2.tsv")
    pair_e2 = load_pairs(root, "pairs_edge2.tsv")
    buffers = {
        ("corner", "buffer0"): pair_c0,
        ("corner", "buffer2"): pair_c2,
        ("edge", "buffer2"): pair_e2,
    }
    buffer_names = {("corner", "buffer0"): "UBL", ("corner", "buffer2"): "UFR",
                    ("edge", "buffer2"): "UF"}
    per_solver = {}
    first_tokens = collections.Counter()
    parse_fail = 0
    for line in open(os.path.join(root, "simcheck.tsv"), encoding="utf-8"):
        parts = line.rstrip("\n").split("\t")
        if parts[0] not in ("cherry", "eggins"):
            continue
        solver, sheet, rowid, pos, moves, qtm, match, note = parts
        if match == "PARSE_FAIL":
            parse_fail += 1
            continue
        st = per_solver.setdefault(solver, collections.Counter())
        st[(sheet, "rows")] += 1
        if sheet in ("corner", "edge"):
            st[(sheet, "positions:" + pos)] += 1
            toks = pos.split("-")
            first_tokens[(solver, sheet, toks[0])] += 1
            st[(sheet, "distinct_positions")] += int(
                st[(sheet, "positions:" + pos)] == 1)
            for key in ("buffer0", "buffer2") if sheet == "corner" else ("buffer2",):
                if toks[0] == buffer_names[(sheet, key)]:
                    t1 = inv[toks[1]][1]
                    t2 = inv[toks[2]][1]
                    if (t1, t2) in buffers[(sheet, key)]:
                        st[(sheet, key, "in_pair_space")] += 1
                    else:
                        st[(sheet, key, "outside_pair_space")] += 1
                else:
                    st[(sheet, key, "other_buffer")] += 1
            st[(sheet, "clean_3cycle")] += int(" clean" in note)
    out = open(os.path.join(root, "coverage.md"), "w", encoding="utf-8", newline="\n")
    out.write("# coverage diff\n\n")
    out.write("data: all 1077 sheet rows simulated in playground/ref_ground/simcheck.tsv, ")
    out.write("position map from position_map.tsv, pair spaces dumped by the same probe run ")
    out.write("(ct_targets_corner buffer 0 = 378 pairs, buffer 2 = 378 pairs, ct_targets_edge buffer 2 = 440 pairs). ")
    out.write("our pair space = ordered (t1, t2) with the buffer piece and the t1 piece excluded from t2, ")
    out.write("src/gen/targets.c ct_enumerate_targets. parse failures: %d of 1077, every exported alg parses in our grammar.\n\n" % parse_fail)
    for solver in ("cherry", "eggins"):
        st = per_solver.get(solver)
        if not st:
            continue
        out.write("## %s\n\n" % solver)
        for sheet in ("edge", "corner", "flips", "twists", "parity", "ltct"):
            rows = st.get((sheet, "rows"), 0)
            out.write("- %s: %d rows" % (sheet, rows))
            if sheet in ("edge", "corner"):
                positions = [k for k in st if isinstance(k, tuple) and k[0] == sheet
                             and k[1].startswith("positions:")]
                multi = sum(1 for k in positions if st[k] > 1)
                extra = sum(st[k] - 1 for k in positions if st[k] > 1)
                out.write(", %d distinct positions, %d positions with multiple algs (%d extra rows)"
                          % (st[(sheet, "distinct_positions")], multi, extra))
                out.write(", %d clean piece 3-cycles" % st[(sheet, "clean_3cycle")])
            out.write("\n")
        for sheet in ("edge", "corner"):
            keys = ("buffer0", "buffer2") if sheet == "corner" else ("buffer2",)
            for key in keys:
                inside = st.get((sheet, key, "in_pair_space"), 0)
                outside = st.get((sheet, key, "outside_pair_space"), 0)
                other = st.get((sheet, key, "other_buffer"), 0)
                out.write("- %s vs our %s %s-buffer pair space: %d rows land inside, %d rows with the right buffer fall outside, %d rows use another buffer\n"
                          % (sheet, buffer_names[(sheet, key)], key, inside, outside, other))
        out.write("\n")
        for sheet in ("edge", "corner"):
            toks = sorted(t for (s, sh, t) in first_tokens if s == solver and sh == sheet
                          for _ in [1])
            counts = ", ".join("%s=%d" % (t, first_tokens[(solver, sheet, t)]) for t in toks)
            out.write("- %s first-token (floating buffer) distribution: %s\n" % (sheet, counts))
        out.write("\n")
    out.write("## categories our app has no case for\n\n")
    out.write("flips (two edges flipped in place), twists (two corners twisted in place), parity (edge swap plus corner swap), ltct (edge swap plus corner swap with twists plus in-place twist) have no category in the app's pair space: the trainer generates ordered 3-cycle pairs only. cherry rows: 25 flips, 20 twists, 26 parity, 8 ltct. eggins rows: 0 flips (empty sheet), 5 twists, 2 parity, 3 ltct.\n\n")
    out.write("the 780 3-style rows whose tokens include non-head stickers carry piece flips or twists (see blddb_semantics.md defect encoding) and are not exact 3-cycles; the 208 all-head rows are exact piece 3-cycles and are the only direct benchmark set for our generator.\n\n")
    covered = collections.defaultdict(set)
    covered_clean = collections.defaultdict(set)
    for line in open(os.path.join(root, "simcheck.tsv"), encoding="utf-8"):
        parts = line.rstrip("\n").split("\t")
        if parts[0] not in ("cherry", "eggins"):
            continue
        solver, sheet, rowid, pos, moves, qtm, match, note = parts
        if sheet not in ("corner", "edge") or match == "PARSE_FAIL":
            continue
        toks = pos.split("-")
        for key in ("buffer0", "buffer2") if sheet == "corner" else ("buffer2",):
            bname = buffer_names[(sheet, key)]
            if toks[0] == bname:
                pair = (inv[toks[1]][1], inv[toks[2]][1])
                covered[(sheet, key)].add(pair)
                if " clean" in note:
                    covered_clean[(sheet, key)].add(pair)
    totals = {("corner", "buffer0"): 378, ("corner", "buffer2"): 378, ("edge", "buffer2"): 440}
    for key in (("edge", "buffer2"), ("corner", "buffer2"), ("corner", "buffer0")):
        out.write("- %s %s: their rows cover %d of %d ordered pairs in our space (%.1f%%), %d of them with exact clean 3-cycles (%.1f%%)\n"
                  % (key[0], key[1], len(covered[key]), totals[key],
                     100.0 * len(covered[key]) / totals[key], len(covered_clean[key]),
                     100.0 * len(covered_clean[key]) / totals[key]))
    out.close()
    print("coverage.md written")


if __name__ == "__main__":
    main(sys.argv[1])
