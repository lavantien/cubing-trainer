import os
import sys
import zipfile
import xml.etree.ElementTree as ET

NS = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main",
      "r": "http://schemas.openxmlformats.org/officeDocument/2006/relationships"}
RELNS = {"pr": "http://schemas.openxmlformats.org/package/2006/relationships"}

FILES = [
    ("cherry", os.path.join("ref", "Tommy Cherry's 3BLD algs.xlsx")),
    ("eggins", os.path.join("ref", "Charlie Eggins's 3BLD algs.xlsx")),
]


def main(root):
    for tag, rel in FILES:
        z = zipfile.ZipFile(os.path.join(root, rel))
        wb = ET.fromstring(z.read("xl/workbook.xml"))
        rels = ET.fromstring(z.read("xl/_rels/workbook.xml.rels"))
        rid = {r.get("Id"): r.get("Target") for r in rels.findall("pr:Relationship", RELNS)}
        out = os.path.join(root, "playground", "ref_ground", tag, "hyperlinks.tsv")
        count = 0
        with open(out, "w", encoding="utf-8", newline="") as f:
            f.write("sheet\tcell\tdisplay\ttarget\n")
            for sh in wb.find("m:sheets", NS).findall("m:sheet", NS):
                t = rid[sh.get("{%s}id" % NS["r"])]
                if not t.startswith("/"):
                    t = "xl/" + t
                root_el = ET.fromstring(z.read(t))
                hls = root_el.find("m:hyperlinks", NS)
                if hls is None:
                    continue
                rpath = t.replace("worksheets/", "worksheets/_rels/") + ".rels"
                srel = ET.fromstring(z.read(rpath))
                srid = {r.get("Id"): r.get("Target") for r in srel.findall("pr:Relationship", RELNS)}
                for h in hls.findall("m:hyperlink", NS):
                    hid = h.get("{%s}id" % NS["r"])
                    f.write("%s\t%s\t%s\t%s\n" % (sh.get("name"), h.get("ref"), h.get("display") or "",
                                                   srid.get(hid, "")))
                    count += 1
        print(tag, "hyperlinks", count)


if __name__ == "__main__":
    main(sys.argv[1])
