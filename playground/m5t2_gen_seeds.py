import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
PACKS = ROOT / "db" / "seeds" / "packs"
CLL_URL = "https://jperm.net/algs/2x2/cll"
L4E_URL = "http://cubing.mikeg.me.uk/cubing-algs/html/l4e.html"
AT = "2026-10-10"
HEADER = "case_key\talg\tfront_json\tsource_url\tsource_at\n"


def front(case):
    return json.dumps({"case": case}, separators=(",", ":"))


def write_tsv(name, url, rows):
    PACKS.mkdir(parents=True, exist_ok=True)
    p = PACKS / name
    with p.open("w", encoding="ascii", newline="\n") as f:
        f.write(HEADER)
        for key, alg, case in rows:
            for field in (key, alg, front(case)):
                assert field != "" and "\t" not in field and "\n" not in field
            f.write(f"{key}\t{alg}\t{front(case)}\t{url}\t{AT}\n")
    print(name, len(rows))


def gen_cll():
    src = (ROOT / "playground" / "m5t2_jperm_cll.js").read_text(encoding="utf-8")
    rows = []
    for m in re.finditer(r'name:"(L\d|U\d|T\d|P\d|H\d|AS\d|S\d|Adjacent|Diagonal)",alg:\["((?:[^"\\]|\\.)*)"', src):
        rows.append((m.group(1), m.group(2)))
    assert len(rows) == 42, len(rows)
    assert len({r[0] for r in rows}) == 42
    return [(f"cll.{i + 1:02d}", alg, name) for i, (name, alg) in enumerate(rows)]


def gen_l4e():
    src = (ROOT / "playground" / "m5t2_mikeg_l4e.js").read_text(encoding="utf-8")
    body = src[src.index("cases:[") :]
    chunks = body.split('{id:"')[1:]
    rows = []
    for chunk in chunks:
        cid = chunk.split('"', 1)[0]
        if not re.fullmatch(r"(?:DS|DF|US|UF|NP)\d+", cid):
            continue
        alg_m = re.search(r'algs:\[\{alg:"((?:[^"\\]|\\.)*)"[^}]*?status:(-?\d+)', chunk)
        if alg_m is None:
            assert cid == "DS1" and "algs:[]" in chunk, cid
            continue
        assert alg_m.group(2) == "1", (cid, alg_m.group(2))
        rows.append((cid, alg_m.group(1)))
    assert len(rows) == 35, len(rows)
    return [(f"l4e.{i + 1:02d}", alg, cid) for i, (cid, alg) in enumerate(rows)]


write_tsv("twoby2_cll.tsv", CLL_URL, gen_cll())
write_tsv("pyraminx_l4e.tsv", L4E_URL, gen_l4e())
