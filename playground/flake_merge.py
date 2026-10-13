import sys

PATH = "tests/mutant_allowlist.txt"

ADDS = [
    ("src/app/script.c:138", "final-line newline boundary only shifts diagnostic line numbers"),
    ("src/app/script.c:513", "scrub index display value unused by script assertions"),
    ("src/app/script.c:656", "empty target guard, suite scripts always scope a pair first"),
    ("src/app/script.c:661", "repeated synth init is idempotent"),
    ("src/app/script.c:677", "format returns positive length on success, guard flips stay false"),
    ("src/app/script.c:857", "initial rc overwritten by first step, empty script unasserted"),
]

REWORDS = {
    "src/app/script.c:642": "empty and negative case counts reject identically, count zero is excluded by every selector list holding at least one case across cfop stages, chart targets, and seeded packs, a corpus precondition pinned by selector tests",
    "src/ui/screen_trainer.c:199": "corpus precondition, both chart orbits seed the full 576 memo table matching the row cap so n saturates and both arms agree, pinned by seed registry counts",
    "src/ui/screen_cards.c:57": "selected sentinel unobserved, open assigns 0 right after when rows exist and the reload clamp restores -1 for the zero arm when no rows",
    "src/app/shots.c:202": "negative pick index clamps to row zero inside ct_cards_form_select, the minus one arm captures the identical first row, the plus one arm is killed by the two card selected pin in shots_state_guards",
}

rows = []
for raw in open(PATH, encoding="utf-8"):
    line = raw.rstrip("\n")
    if line and not line.startswith("#"):
        rows.append(line)

keys = {r.split("\t")[0]: r for r in rows}
for key, mech in ADDS:
    if key in keys:
        sys.exit(f"add collision at {key}")
for key in REWORDS:
    if key not in keys:
        sys.exit(f"reword target missing: {key}")

out = []
for r in rows:
    key = r.split("\t")[0]
    if key in REWORDS:
        out.append(key + "\t" + REWORDS[key])
    else:
        out.append(r)
out.extend(key + "\t" + mech for key, mech in ADDS)

with open(PATH, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out) + "\n")

final = {r.split("\t")[0] for r in out if r and not r.startswith("#")}
if len(final) != len([r for r in out if r and not r.startswith("#")]):
    sys.exit("duplicate keys after merge")
print(f"rows before {len(rows)} after {len(out)} adds {len(ADDS)} rewords {len(REWORDS)}")
print("script.c:461 present:", "src/app/script.c:461" in final)
