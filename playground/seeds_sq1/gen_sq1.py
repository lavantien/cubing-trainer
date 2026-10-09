import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[2]
PACKS = ROOT / "db" / "seeds" / "packs"
HERE = pathlib.Path(__file__).resolve().parent
AT = "2026-10-10"
HEADER = "case_key\talg\tfront_json\tsource_url\tsource_at\n"
CS_URL = "https://cubingapp.com/algorithms/SQ1-Cube-Shape"
CO_URL = "https://www.speedcube.us/blogs/square-1-co-corner-orientation"
SARAH = "https://sarah.cubing.net/square-1/"


def norm(s):
    s = re.sub(r"<!--.*?-->", " ", s, flags=re.S)
    s = re.sub(r"<[^>]+>", " ", s)
    s = s.replace(" ", " ")
    s = s.replace("•", " ")
    return re.sub(r"\s+", " ", s).strip()


def front(obj):
    return json.dumps(obj, separators=(",", ":"), ensure_ascii=True)


def write_tsv(name, url, rows):
    keys = [k for k, _, _ in rows]
    assert len(keys) == len(set(keys)), name
    p = PACKS / name
    with p.open("w", encoding="ascii", newline="\n") as f:
        f.write(HEADER)
        for key, alg, case in rows:
            for field in (key, alg, case, url, AT):
                assert field != "" and "\t" not in field and "\n" not in field
            assert len(key) < 64 and len(alg) < 256 and len(front(json.loads(case))) < 512
            f.write(f"{key}\t{alg}\t{case}\t{url}\t{AT}\n")
    print(name, len(rows))


def gen_cubeshape():
    h = (HERE / "cubingapp_cs.html").read_text(encoding="utf-8")
    cases = re.findall(r'<div class="prose"><h3>(.*?)</h3><p>(.*?)</p></div>', h, re.S)
    rows = []
    for i, (raw_name, blob) in enumerate(cases, 1):
        name = norm(raw_name)
        first = norm(blob.split("</p><p>")[0])
        assert name and first, i
        rows.append((f"cs.{i:03d}", first, front({"case": name})))
    assert len(rows) == 169, len(rows)
    write_tsv("sq1_cubeshape.tsv", CS_URL, rows)


def gen_co():
    h = (HERE / "speedcubeus_square-1-co-corner-orientation.html").read_text(encoding="utf-8")
    found = re.findall(r'class="link">(CO \d+)</a></u></strong>\s*<p><strong>(.*?)</strong></p>', h)
    found = [(n, norm(a)) for n, a in found]
    found = [(n, a) for n, a in found if a]
    assert len(found) == 7, found
    rows = []
    for n, a in sorted(found):
        num = int(n.split()[1])
        rows.append((f"co.{num:02d}", a, front({"case": n})))
    write_tsv("sq1_co.tsv", CO_URL, rows)


def sarah_lis(page, idpat):
    h = (HERE / page).read_text(encoding="utf-8")
    return re.findall(r'<li id="(' + idpat + r')">(.*?)</li>', h, re.S)


def gen_eo():
    rows = []
    for li_id, body in sarah_lis("sarah_sq1_eo.html", r"sq1-eo\d+"):
        m = re.search(r'<span class="alg">(.*?)</span>', body, re.S)
        alg = norm(m.group(1))
        if alg == "Skip!":
            continue
        num = int(li_id.split("eo")[1])
        rows.append((f"eo.{num:02d}", alg, front({"case": li_id})))
    assert len(rows) == 7, len(rows)
    write_tsv("sq1_eo.tsv", SARAH + "eo", rows)


def gen_cp():
    fixed = 0
    rows = []
    for li_id, body in sarah_lis("sarah_sq1_cp.html", r"sq1-cp{1,2}\d+"):
        m = re.search(r'<span class="alg">(.*?)</span>', body, re.S)
        alg = norm(m.group(1))
        if alg == "Skip!":
            continue
        if "))" in alg:
            alg = alg.replace("))", ")")
            fixed += 1
        num = int(re.search(r"\d+$", li_id).group(0))
        key = f"cpp.{num:02d}" if li_id.startswith("sq1-cpp") else f"cp.{num:02d}"
        rows.append((key, alg, front({"case": li_id})))
    assert len(rows) == 16 and fixed == 1, (len(rows), fixed)
    write_tsv("sq1_cp.tsv", SARAH + "cp", rows)


def gen_ep():
    rows = []
    for li_id, body in sarah_lis("sarah_sq1_ep.html", r"sq1-ep\d+"):
        alg_m = re.search(r'<span class="(alg red|alg)">(.*?)</span>', body, re.S)
        name_m = re.search(r'<h3 class="alg-nickname">\s*(.*?)\s*</h3>', body, re.S)
        num_m = re.search(r'<span class="alg-name">(.*?)</span>', body, re.S)
        alg = norm(alg_m.group(2))
        if alg == "Skip!":
            continue
        parity = alg_m.group(1) == "alg red"
        name = norm(name_m.group(1)) if name_m else norm(num_m.group(1))
        obj = {"case": name}
        if parity:
            obj["parity"] = True
        num = int(li_id.split("ep")[1])
        rows.append((f"ep.{num:02d}", alg, front(obj)))
    assert len(rows) == 99, len(rows)
    write_tsv("sq1_ep.tsv", SARAH + "ep", rows)


gen_cubeshape()
gen_co()
gen_eo()
gen_cp()
gen_ep()
