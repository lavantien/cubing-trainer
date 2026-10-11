import json
import re
import sys
import zipfile
import xml.etree.ElementTree as ET

NS_MAIN = "{http://schemas.openxmlformats.org/spreadsheetml/2006/main}"
NS_REL = "{http://schemas.openxmlformats.org/officeDocument/2006/relationships}"
NS_PKG_REL = "{http://schemas.openxmlformats.org/package/2006/relationships}"


def col_index(ref):
    m = re.match(r"([A-Z]+)", ref)
    n = 0
    for ch in m.group(1):
        n = n * 26 + (ord(ch) - 64)
    return n - 1


def read_shared(z):
    try:
        data = z.read("xl/sharedStrings.xml")
    except KeyError:
        return []
    root = ET.fromstring(data)
    out = []
    for si in root.findall(f"{NS_MAIN}si"):
        parts = []
        for t in si.iter(f"{NS_MAIN}t"):
            parts.append(t.text or "")
        out.append("".join(parts))
    return out


def read_sheets(z):
    wb = ET.fromstring(z.read("xl/workbook.xml"))
    rels = ET.fromstring(z.read("xl/_rels/workbook.xml.rels"))
    rel_map = {r.get("Id"): r.get("Target") for r in rels.findall(f"{NS_PKG_REL}Relationship")}
    sheets = []
    for sh in wb.iter(f"{NS_MAIN}sheet"):
        rid = sh.get(f"{NS_REL}id")
        target = rel_map[rid]
        if target.startswith("/"):
            path = target.lstrip("/")
        else:
            path = "xl/" + target
        sheets.append((sh.get("name"), path))
    return sheets


def read_sheet_rows(z, path, shared):
    root = ET.fromstring(z.read(path))
    rows = []
    for row in root.iter(f"{NS_MAIN}row"):
        cells = {}
        for c in row.findall(f"{NS_MAIN}c"):
            ref = c.get("r")
            idx = col_index(ref)
            typ = c.get("t")
            if typ == "inlineStr":
                is_el = c.find(f"{NS_MAIN}is")
                val = "".join(t.text or "" for t in is_el.iter(f"{NS_MAIN}t")) if is_el is not None else ""
            else:
                v = c.find(f"{NS_MAIN}v")
                val = v.text if v is not None else ""
                if typ == "s" and val != "":
                    val = shared[int(val)]
            cells[idx] = val
        width = max(cells) + 1 if cells else 0
        rows.append([cells.get(i, "") for i in range(width)])
    return rows


def main(path, out_prefix):
    z = zipfile.ZipFile(path)
    shared = read_shared(z)
    for name, sheet_path in read_sheets(z):
        rows = read_sheet_rows(z, sheet_path, shared)
        safe = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_")
        out = f"{out_prefix}_{safe}.tsv"
        with open(out, "w", encoding="utf-8", newline="") as f:
            for row in rows:
                line = "\t".join(cell.replace("\t", " ").replace("\r", " ").replace("\n", " ") for cell in row)
                f.write(line + "\n")
        print(f"{name}: {len(rows)} rows -> {out}")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
