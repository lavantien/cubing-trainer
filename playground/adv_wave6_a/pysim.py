import json
import math
import re
import sys

FACE_OF = {(0, 1, 0): "U", (0, -1, 0): "D", (1, 0, 0): "R", (-1, 0, 0): "L", (0, 0, 1): "F",
           (0, 0, -1): "B"}
ROT = {
    ("x", 1): lambda p: (p[0], -p[2], p[1]),
    ("x", -1): lambda p: (p[0], p[2], -p[1]),
    ("y", 1): lambda p: (p[2], p[1], -p[0]),
    ("y", -1): lambda p: (-p[2], p[1], p[0]),
    ("z", 1): lambda p: (-p[1], p[0], p[2]),
    ("z", -1): lambda p: (p[1], -p[0], p[2]),
}


def axis_layers(face):
    wide = face.islower()
    if face in "Rr":
        return "x", [1] + ([0] if wide else []), -1
    if face in "Ll":
        return "x", [-1] + ([0] if wide else []), 1
    if face in "Uu":
        return "y", [1] + ([0] if wide else []), -1
    if face in "Dd":
        return "y", [-1] + ([0] if wide else []), 1
    if face in "Ff":
        return "z", [1] + ([0] if wide else []), -1
    if face in "Bb":
        return "z", [-1] + ([0] if wide else []), 1
    if face == "M":
        return "x", [0], 1
    if face == "E":
        return "y", [0], 1
    if face == "S":
        return "z", [0], -1
    if face == "x":
        return "x", [-1, 0, 1], -1
    if face == "y":
        return "y", [-1, 0, 1], -1
    if face == "z":
        return "z", [-1, 0, 1], -1
    raise ValueError(face)


def move(state, token):
    m = re.fullmatch(r"([URLFBDurlfbdMESxyz])(2)?(')?", token)
    if not m:
        raise ValueError(f"bad token {token!r}")
    face, double, prime = m.group(1), m.group(2) is not None, m.group(3) is not None
    axis_name, layers, quarter = axis_layers(face)
    turns = 2 if double else (-1 if prime else 1)
    base = ROT[(axis_name, quarter)]

    def rot(p):
        q = p
        for _ in range(abs(turns)):
            q = base(q) if turns > 0 else ROT[(axis_name, -quarter)](q)
        return q

    ai = {"x": 0, "y": 1, "z": 2}[axis_name]
    out = {}
    for (pos, nor), color in state.items():
        if pos[ai] in layers:
            out[(rot(pos), rot(nor))] = color
        else:
            out[(pos, nor)] = color
    return out


def solved_state():
    state = {}
    for x in (-1, 0, 1):
        for y in (-1, 0, 1):
            for z in (-1, 0, 1):
                pos = (x, y, z)
                if pos == (0, 0, 0):
                    continue
                for n in FACE_OF:
                    if all(p == c for p, c in zip(pos, n) if c != 0):
                        state[(pos, n)] = FACE_OF[n]
    return state


def apply_alg(state, alg):
    for token in alg.split():
        state = move(state, token)
    return state


def slot_colors(state, pos):
    return {n: c for (p, n), c in state.items() if p == pos}


def all_corners():
    return [(x, y, z) for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)]


def all_edges():
    out = []
    for a in range(3):
        for b in range(a + 1, 3):
            for va in (-1, 1):
                for vb in (-1, 1):
                    pos = [0, 0, 0]
                    pos[a], pos[b] = va, vb
                    out.append(tuple(pos))
    return out


def rodrigues(axis, v, deg):
    n = math.sqrt(sum(a * a for a in axis))
    u = tuple(a / n for a in axis)
    t = math.radians(deg)
    c, s = math.cos(t), math.sin(t)
    cx = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    k = u[0] * v[0] + u[1] * v[1] + u[2] * v[2]
    return tuple(round(a * c + s * b + k * (1 - c) * g) for a, b, g in zip(v, cx, u))


def corner_twist(pos, colors):
    ud_color = "U" if "U" in colors.values() else "D"
    ud_slot = (0, 1, 0) if pos[1] == 1 else (0, -1, 0)
    facing = next(n for n, c in colors.items() if c == ud_color)
    if facing == ud_slot:
        return 0
    outward = pos
    ccw_slot = rodrigues(outward, ud_slot, 120)
    if facing == ccw_slot:
        return 1
    return -1


def edge_flip(pos, colors):
    has_ud = any(c in "UD" for c in colors.values())
    for n, c in colors.items():
        if has_ud and c in "UD":
            return 0 if abs(n[1]) == 1 else 1
        if not has_ud and c in "FB":
            return 0 if abs(n[2]) == 1 else 1
    raise ValueError(f"flip unreadable {pos} {colors}")


SOLVED = solved_state()
CORNER_HOME = {"".join(sorted(slot_colors(SOLVED, p).values())): p for p in all_corners()}
EDGE_HOME = {"".join(sorted(slot_colors(SOLVED, p).values())): p for p in all_edges()}


def diff(state):
    d = {"corner_perm": {}, "corner_twist": {}, "edge_perm": {}, "edge_flip": {}, "centers": []}
    for pos in all_corners():
        colors = slot_colors(state, pos)
        name = "".join(sorted(colors.values()))
        home = CORNER_HOME[name]
        if pos != home:
            d["corner_perm"][name] = (home, pos)
        t = corner_twist(pos, colors)
        if t != 0:
            d["corner_twist"][name] = t
    for pos in all_edges():
        colors = slot_colors(state, pos)
        name = "".join(sorted(colors.values()))
        home = EDGE_HOME[name]
        if pos != home:
            d["edge_perm"][name] = (home, pos)
        f = edge_flip(pos, colors)
        if f != 0:
            d["edge_flip"][name] = f
    for axis in range(3):
        for v in (-1, 1):
            pos = tuple(v if i == axis else 0 for i in range(3))
            n = pos
            if state[(pos, n)] != FACE_OF[n]:
                d["centers"].append((pos, state[(pos, n)]))
    return d


def names_of(pos_text):
    edges = re.findall(r"\b([URLFBD]{2})\s*&\s*([URLFBD]{2})\b", pos_text)
    edges += re.findall(r"\b([URLFBD]{3})\s*&\s*([URLFBD]{3})\b", pos_text)
    epairs = re.findall(r"\b([URLFBD]{2})-([URLFBD]{2})\b", pos_text)
    cpairs = re.findall(r"\b([URLFBD]{3})-([URLFBD]{3})(?:\[([URLFBD]{3})\])?", pos_text)
    return edges, epairs, cpairs


def cname(name):
    return "".join(sorted(name))


def check_row(key, alg, pos_text):
    d = diff(apply_alg(SOLVED, alg))
    problems = []
    cat = key.split(".")[0]
    amp, epairs, cpairs = names_of(pos_text)
    if cat == "flip":
        want = [cname(n) for pair in amp for n in pair]
        for name in want:
            if d["edge_perm"] or d["corner_perm"] or d["corner_twist"] or d["centers"]:
                break
            if d["edge_flip"].get(name) != 1:
                problems.append(f"{name} not flipped")
        extra = [n for n in d["edge_flip"] if n not in want]
        if extra:
            problems.append(f"extra flips {extra}")
    elif cat == "twist":
        want = [cname(n) for pair in amp for n in pair]
        for name in want:
            if name not in d["corner_twist"]:
                problems.append(f"{name} not twisted")
        extra = [n for n in d["corner_twist"] if n not in want]
        if extra:
            problems.append(f"extra twists {extra}")
        if len(d["corner_perm"]) or len(d["edge_perm"]) or len(d["edge_flip"]) or d["centers"]:
            problems.append("unexpected permutation or flip")
        tw = [d["corner_twist"].get(n) for n in want]
        if len(want) == 2 and None not in tw and tw[0] != -tw[1]:
            problems.append(f"twist signs {tw} not opposite")
    elif cat == "parity":
        eperm = {}
        for a, b in epairs:
            eperm[cname(a)] = (EDGE_HOME[cname(a)], EDGE_HOME[cname(b)])
            eperm[cname(b)] = (EDGE_HOME[cname(b)], EDGE_HOME[cname(a)])
        cperm = {}
        for a, b, _ in cpairs:
            cperm[cname(a)] = (CORNER_HOME[cname(a)], CORNER_HOME[cname(b)])
            cperm[cname(b)] = (CORNER_HOME[cname(b)], CORNER_HOME[cname(a)])
        for name in eperm:
            if d["edge_perm"].get(name) != eperm[name]:
                problems.append(f"edge {name}: got {d['edge_perm'].get(name)} want {eperm[name]}")
        for name in cperm:
            if d["corner_perm"].get(name) != cperm[name]:
                problems.append(f"corner {name}: got {d['corner_perm'].get(name)} want {cperm[name]}")
        if d["centers"]:
            problems.append(f"center defects {d['centers']}")
        if any(n not in eperm for n in d["edge_flip"]):
            problems.append(f"flip outside swapped pair {d['edge_flip']}")
        if len(d["edge_perm"]) != len(eperm) or len(d["corner_perm"]) != len(cperm):
            problems.append(f"perm counts edges {len(d['edge_perm'])}/{len(eperm)}"
                            f" corners {len(d['corner_perm'])}/{len(cperm)}")
        if any(n not in cperm for n in d["corner_twist"]):
            problems.append(f"twist outside swapped pair {d['corner_twist']}")
    elif cat == "ltct":
        eperm = {cname(a): (EDGE_HOME[cname(a)], EDGE_HOME[cname(b)]) for a, b in epairs}
        for name in eperm:
            if d["edge_perm"].get(name) != eperm[name]:
                problems.append(f"edge {name}: got {d['edge_perm'].get(name)} want {eperm[name]}")
        if d["edge_flip"] or d["centers"]:
            problems.append(f"edge/center defects {d['edge_flip']} {d['centers']}")
        if len(d["edge_perm"]) != 2 * len(epairs):
            problems.append(f"edge perm count {len(d['edge_perm'])}")
        if any(n not in {x for pair in epairs for x in (cname(pair[0]), cname(pair[1]))}
               for n in d["edge_flip"]):
            problems.append(f"flip outside swapped pair {d['edge_flip']}")
        bracket = cpairs[0][2] if cpairs and cpairs[0][2] else None
        twists = d["corner_twist"]
        if bracket and twists.get(cname(bracket), 0) == 0:
            problems.append(f"bracket {bracket} not twisted")
        total = sum(twists.values()) % 3
        if total != 0:
            problems.append(f"twist sum {total}")
        cperm_names = {cname(cpairs[0][0]), cname(cpairs[0][1])} if cpairs else set()
        for name in d["corner_perm"]:
            if name not in cperm_names:
                problems.append(f"unexpected corner perm {name}")
        if len(d["corner_perm"]) != len(cperm_names):
            problems.append(f"corner perm count {len(d['corner_perm'])}")
        allowed_twisters = cperm_names | ({cname(bracket)} if bracket else set())
        if any(n not in allowed_twisters for n in twists):
            problems.append(f"twist outside pair or bracket {twists}")
    return problems, d


def self_test():
    s = SOLVED
    for seq, times in [("R U", 105), ("R U R' U'", 6), ("U", 4), ("M", 4), ("E", 4), ("S", 4),
                       ("x", 4), ("y", 4), ("z", 4), ("l", 4), ("f", 4), ("r", 4),
                       ("L F R B U D", 12)]:
        st = s
        for _ in range(times):
            st = apply_alg(st, seq)
        if diff(st) != {"corner_perm": {}, "corner_twist": {}, "edge_perm": {}, "edge_flip": {},
                        "centers": []}:
            print(f"SELFTEST FAIL ({seq}) x{times}")
            return False
    st = apply_alg(SOLVED, "R U R' U' R' F R2 U' R' U' R U R' F'")
    d = diff(st)
    want_e = {cname("UL"): (EDGE_HOME[cname("UL")], EDGE_HOME[cname("UR")]),
              cname("UR"): (EDGE_HOME[cname("UR")], EDGE_HOME[cname("UL")])}
    want_c = {cname("UFR"): (CORNER_HOME[cname("UFR")], CORNER_HOME[cname("UBR")]),
              cname("UBR"): (CORNER_HOME[cname("UBR")], CORNER_HOME[cname("UFR")])}
    if d["edge_perm"] != want_e or d["corner_perm"] != want_c or d["edge_flip"] or d["corner_twist"] or d["centers"]:
        print(f"SELFTEST FAIL tperm {d}")
        return False
    print("selftest ok")
    return True


def main():
    if not self_test():
        sys.exit(2)
    with open(sys.argv[1], encoding="utf-8") as f:
        rows = [line.rstrip("\n").split("\t") for line in f.readlines()[1:] if line.strip()]
    failures = 0
    first_twists = {}
    for cells in rows:
        key, alg = cells[0], cells[1]
        fj = json.loads(cells[2])
        problems, d = check_row(key, alg, fj["position"])
        cat = key.split(".")[0]
        if cat == "twist":
            amp, _, _ = names_of(fj["position"])
            first_twists[key] = (cname(amp[0][0]), d["corner_twist"].get(cname(amp[0][0])),
                                 d["corner_twist"].get(cname(amp[0][1])))
        if problems:
            failures += 1
            print(f"FAIL {key} [{fj['position']}] {'; '.join(problems)}")
        else:
            extra = ""
            if cat == "twist":
                t = first_twists[key]
                extra = f" first {t[0]} {t[1]:+d} second {t[2]:+d}"
            if cat == "ltct":
                extra = f" twists {d['corner_twist']}"
            print(f"OK {key}{extra}")
    print(f"failures {failures} of {len(rows)}")
    signs = {t[1] for t in first_twists.values()}
    print(f"twist first-token signs across rows: {signs}")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
