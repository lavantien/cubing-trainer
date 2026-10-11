import sys
import zipfile
import xml.etree.ElementTree as ET

NS = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main",
      "r": "http://schemas.openxmlformats.org/officeDocument/2006/relationships"}
RELNS = {"pr": "http://schemas.openxmlformats.org/package/2006/relationships"}


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
        out.append((sh.get("name"), sh.get("sheetId"), target))
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
        if cells:
            width = max(cells)
            rows.append([cells.get(i, "") for i in range(1, width + 1)])
        else:
            rows.append([])
    return rows


def main(path, head):
    z = zipfile.ZipFile(path)
    shared = load_shared(z)
    print("file:", path)
    print("shared strings:", len(shared))
    for name, sid, target in sheet_paths(z):
        rows = read_rows(z, target, shared)
        nonempty = sum(1 for r in rows if any(c != "" for c in r))
        print()
        print("sheet:", repr(name), "sheetId:", sid, "rows:", len(rows), "nonempty:", nonempty)
        shown = 0
        for i, r in enumerate(rows):
            if not any(c != "" for c in r):
                continue
            print("  row", i + 1, "|", " | ".join(x.replace("\n", "\\n")[:40] for x in r[:8]))
            shown += 1
            if shown >= head:
                print("  ...")
                break


if __name__ == "__main__":
    main(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 12)
