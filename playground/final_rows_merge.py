import sys

PATH = "tests/mutant_allowlist.txt"

DROP = {"src/app/script.c:461"}
ADDS = [
    ("src/ui/screen_review.c:103", "redundant with idle zeroing of memo image at load start"),
    ("src/ui/screen_review.c:108", "memo set validates images non-empty so corner get success implies image present"),
]

rows = [l.rstrip("\n") for l in open(PATH, encoding="utf-8") if l.strip() and not l.startswith("#")]
keys = [r.split("\t")[0] for r in rows]
if len(set(keys)) != len(keys):
    sys.exit("duplicate keys before edit")
for key, _ in ADDS:
    if key in set(keys):
        sys.exit(f"add collision at {key}")
if not DROP <= set(keys):
    sys.exit(f"drop target missing: {DROP - set(keys)}")

out = [r for r in rows if r.split("\t")[0] not in DROP]
out.extend(key + "\t" + mech for key, mech in ADDS)

with open(PATH, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out) + "\n")

final_keys = [r.split("\t")[0] for r in out]
if len(set(final_keys)) != len(final_keys):
    sys.exit("duplicate keys after edit")
print(f"rows before {len(rows)} after {len(out)} dropped {len(rows) - len(out) + len(ADDS)} added {len(ADDS)}")
