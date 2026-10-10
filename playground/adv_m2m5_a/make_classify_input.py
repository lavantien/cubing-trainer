import re

IDX_FACE = "ULFRBD"
mv_re = re.compile(r"\[(\d+),(\d+),(\d+),(\d+)\]")

def fmt(moves):
    out = []
    for f, a, b, q in moves:
        s = IDX_FACE[int(f)]
        if int(b) > 1:
            s += "w"
        if int(q) == 2:
            s += "2"
        elif int(q) == 3:
            s += "'"
        out.append(s)
    return " ".join(out)

rows = [
    ("L F R' F' L' F R F'", "pure"),
    ("U' B D' B' U B D B'", "pure"),
    ("D F2 D' B2 D F2 D' B2", "pure"),
    ("R B L B' R' B L' B'", "pure"),
    ("U B' D' B U' B' D B", "pure"),
    ("L F' L B2 L' F L B2 L2", "A9"),
    ("R2 B2 R F R' B2 R F' R", "A9"),
    ("U R' B2 R F R' B2 R F' U'", "orthogonal"),
    ("F R' U2 R F' R' F U2 F' R", "cyclic shift"),
    ("D B' R2 B D' B' D R2 D' B", "cyclic shift"),
    ("R U2 R' U' L2 U R U' L2 U' R'", "column"),
    ("U F2 U' F2 U' R2 U F2 U F2 U' R2", "per special"),
    ("R2 S' R2 S", "half slice-plane"),
    ("U M' U2 M U", "SP5"),
    ("M2 U M' U2 M U M2", "SP7"),
    ("U' M U2 M' U2 M U2 M' U'", "SP9"),
    ("U F E F' U' F E' F'", "pure"),
    ("M F R' F' M' F R F'", "pure"),
    ("R' F R' S2 R F' R' S2 R2", "A9"),
    ("U F' E2 F U F' E2 F U2", "B9"),
    ("F' U R' E R U' R' E' R F", "orthogonal"),
    ("R U R' U' R' F R2 U' R' U' R U R' F'", "parity"),
    ("M' U M' U M' U2 M U M U M U2", "flip"),
    ("R U R' U R U2 R' L' U' L U' L' U2 L", "twist"),
    ("R U R' U'", ""),
    ("R U R' U R U2 R'", ""),
    ("R2 U2 R2 U2 R2 U2", ""),
    ("", ""),
    ("R", ""),
    (" ".join(["R U"] * 35), ""),
]
with open("playground/adv_m2m5_a/classify_known.txt", "w", encoding="utf-8") as fh:
    for alg, want in rows:
        fh.write("%s|%s\n" % (alg, want))

synth_re = re.compile(r"SYNTH orbit=(\d+) t1=(\d+) t2=(\d+) setup=(\d+) fallback=(\d+) alg len=(\d+) moves=(.*)")
per_len = {}
for line in open("playground/adv_m2m5_a/probe_synth_out.txt", encoding="utf-8"):
    m = synth_re.match(line)
    if not m:
        continue
    ln = int(m.group(6))
    per_len.setdefault(ln, []).append((m.group(1), m.group(2), m.group(3),
                                       [tuple(int(g) for g in mv) for mv in
                                        mv_re.findall(m.group(7))]))
with open("playground/adv_m2m5_a/classify_synth.txt", "w", encoding="utf-8") as fh:
    for ln in sorted(per_len):
        for orbit, t1, t2, moves in per_len[ln]:
            fh.write("%s|len%d_o%s_%s_%s\n" % (fmt(moves), ln, orbit, t1, t2))
print("lengths:", {k: len(v) for k, v in sorted(per_len.items())})
