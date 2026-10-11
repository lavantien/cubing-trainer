import sys

sys.path.insert(0, "playground/buffer_ufr")
from verify_chart import IDX_CORNER, IDX_EDGE, AXIS, FACE_IDX, dot, scale, ring_slot, N


def name_to_index(name, corner):
    normal = AXIS[name[0]]
    center = [0, 0, 0]
    for ch in name:
        for i in range(3):
            center[i] += AXIS[ch][i]
    center = scale(tuple(center), N - 1)
    face = FACE_IDX[name[0]]
    slot = ring_slot(face, center)
    return face * 4 + slot


def main():
    chart = {}
    with open(sys.argv[1], encoding="utf-8") as f:
        header = f.readline()
        f.readline()
        for line in f:
            fields = line.rstrip("\n").split("\t")
            if fields[0] in ("SYNTH_MISS", "FMT_FAIL"):
                continue
            orbit, buffer, t1, t2 = (int(x) for x in fields[:4])
            chart[(orbit, buffer, t1, t2)] = fields[8]

    checked = 0
    missing = []
    skipped = 0
    pair_format = 0
    triples = {}
    total = 0
    with open(sys.argv[2], encoding="utf-8") as f:
        f.readline()
        for line in f:
            fields = line.rstrip("\n").split("\t")
            if len(fields) < 8:
                continue
            total += 1
            solver, sheet, rowid, position = fields[:4]
            names = position.split("-")
            if len(names) != 3 or any(len(n) != 3 for n in names):
                pair_format += 1
                skipped += 1
                continue
            triples[names[0]] = triples.get(names[0], 0) + 1
            corner = sheet == "corner"
            buffer_name, t1_name, t2_name = names
            buffer = name_to_index(buffer_name, corner)
            want_buffer = 2 if corner else 2
            if buffer != want_buffer:
                skipped += 1
                continue
            t1 = name_to_index(t1_name, corner)
            t2 = name_to_index(t2_name, corner)
            key = (0 if corner else 1, buffer, t1, t2)
            if key not in chart:
                missing.append(position)
                continue
            checked += 1
    print("ref_rows=%d ref_triples=%d ref_pair_format=%d" % (total, sum(triples.values()),
                                                             pair_format))
    for name in sorted(triples):
        print("ref_triple_buffer %s %d" % (name, triples[name]))
    print("ref_shared=%d ref_missing=%d ref_skipped=%d" % (checked, len(missing), skipped))
    for m in missing[:10]:
        print("MISSING " + m)
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
