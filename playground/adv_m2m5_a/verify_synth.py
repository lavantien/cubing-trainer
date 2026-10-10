import re
import sys

N = 3
AXIS = {"U": (0, 1, 0), "D": (0, -1, 0), "L": (-1, 0, 0), "R": (1, 0, 0),
        "F": (0, 0, 1), "B": (0, 0, -1)}
FACE_IDX = {"U": 0, "L": 1, "F": 2, "R": 3, "B": 4, "D": 5}
IDX_FACE = "ULFRBD"

VIEW_R = {0: (1, 0, 0), 5: (1, 0, 0), 1: (0, 0, 1), 3: (0, 0, -1), 2: (1, 0, 0), 4: (-1, 0, 0)}
VIEW_U = {0: (0, 0, -1), 5: (0, 0, 1), 1: (0, 1, 0), 3: (0, 1, 0), 2: (0, 1, 0), 4: (0, 1, 0)}

CORNER_LETTER = {f: "ABCD" for f in range(6)}
EDGE_LETTER = {f: "ABCD" for f in range(6)}
for i in range(6):
    CORNER_LETTER[i] = chr(ord("A") + i * 4) + chr(ord("A") + i * 4 + 1) + \
        chr(ord("A") + i * 4 + 2) + chr(ord("A") + i * 4 + 3)
    EDGE_LETTER[i] = CORNER_LETTER[i]


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def scale(a, k):
    return (a[0] * k, a[1] * k, a[2] * k)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


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
        others = [k for k in "xyz"]
        for a in (-1, 0, 1):
            for b in (-1, 0, 1):
                inplane = [0, 0, 0]
                if normal[0] != 0:
                    inplane = [0, 2 * a, 2 * b]
                elif normal[1] != 0:
                    inplane = [2 * a, 0, 2 * b]
                else:
                    inplane = [2 * a, 2 * b, 0]
                center = (normal[0] * 2 + inplane[0], normal[1] * 2 + inplane[1],
                          normal[2] * 2 + inplane[2])
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


def sticker_name(st):
    center, normal = st
    fname = {v: k for k, v in AXIS.items()}[normal]
    if center == scale(normal, N - 1):
        return None, "center"
    slot = ring_slot(FACE_IDX[fname], center)
    if center[0] != 0 and center[1] != 0 and center[2] != 0:
        return CORNER_LETTER[FACE_IDX[fname]][slot], "corner"
    return EDGE_LETTER[FACE_IDX[fname]][slot], "edge"


LETTER_OF = {}
for st in STICKERS:
    letter, orbit = sticker_name(st)
    if letter is not None:
        LETTER_OF[st] = letter
def is_corner(st):
    c = st[0]
    return c[0] != 0 and c[1] != 0 and c[2] != 0


letters_seen = {}
for st, l in LETTER_OF.items():
    key = (l, st[1], is_corner(st))
    assert key not in letters_seen, ("duplicate letter/face", l, st)
    letters_seen[key] = st


def piece_of(st):
    return st[0]


def name_to_sticker(name):
    normal = AXIS[name[0]]
    center = [0, 0, 0]
    for ch in name:
        for i in range(3):
            center[i] += AXIS[ch][i]
    center = scale(tuple(center), N - 1)
    return (center, normal)


def apply_move(state, mv):
    face, dfrom, dto, quarter = mv
    fname = IDX_FACE[face]
    normal = AXIS[fname]
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
    dest = {}
    for home, now in zip(STICKERS, state):
        dest[home] = now
    return dest


def cycle_of(dest, home):
    seen = []
    cur = home
    while cur not in seen and dest[cur] != cur:
        seen.append(cur)
        cur = dest[cur]
    return seen


def check_alg(tag, moves, want_cycle_names, orbit_kind):
    dest = simulate(moves)
    problems = []
    moved = [h for h in STICKERS if dest[h] != h]
    want_homes = [name_to_sticker(nm) for nm in want_cycle_names]
    if dest[want_homes[0]] != want_homes[1] or dest[want_homes[1]] != want_homes[2] or \
            dest[want_homes[2]] != want_homes[0]:
        problems.append("cycle direction wrong: %s->%s %s->%s %s->%s" % (
            want_cycle_names[0], sticker_name(dest[want_homes[0]]),
            want_cycle_names[1], sticker_name(dest[want_homes[1]]),
            want_cycle_names[2], sticker_name(dest[want_homes[2]])))
    want_pieces = {p[0] for p in want_homes}
    for h in moved:
        if h[0] not in want_pieces:
            problems.append("extra moved sticker %s" % (sticker_name(h),))
    want_sticker_count = 9 if orbit_kind == "corner" else 6
    if len(moved) != want_sticker_count:
        problems.append("moved %d stickers, want %d" % (len(moved), want_sticker_count))
    return problems


FIXTURES = [
    ("corner", ["URB", "UBL", "ULF"],
     [[1, 1, 1, 1], [2, 1, 1, 3], [1, 1, 1, 1], [4, 1, 1, 2], [1, 1, 1, 3], [2, 1, 1, 1],
      [1, 1, 1, 1], [4, 1, 1, 2], [1, 1, 1, 2]]),
    ("corner", ["URB", "UBL", "UFR"],
     [[3, 1, 1, 2], [4, 1, 1, 2], [3, 1, 1, 1], [2, 1, 1, 1], [3, 1, 1, 3], [4, 1, 1, 2],
      [3, 1, 1, 1], [2, 1, 1, 3], [3, 1, 1, 1]]),
    ("corner", ["URB", "UBL", "FRU"],
     [[3, 1, 1, 1], [4, 1, 1, 1], [1, 1, 1, 1], [4, 1, 1, 3], [3, 1, 1, 3], [4, 1, 1, 1],
      [1, 1, 1, 3], [4, 1, 1, 3]]),
    ("corner", ["URB", "UBL", "FLD"],
     [[0, 1, 1, 3], [4, 1, 1, 1], [5, 1, 1, 3], [4, 1, 1, 3], [0, 1, 1, 1], [4, 1, 1, 1],
      [5, 1, 1, 1], [4, 1, 1, 3]]),
    ("corner", ["URB", "BRD", "UBL"],
     [[0, 1, 1, 1], [4, 1, 1, 3], [5, 1, 1, 3], [4, 1, 1, 1], [0, 1, 1, 3], [4, 1, 1, 3],
      [5, 1, 1, 1], [4, 1, 1, 1]]),
    ("edge", ["UR", "UF", "FD"],
     [[0, 1, 1, 1], [2, 1, 1, 1], [5, 2, 2, 1], [2, 1, 1, 3], [0, 1, 1, 3], [2, 1, 1, 1],
      [5, 2, 2, 3], [2, 1, 1, 3]]),
    ("edge", ["UR", "UF", "BR"],
     [[0, 1, 1, 1], [2, 1, 1, 1], [5, 2, 2, 3], [2, 1, 1, 3], [0, 1, 1, 3], [2, 1, 1, 1],
      [5, 2, 2, 1], [2, 1, 1, 3]]),
    ("edge", ["UR", "UF", "BL"],
     [[3, 1, 1, 3], [5, 2, 2, 2], [3, 1, 1, 1], [0, 1, 1, 3], [3, 1, 1, 3], [5, 2, 2, 2],
      [3, 1, 1, 1], [0, 1, 1, 1]]),
    ("edge", ["UR", "UF", "LU"],
     [[1, 1, 1, 1], [2, 1, 1, 3], [1, 1, 1, 3], [2, 2, 2, 3], [1, 1, 1, 1], [2, 1, 1, 1],
      [1, 1, 1, 3], [2, 2, 2, 1]]),
    ("edge", ["UR", "UF", "DR"],
     [[5, 1, 1, 1], [1, 2, 2, 2], [5, 1, 1, 3], [3, 1, 1, 2], [5, 1, 1, 1], [1, 2, 2, 2],
      [5, 1, 1, 3], [3, 1, 1, 2]]),
]

fail = 0
for kind, cycle, moves in FIXTURES:
    probs = check_alg("fixture", moves, cycle, kind)
    if probs:
        fail += 1
        print("FIXTURE_FAIL %s %s: %s" % (kind, cycle, "; ".join(probs)))
print("fixture_failures=%d" % fail)

BUFFERS = {0: "A", 1: "C"}
synth_re = re.compile(
    r"SYNTH orbit=(\d+) t1=(\d+) t2=(\d+) setup=(\d+) fallback=(\d+) alg len=(\d+) moves=(.*)")
mv_re = re.compile(r"\[(\d+),(\d+),(\d+),(\d+)\]")

nlines = 0
sfail = 0
fallback_n = 0
setup_n = 0
lengths = {}
for line in open(sys.argv[1], encoding="utf-8"):
    m = synth_re.match(line)
    if not m:
        continue
    nlines += 1
    orbit = int(m.group(1))
    t1 = int(m.group(2))
    t2 = int(m.group(3))
    setup = int(m.group(4)) == 1
    fb = int(m.group(5)) == 1
    length = int(m.group(6))
    moves = [tuple(int(g) for g in mv) for mv in mv_re.findall(m.group(7))]
    if len(moves) != length:
        print("PARSE_LEN_MISMATCH", line.strip())
        sfail += 1
        continue
    fallback_n += fb
    setup_n += setup
    lengths[length] = lengths.get(length, 0) + 1
    buf_letter = BUFFERS[orbit]
    l1 = chr(ord("A") + t1)
    l2 = chr(ord("A") + t2)
    def sticker_of_letter(l, orbit):
        for st in STICKERS:
            if st in LETTER_OF and LETTER_OF[st] == l and ((orbit == 0) == is_corner(st)):
                return st
        raise KeyError(l)
    homes = [sticker_of_letter(x, orbit) for x in (buf_letter, l1, l2)]
    dest = simulate(moves)
    probs = []
    if dest[homes[0]] != homes[1] or dest[homes[1]] != homes[2] or dest[homes[2]] != homes[0]:
        probs.append("cycle wrong: buf->%s t1->%s t2->%s" % (
            sticker_name(dest[homes[0]]), sticker_name(dest[homes[1]]),
            sticker_name(dest[homes[2]])))
    want_pieces = {h[0] for h in homes}
    moved = [h for h in STICKERS if dest[h] != h]
    for h in moved:
        if h[0] not in want_pieces:
            probs.append("extra moved %s" % (sticker_name(h),))
    wantc = 9 if orbit == 0 else 6
    if len(moved) != wantc:
        probs.append("moved %d want %d" % (len(moved), wantc))
    if probs:
        sfail += 1
        print("SYNTH_FAIL orbit=%d t1=%d(%s) t2=%d(%s): %s" % (orbit, t1, l1, t2, l2,
                                                              "; ".join(probs)))

print("synth_lines=%d synth_failures=%d setup=%d fallback=%d" % (nlines, sfail, setup_n,
                                                                 fallback_n))
print("length_histogram=%s" % dict(sorted(lengths.items())))
