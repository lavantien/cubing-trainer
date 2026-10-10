import re
import sys

N = 3
AXIS = {"U": (0, 1, 0), "D": (0, -1, 0), "L": (-1, 0, 0), "R": (1, 0, 0),
        "F": (0, 0, 1), "B": (0, 0, -1)}
FACE_IDX = {"U": 0, "L": 1, "F": 2, "R": 3, "B": 4, "D": 5}
IDX_FACE = "ULFRBD"

VIEW_R = {0: (1, 0, 0), 5: (1, 0, 0), 1: (0, 0, 1), 3: (0, 0, -1), 2: (1, 0, 0), 4: (-1, 0, 0)}
VIEW_U = {0: (0, 0, -1), 5: (0, 0, 1), 1: (0, 1, 0), 3: (0, 1, 0), 2: (0, 1, 0), 4: (0, 1, 0)}


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def scale(a, k):
    return (a[0] * k, a[1] * k, a[2] * k)


def rot_about_axis(v, axis_idx, quarter):
    x, y, z = v
    for _ in range(quarter % 4):
        if axis_idx == 0:
            x, y, z = x, -z, y
        elif axis_idx == 1:
            x, y, z = z, y, -x
        else:
            x, y, z = -y, x, z
    return (x, y, z)


def stickers():
    out = []
    for fname, normal in AXIS.items():
        for a in (-1, 0, 1):
            for b in (-1, 0, 1):
                if normal[0] != 0:
                    inplane = [0, 2 * a, 2 * b]
                elif normal[1] != 0:
                    inplane = [2 * a, 0, 2 * b]
                else:
                    inplane = [2 * a, 2 * b, 0]
                center = (normal[0] * 2 + inplane[0], normal[1] * 2 + inplane[1],
                          normal[2] * 2 + inplane[2])
                if center == scale(normal, 2):
                    continue
                out.append((center, normal))
    return out


STICKERS = stickers()


def ring_slot(face_idx, center):
    r = dot(center, VIEW_R[face_idx])
    u = dot(center, VIEW_U[face_idx])
    if r != 0 and u != 0:
        if r < 0 and u > 0:
            return 0
        if r > 0 and u > 0:
            return 1
        if r > 0 and u < 0:
            return 2
        return 3
    if r == 0:
        return 0 if u > 0 else 2
    return 1 if r > 0 else 3


PIECES_CORNER = {}
PIECES_EDGE = {}
IDX_CORNER = [None] * 24
IDX_EDGE = [None] * 24
for home in STICKERS:
    center, normal = home
    face = FACE_IDX[[k for k, v in AXIS.items() if v == normal][0]]
    slot = ring_slot(face, center)
    corner = center[0] != 0 and center[1] != 0 and center[2] != 0
    table, idx = (PIECES_CORNER, IDX_CORNER) if corner else (PIECES_EDGE, IDX_EDGE)
    table.setdefault(center, []).append(home)
    idx[face * 4 + slot] = home

assert all(s is not None for s in IDX_CORNER), "corner index mapping incomplete"
assert all(s is not None for s in IDX_EDGE), "edge index mapping incomplete"
assert len(PIECES_CORNER) == 8 and len(PIECES_EDGE) == 12


def apply_move(state, mv):
    face, dfrom, dto, quarter = mv
    normal = AXIS[IDX_FACE[face]]
    axis_idx = [i for i in range(3) if normal[i] != 0][0]
    sign = normal[axis_idx]
    k = (-quarter * sign) % 4
    lo = (N - 1) - 2 * (dto - 1)
    hi = (N - 1) - 2 * (dfrom - 1)
    out = []
    for (center, nrm) in state:
        comp = center[axis_idx] * sign
        if lo <= comp <= hi:
            center = rot_about_axis(center, axis_idx, k)
            nrm = rot_about_axis(nrm, axis_idx, k)
        out.append((center, nrm))
    return out


def simulate(moves):
    state = list(STICKERS)
    for mv in moves:
        state = apply_move(state, mv)
    return {home: now for home, now in zip(STICKERS, state)}


MV_RE = re.compile(r"\[(\d+),(\d+),(\d+),(\d+)\]")


def check_row(fields):
    orbit, buffer, t1, t2 = (int(fields[i]) for i in range(4))
    setup, fallback, length, qtm = (int(fields[i]) for i in range(4, 8))
    notation = fields[8]
    moves = [tuple(int(g) for g in mv) for mv in MV_RE.findall(fields[9])]
    problems = []
    if len(moves) != length:
        problems.append("len mismatch row=%d moves=%d" % (length, len(moves)))
    want_qtm = sum(2 if mv[3] == 2 else 1 for mv in moves)
    if want_qtm != qtm:
        problems.append("qtm mismatch row=%d moves=%d" % (qtm, want_qtm))
    if any(mv[3] < 1 or mv[3] > 3 for mv in moves):
        problems.append("bad quarter value")
    idx = IDX_CORNER if orbit == 0 else IDX_EDGE
    pieces = PIECES_CORNER if orbit == 0 else PIECES_EDGE
    cb, c1, c2 = idx[buffer], idx[t1], idx[t2]
    dest = simulate(moves)
    if dest[cb] != c1 or dest[c1] != c2 or dest[c2] != cb:
        problems.append("cycle wrong for buffer=%d t1=%d t2=%d" % (buffer, t1, t2))
    moved = [h for h in STICKERS if dest[h] != h]
    want_moved = set(pieces[cb[0]] + pieces[c1[0]] + pieces[c2[0]])
    if set(moved) != want_moved:
        problems.append("locality wrong for buffer=%d t1=%d t2=%d moved=%d want=%d" %
                        (buffer, t1, t2, len(moved), len(want_moved)))
    return problems, (orbit, buffer, t1, t2, notation, length, qtm, setup, fallback)


def main():
    rows = 0
    misses = 0
    failures = []
    header = sys.stdin.readline().split("\t")
    info = dict(p.split("=") for p in header[0].split(" "))
    assert int(info["corner_targets"]) == 378, info
    assert int(info["edge_targets"]) == 440, info
    assert sys.stdin.readline().startswith("orbit\tbuffer\tt1\tt2")
    ab_case = None
    for line in sys.stdin:
        fields = line.rstrip("\n").split("\t")
        if fields[0] == "SYNTH_MISS":
            misses += 1
            continue
        problems, key = check_row(fields)
        if problems:
            failures.extend(problems)
        if key[0] == 0 and key[2] == 0 and key[3] == 1:
            ab_case = key
        rows += 1
    print("rows=%d misses=%d failures=%d" % (rows, misses, len(failures)))
    for f in failures[:20]:
        print("FAIL " + f)
    if ab_case is not None:
        print("AB case: orbit=%d buffer=%d t1=%d t2=%d notation=%s len=%d qtm=%d setup=%d"
              " fallback=%d" % ab_case)
    return 1 if failures or misses or rows != 818 or ab_case is None else 0


if __name__ == "__main__":
    sys.exit(main())
