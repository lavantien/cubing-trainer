import sys

FACES = {"U": (0, 1, 0), "L": (-1, 0, 0), "F": (0, 0, 1), "R": (1, 0, 0), "B": (0, 0, -1), "D": (0, -1, 0)}
AXIS_OF = {"U": 1, "D": 1, "L": 0, "R": 0, "F": 2, "B": 2}
SIGN_OF = {"U": 1, "D": -1, "L": -1, "R": 1, "F": 1, "B": -1}
PIECES = {"UFR": (1, 1, 1), "UFL": (-1, 1, 1), "UBR": (1, 1, -1), "UBL": (-1, 1, -1),
          "DFR": (1, -1, 1), "DFL": (-1, -1, 1), "DBR": (1, -1, -1), "DBL": (-1, -1, -1)}


def grid(face, r, c):
    if face == "U":
        return (c - 1, 1, r - 1)
    if face == "L":
        return (-1, 1 - r, c - 1)
    if face == "F":
        return (c - 1, 1 - r, 1)
    if face == "R":
        return (1, 1 - r, 1 - c)
    if face == "B":
        return (1 - c, 1 - r, -1)
    return (c - 1, -1, 1 - r)


def cell_of(face, pos):
    for r in range(3):
        for c in range(3):
            if grid(face, r, c) == pos:
                return (r, c)
    return None


def rot(v, axis, quarter):
    x, y, z = v
    for _ in range(quarter):
        if axis == 0:
            x, y, z = x, -z, y
        elif axis == 1:
            x, y, z = z, y, -x
        else:
            x, y, z = -y, x, z
    return (x, y, z)


def simulate(alg):
    stickers = []
    for f, n in FACES.items():
        for r in range(3):
            for c in range(3):
                stickers.append([grid(f, r, c), n, f + str(r) + str(c)])
    for tok in alg.split():
        base = tok[0]
        quarter = 1
        if tok.endswith("2"):
            quarter = 2
        elif tok.endswith("'"):
            quarter = 3
        n = FACES[base]
        axis = AXIS_OF[base]
        sign = SIGN_OF[base]
        applied = quarter if sign < 0 else (4 - quarter) % 4
        for s in stickers:
            if sum(a * b for a, b in zip(s[0], n)) == 1:
                s[0] = rot(s[0], axis, applied)
                s[1] = rot(s[1], axis, applied)
    return stickers


def main(alg, mode):
    stickers = simulate(alg)
    face_of = {v: k for k, v in FACES.items()}
    if mode == "perm":
        for pos, n, label in sorted(stickers, key=lambda s: s[2]):
            dstface = face_of[n]
            rc = cell_of(dstface, pos)
            print(label, "->", dstface + str(rc[0]) + str(rc[1]))
        return
    centers = {}
    for s in stickers:
        centers.setdefault(s[0], []).append(s)
    for name, center in sorted(PIECES.items()):
        group = centers.get(center)
        if not group:
            continue
        mapping = sorted((s[2] + "->" + face_of[s[1]]) for s in group)
        print(name, " ".join(mapping))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "piece")
