import sys

PATH = "tests/mutant_allowlist.txt"

DROPS = {
    "src/ui/screen_review.c:253",
    "src/ui/screen_review.c:262",
    "src/ui/screen_review.c:280",
    "src/ui/screen_review.c:281",
    "src/ui/screen_review.c:282",
    "src/ui/screen_review.c:283",
    "src/ui/screen_review.c:284",
    "src/ui/screen_review.c:285",
    "src/ui/screen_review.c:289",
    "src/ui/screen_review.c:295",
    "src/ui/screen_review.c:296",
    "src/ui/screen_review.c:297",
}

REWORDS = {
    "src/ui/screen_trainer.c:18": "decomp step spacing arithmetic, consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:20": "memo decomp reserve arithmetic, consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:57": "saved memo clear, the field is fully overwritten by the memo row load before the only draw-side read",
    "src/ui/screen_trainer.c:58": "saved memo clear, the field is fully overwritten by the memo row load before the only draw-side read",
    "src/ui/screen_review.c:307": "scheme face loop bound, write-only scheme state read only by the draw path",
}

rows = [l.rstrip("\n") for l in open(PATH, encoding="utf-8") if l.strip() and not l.startswith("#")]
keys = {r.split("\t")[0] for r in rows}
missing = (DROPS | set(REWORDS)) - keys
if missing:
    sys.exit(f"targets missing: {sorted(missing)}")

out = []
for r in rows:
    key = r.split("\t")[0]
    if key in DROPS:
        continue
    if key in REWORDS:
        out.append(key + "\t" + REWORDS[key])
    else:
        out.append(r)

with open(PATH, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out) + "\n")

final_keys = [r.split("\t")[0] for r in out]
if len(set(final_keys)) != len(final_keys):
    sys.exit("duplicate keys after edit")
print(f"rows before {len(rows)} after {len(out)} dropped {len(DROPS)} reworded {len(REWORDS)}")
