import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[2]
PACKS = ROOT / "db" / "seeds" / "packs"
HERE = pathlib.Path(__file__).resolve().parent
AT = "2026-10-10"
HEADER = "case_key\talg\tfront_json\tsource_url\tsource_at\n"
URL = "http://www.jaapsch.net/puzzles/clock.htm"

CROSS = ("Push down the bottom two pins, push up the top two. "
         "Turn a top corner until the centre clock matches the bottom middle.")
ALL12 = "Push up all pins. Turn any corner till the centre clock is 12."
CORNER = ("Push down the bottom left pin, push the other three up. "
          "Turn a top corner till the centre clock matches the bottom left clock.")

ROWS = [
    ("clock.01", CROSS, "front cross 1"),
    ("clock.02", CROSS, "front cross 2"),
    ("clock.03", CROSS, "front cross 3"),
    ("clock.04", CROSS, "front cross 4"),
    ("clock.05", ALL12, "front all to 12"),
    ("clock.06", CROSS, "back cross 1"),
    ("clock.07", CROSS, "back cross 2"),
    ("clock.08", CROSS, "back cross 3"),
    ("clock.09", CROSS, "back cross 4"),
    ("clock.10", CORNER, "back corners 1"),
    ("clock.11", CORNER, "back corners 2"),
    ("clock.12", CORNER, "back corners 3"),
    ("clock.13", CORNER, "back corners 4"),
    ("clock.14", ALL12, "back all to 12"),
]


def archive_text():
    h = (HERE / "jaap_clock.html").read_text(encoding="utf-8", errors="replace")
    h = re.sub(r"<[^>]+>", " ", h)
    h = h.replace(" ", " ")
    return re.sub(r"\s+", " ", h)


def main():
    ref = archive_text()
    for _, alg, _ in ROWS:
        for sentence in alg.split(". "):
            sentence = sentence.strip()
            assert sentence in ref, sentence
    p = PACKS / "clock_algs.tsv"
    with p.open("w", encoding="ascii", newline="\n") as f:
        f.write(HEADER)
        for key, alg, case in ROWS:
            fj = json.dumps({"case": case}, separators=(",", ":"))
            for field in (key, alg, fj, URL, AT):
                assert field != "" and "\t" not in field and "\n" not in field
            f.write(f"{key}\t{alg}\t{fj}\t{URL}\t{AT}\n")
    print("clock_algs.tsv", len(ROWS))


main()
