import json
import sys


def load_sheet(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        header = f.readline().rstrip("\n").split("\t")
        for line in f:
            line = line.rstrip("\n")
            if not line.strip():
                continue
            cells = line.split("\t")
            while len(cells) < 5:
                cells.append("")
            rows.append({"position": cells[0].strip(), "alg": cells[1].strip(), "comm": cells[2].strip(),
                         "url": cells[3].strip()})
    return header, rows


def load_seed(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        header = f.readline().rstrip("\n").split("\t")
        for line in f:
            line = line.rstrip("\n")
            if not line.strip():
                continue
            cells = line.split("\t")
            fj = json.loads(cells[2])
            rows.append({"key": cells[0], "alg": cells[1], "position": fj["position"],
                         "solver": fj["solver"], "comm": fj["comm"], "url": cells[3], "at": cells[4]})
    return header, rows


def main():
    cats = ["flip", "twist", "parity", "ltct"]
    stem = {"flip": "flips", "twist": "twists", "parity": "parity", "ltct": "ltct"}
    sheets = {}
    for cat in cats:
        sheets[("cherry", cat)] = load_sheet(f"cherry_{stem[cat]}.tsv")[1]
        sheets[("eggins", cat)] = load_sheet(f"eggins_{stem[cat]}.tsv")[1]
    _, seed = load_seed("../../db/seeds/packs/bld_3style_specials.tsv")
    problems = []
    pool = []
    for (solver, cat), rows in sheets.items():
        for r in rows:
            pool.append({**r, "solver": solver, "cat": cat})
    used = set()
    for s in seed:
        cat = s["key"].split(".")[0]
        matches = [i for i, p in enumerate(pool)
                   if i not in used and p["cat"] == cat and p["position"] == s["position"]
                   and p["alg"] == s["alg"] and p["comm"] == s["comm"] and p["url"] == s["url"]]
        if not matches:
            loose = [i for i, p in enumerate(pool) if i not in used and p["cat"] == cat
                     and p["position"] == s["position"]]
            detail = f"no exact sheet match; {len(loose)} position-only candidates"
            if loose:
                p = pool[loose[0]]
                for field in ("alg", "comm", "url"):
                    if p[field] != s[field]:
                        detail += f"; {field} seed={s[field]!r} sheet={p[field]!r}"
            problems.append(f"{s['key']}: {detail}")
            continue
        i = matches[0]
        p = pool[i]
        used.add(i)
        if p["solver"] != s["solver"]:
            problems.append(f"{s['key']}: solver seed={s['solver']} sheet={p['solver']}")
        if s["at"] != "2026-10-10":
            problems.append(f"{s['key']}: source_at {s['at']}")
    leftovers = [pool[i] for i in range(len(pool)) if i not in used]
    print(f"seed rows {len(seed)} pool rows {len(pool)} matched {len(seed) - len([p for p in problems])}"
          f" leftover pool {len(leftovers)}")
    for p in leftovers:
        print(f"LEFTOVER {p['solver']} {p['cat']} {p['position']} | {p['alg']} | {p['url']}")
    for line in problems:
        print(f"MISMATCH {line}")
    by_cat = {}
    pos_seen = {}
    for s in seed:
        cat = s["key"].split(".")[0]
        by_cat[cat] = by_cat.get(cat, 0) + 1
        tag = (cat, s["position"])
        if tag in pos_seen:
            print(f"DUPLICATE POSITION {s['key']} vs {pos_seen[tag]} {s['position']}")
        pos_seen[tag] = s["key"]
    print("counts", by_cat)
    keys = [s["key"] for s in seed]
    for cat in cats:
        n = by_cat.get(cat, 0)
        expect = [f"{cat}.{i:02d}" for i in range(1, n + 1)]
        actual = [k for k in keys if k.startswith(cat + ".")]
        if expect != actual:
            print(f"KEY ORDER {cat}: expected {expect[:3]}.. got {actual[:3]}..")
    solver_seq = [(s["key"].split(".")[0], s["solver"]) for s in seed]
    for cat in cats:
        seq = [sv for c, sv in solver_seq if c == cat]
        if "eggins" in seq:
            first = seq.index("eggins")
            if "cherry" in seq[first:]:
                print(f"SOLVER ORDER {cat}: cherry after eggins at index {first}")


if __name__ == "__main__":
    main()
