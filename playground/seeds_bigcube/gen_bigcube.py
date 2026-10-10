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
L2E_URL = "https://www.speedcube.us/blogs/5x5-last-two-edges"

PARITY_ROWS = [
    ("parity.oll4", "Rw U2, X, Rw U2, Rw U2 , Rw' U2, Lw U2, Rw' U2, Rw U2, Rw' U2, Rw'",
     "oll4", OLL_URL),
    ("parity.oll4flip", "r', U2, l, F2, l', F2, r2, U2, r, U2, r', U2, F2, r2, F2",
     "oll4flip", OLL_URL),
    ("parity.pll4", "r2, U2, r2, Uw2, r2, u2", "pll4", PLL_URL),
    ("parity.pll4adj", "(R' U R U' ) r2, U2, r2, Uw2, r2, u2, (U R' U' R )", "pll4adj", PLL_URL),
    ("parity.wing5", "Rw U2 x Rw U2 Rw U2 Rw' U2 Lw U2 3Rw' U2 Rw U2 Rw' U2 Rw'",
     "wing5", WING_URL),
]

L2E_ROWS = [
    ("l2e.01", "Rw' Lw U' R' U (R' F R F') Rw Lw'", "l2e01"),
    ("l2e.02", "Rw' U' R' U R' F R F' Rw", "l2e02"),
    ("l2e.03", "3Lw U' R' U (R' F R F') Rw'", "l2e03"),
    ("l2e.04", "Rw2' F2 U2' Rw2' U2' F2 Rw2", "l2e04"),
    ("l2e.05", "Rw U2 x Rw U2 Rw U2' Rw' U2 Lw U2 3Rw' U2' Rw U2 Rw' U2' Rw", "l2e05"),
    ("l2e.06", "Rw U2 Rw U2' x U2 Rw U2' 3Rw' U2 Lw U2' Rw2", "l2e06"),
    ("l2e.07", "F2 Rw U2 Rw U2' Rw' F2 Rw' U2 Rw' U2' Rw U2 Rw' U2' Rw2", "l2e07"),
    ("l2e.08", "B2 Rw' U2 Rw' U2' Rw B2 Rw U2 Rw U2' Rw' U2 Rw U2' Rw2", "l2e08"),
    ("l2e.09", "Rw U2 Rw2 U2' Rw' U2 Rw U2' Rw' U2 Rw2 U2' Rw", "l2e09"),
    ("l2e.10", "Rw' U2' Rw2 U2' Rw U2' Rw' U2 Rw U2' Rw2 U2' Rw'", "l2e10"),
    ("l2e.11", "Rw' U2 Rw U2' 3Lw' U2 Rw U2 Rw U2' Rw' U2 Rw U2' Rw2", "l2e11"),
    ("l2e.12", "Rw2 B2 Rw' U2 Rw' U2' x' U2 Rw' U2' Rw U2 Rw' U2' Rw2", "l2e12"),
]


def archive_text(name):
    h = (HERE / name).read_text(encoding="utf-8", errors="replace")
    h = re.sub(r"<script[^>]*>.*?</script>", " ", h, flags=re.S)
    h = re.sub(r"<style[^>]*>.*?</style>", " ", h, flags=re.S)
    h = re.sub(r"<[^>]+>", " ", h)
    h = h.replace("\xa0", " ")
    return re.sub(r"\s+", " ", h)


def write_pack(path, rows):
    with path.open("w", encoding="ascii", newline="\n") as f:
        f.write(HEADER)
        for row in rows:
            key, alg, case, url = row
            fj = json.dumps({"case": case}, separators=(",", ":"))
            for field in (key, alg, fj, url, AT):
                assert field != "" and "\t" not in field and "\n" not in field
            f.write(f"{key}\t{alg}\t{fj}\t{url}\t{AT}\n")
    print(path.name, len(rows))


def main():
    parity_refs = {
        OLL_URL: archive_text("speedcubeus_4x4_oll_parity.html"),
        PLL_URL: archive_text("speedcubeus_4x4_pll_parity.html"),
        WING_URL: archive_text("speedcubeus_5x5_edge_parity.html"),
    }
    for _, alg, _, url in PARITY_ROWS:
        assert alg in parity_refs[url], alg
    l2e_text = archive_text("speedcubeus_5x5_l2e.html")
    for key, alg, _ in L2E_ROWS:
        assert f"5x5 L2E {key.split('.')[1]}" in l2e_text, key
        assert alg in l2e_text, alg
    write_pack(PACKS / "bigcube_parity.tsv", PARITY_ROWS)
    write_pack(PACKS / "bigcube_l2e.tsv",
               [(key, alg, case, L2E_URL) for key, alg, case in L2E_ROWS])


main()
