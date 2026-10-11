import csv
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
REF = ROOT / "playground" / "ref_ground"
OUT = ROOT / "playground" / "parity_flip_twist" / "draft"
SOURCE_AT = "2026-10-10"
CATEGORIES = ("flips", "twists", "parity", "ltct")
PREFIX = {"flips": "flip", "twists": "twist", "parity": "parity", "ltct": "ltct"}
SOLVERS = ("cherry", "eggins")


def read_sheet(solver, sheet):
    path = REF / solver / f"{sheet}.tsv"
    rows = []
    with path.open(encoding="utf-8", newline="") as f:
        reader = csv.reader(f, delimiter="\t")
        header = next(reader)
        cols = [h.strip() for h in header]
        pos_i = next(i for i, h in enumerate(cols) if h.startswith("Position"))
        alg_i = cols.index("Alg")
        comm_i = cols.index("Commutator")
        link_i = cols.index("Reconstruction Link")
        for raw in reader:
            if not raw or all(cell.strip() == "" for cell in raw):
                continue
            rows.append(
                {
                    "position": raw[pos_i].strip(),
                    "alg": raw[alg_i].strip(),
                    "comm": raw[comm_i].strip(),
                    "link": raw[link_i].strip(),
                }
            )
    return rows


def main():
    out_rows = []
    merge = []
    for sheet in CATEGORIES:
        seen = {}
        picked = []
        dropped = []
        for solver in SOLVERS:
            for row in read_sheet(solver, sheet):
                if row["position"] in seen:
                    dropped.append((solver, row["position"], seen[row["position"]]))
                    continue
                seen[row["position"]] = solver
                row["solver"] = solver
                picked.append(row)
        for idx, row in enumerate(picked, start=1):
            front = {
                "position": row["position"],
                "solver": row["solver"],
                "comm": row["comm"],
            }
            out_rows.append(
                (
                    f"{PREFIX[sheet]}.{idx:02d}",
                    row["alg"],
                    json.dumps(front, ensure_ascii=False, separators=(",", ":")),
                    row["link"],
                    SOURCE_AT,
                )
            )
        merge.append((sheet, len(picked), dropped))
    OUT.mkdir(parents=True, exist_ok=True)
    target = OUT / "bld_3style_specials.tsv"
    with target.open("w", encoding="utf-8", newline="") as f:
        f.write("case_key\talg\tfront_json\tsource_url\tsource_at\n")
        for row in out_rows:
            f.write("\t".join(row) + "\n")
    print(f"wrote {target} rows={len(out_rows)}")
    keys = set()
    for row in out_rows:
        if row[0] in keys:
            print(f"DUP KEY {row[0]}")
            return 1
        keys.add(row[0])
        bad = any("\t" in cell or "\n" in cell or "\r" in cell for cell in row[:2]) or any(
            "\t" in cell or "\n" in cell or "\r" in cell for cell in row[3:]
        )
        if bad:
            print(f"BAD CHARS {row[0]}")
            return 1
    for sheet, count, dropped in merge:
        print(f"{sheet}: kept={count} dropped={len(dropped)}")
        for solver, position, kept_by in dropped:
            print(f"  dropped {solver} {position} kept {kept_by}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
