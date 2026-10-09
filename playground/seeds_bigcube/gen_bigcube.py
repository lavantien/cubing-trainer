import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[2]
PACKS = ROOT / "db" / "seeds" / "packs"
HERE = pathlib.Path(__file__).resolve().parent
AT = "2026-10-10"
HEADER = "case_key\talg\tfront_json\tsource_url\tsource_at\n"
OLL_URL = "https://www.speedcube.us/blogs/speedcubing-solutions/4x4-oll-parity-algorithms"
PLL_URL = "https://www.speedcube.us/blogs/speedcubing-solutions/4x4-pll-parity-algorithms"
WING_URL = "https://www.speedcube.us/blogs/speedcubing-solutions/5x5-edge-parity-solution-algorithm"

ROWS = [
    ("parity.oll4", "Rw U2, X, Rw U2, Rw U2 , Rw' U2, Lw U2, Rw' U2, Rw U2, Rw' U2, Rw'",
     "oll4", OLL_URL),
    ("parity.oll4flip", "r', U2, l, F2, l', F2, r2, U2, r, U2, r', U2, F2, r2, F2",
     "oll4flip", OLL_URL),
    ("parity.pll4", "r2, U2, r2, Uw2, r2, u2", "pll4", PLL_URL),
    ("parity.pll4adj", "(R' U R U' ) r2, U2, r2, Uw2, r2, u2, (U R' U' R )", "pll4adj", PLL_URL),
    ("parity.wing5", "Rw U2 x Rw U2 Rw U2 Rw' U2 Lw U2 3Rw' U2 Rw U2 Rw' U2 Rw'",
     "wing5", WING_URL),
]


def archive_text(name):
    h = (HERE / name).read_text(encoding="utf-8", errors="replace")
    h = re.sub(r"<[^>]+>", " ", h)
    h = h.replace(" ", " ")
    return re.sub(r"\s+", " ", h)


def main():
    refs = {
        OLL_URL: archive_text("speedcubeus_4x4_oll_parity.html"),
        PLL_URL: archive_text("speedcubeus_4x4_pll_parity.html"),
        WING_URL: archive_text("speedcubeus_5x5_edge_parity.html"),
    }
    for _, alg, _, url in ROWS:
        assert alg in refs[url], alg
    p = PACKS / "bigcube_parity.tsv"
    with p.open("w", encoding="ascii", newline="\n") as f:
        f.write(HEADER)
        for key, alg, case, url in ROWS:
            fj = json.dumps({"case": case}, separators=(",", ":"))
            for field in (key, alg, fj, url, AT):
                assert field != "" and "\t" not in field and "\n" not in field
            f.write(f"{key}\t{alg}\t{fj}\t{url}\t{AT}\n")
    print("bigcube_parity.tsv", len(ROWS))


main()
