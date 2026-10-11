import os
import sys

SHEETS = ["edge", "corner", "flips", "twists", "parity", "ltct"]


def main(root):
    outdir = os.path.join(root, "playground", "ref_ground")
    out = open(os.path.join(outdir, "rows.tsv"), "w", encoding="utf-8", newline="\n")
    out.write("solver\tsheet\trowid\tposition\talg\n")
    for solver in ("cherry", "eggins"):
        for sheet in SHEETS:
            path = os.path.join(outdir, solver, sheet + ".tsv")
            if not os.path.exists(path):
                continue
            with open(path, encoding="utf-8") as f:
                rows = f.read().split("\n")
            if rows and rows[-1] == "":
                rows.pop()
            for idx, row in enumerate(rows[1:], start=2):
                cells = row.split("\t")
                if len(cells) < 2 or cells[0] == "" or cells[1] == "":
                    continue
                out.write("%s\t%s\t%d\t%s\t%s\n" % (solver, sheet, idx, cells[0], cells[1]))
    out.close()


if __name__ == "__main__":
    main(sys.argv[1])
