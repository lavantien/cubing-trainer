import hashlib
import os
import sys
import zipfile
import xml.etree.ElementTree as ET

NS = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main",
      "r": "http://schemas.openxmlformats.org/officeDocument/2006/relationships"}
RELNS = {"pr": "http://schemas.openxmlformats.org/package/2006/relationships"}

FILES = [
    ("cherry", os.path.join("ref", "Tommy Cherry's 3BLD algs.xlsx"),
     "DE60EFB66F974C123A84004E214D691BA2762B71718A9BF2F764BCA0B33C466A"),
    ("eggins", os.path.join("ref", "Charlie Eggins's 3BLD algs.xlsx"),
     "3C1BECF9C4ABA50C1945D8513E78F34314B7FDAE767112847ABEDB9F57AA9B68"),
]

EXPECT_COLS = 5


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def col_index(ref):
    n = 0
    for ch in ref:
        if ch.isalpha():
            n = n * 26 + (ord(ch.upper()) - 64)
        else:
            break
    return n


def load_shared(z):
    try:
        data = z.read("xl/sharedStrings.xml")
    except KeyError:
        return []
    root = ET.fromstring(data)
    out = []
    for si in root.findall("m:si", NS):
        out.append("".join(t.text or "" for t in si.iter("{%s}t" % NS["m"])))
    return out


def sheet_paths(z):
    wb = ET.fromstring(z.read("xl/workbook.xml"))
    rels = ET.fromstring(z.read("xl/_rels/workbook.xml.rels"))
    rid_to_target = {rel.get("Id"): rel.get("Target")
                     for rel in rels.findall("pr:Relationship", RELNS)}
    out = []
    for sh in wb.find("m:sheets", NS).findall("m:sheet", NS):
        rid = sh.get("{%s}id" % NS["r"])
        target = rid_to_target.get(rid, "")
        if not target.startswith("/"):
            target = "xl/" + target
        out.append((sh.get("name"), target))
    return out


def read_rows(z, path, shared):
    root = ET.fromstring(z.read(path))
    rows = []
    for row in root.iter("{%s}row" % NS["m"]):
        cells = {}
        for c in row.findall("m:c", NS):
            ref = c.get("r") or ""
            t = c.get("t")
            v = c.find("m:v", NS)
            if t == "inlineStr":
                is_el = c.find("m:is", NS)
                text = "".join(x.text or "" for x in is_el.iter("{%s}t" % NS["m"])) if is_el is not None else ""
            elif v is None:
                text = ""
            elif t == "s":
                text = shared[int(v.text)]
            else:
                text = v.text
            cells[col_index(ref)] = text
        width = max(cells) if cells else 0
        width = max(width, EXPECT_COLS)
        rows.append([cells.get(i, "") for i in range(1, width + 1)])
    return rows


def main(root):
    problems = []
    for tag, rel, want in FILES:
        path = os.path.join(root, rel)
        got = sha256(path)
        status = "OK" if got == want else "MISMATCH"
        print("%s sha256 %s want %s" % (rel, got, status))
        if status != "OK":
            problems.append(rel)
        z = zipfile.ZipFile(path)
        shared = load_shared(z)
        outdir = os.path.join(root, "playground", "ref_ground", tag)
        os.makedirs(outdir, exist_ok=True)
        for name, target in sheet_paths(z):
            rows = read_rows(z, target, shared)
            safe = name.replace(" ", "_")
            out = os.path.join(outdir, safe + ".tsv")
            with open(out, "w", encoding="utf-8", newline="") as f:
                for r in rows:
                    for j, cell in enumerate(r):
                        if "\t" in cell or "\n" in cell or "\r" in cell or '"' in cell:
                            problems.append("%s/%s row contains special char %r" % (tag, name, cell))
                        if j:
                            f.write("\t")
                        f.write(cell)
                    f.write("\n")
            nonempty = sum(1 for r in rows if any(x != "" for x in r))
            print("%s sheet %r rows %d nonempty %d" % (tag, name, len(rows), nonempty))
    if problems:
        print("PROBLEMS:")
        for p in problems:
            print(" ", p)
    else:
        print("no problems")


if __name__ == "__main__":
    main(sys.argv[1])
