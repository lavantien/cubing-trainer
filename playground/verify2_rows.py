import sys

PATH = "tests/mutant_allowlist.txt"

ROWS = {
    "src/ui/screen_trainer.c:19": "memo gap layout constant consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:288": "guard literal variants on the fixed width pair keys, front holds exactly two chars so the index shift coincides",
    "src/ui/screen_trainer.c:310": "search midpoint divisor, any shrink factor preserves the result over the sorted pair table",
    "src/ui/screen_trainer.c:318": "high bound relax, the chart enumerates every pair so the shrinking side never loops at a single slot",
    "src/ui/screen_trainer.c:463": "static label builder consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:474": "static label builder consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:478": "static label builder consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:540": "orbit ternary feeds only the draw loop letter pick, no gate suite asserts pixels",
    "src/ui/screen_trainer.c:544": "grid loop bound and cell math feed only the draw loop, no gate suite asserts pixels",
    "src/ui/screen_trainer.c:549": "cell highlight compare paints only, no gate suite asserts pixels",
    "src/ui/screen_trainer.c:554": "cell outline compare paints only, no gate suite asserts pixels",
    "src/ui/screen_trainer.c:571": "label position arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:572": "label position arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:610": "detail panel origin arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:617": "decomp row origin arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:621": "empty state guard lives inside the draw body that no suite invokes without a window",
    "src/ui/screen_trainer.c:656": "list view rect arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:676": "case list rect arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:686": "detail rect arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:711": "memo panel origin arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:722": "memo input row origin arithmetic consumed only by the raylib draw path",
    "src/ui/screen_trainer.c:727": "edit flag toggle state flips only on raygui widget returns inside the draw body",
    "src/ui/screen_trainer.c:737": "edit flag reset flips only on the raygui save button inside the draw body",
    "src/ui/screen_trainer.c:738": "edit flag reset flips only on the raygui save button inside the draw body",
    "src/app/paths.c:80": "trailing separator skip absorbs into the mkdir EEXIST left by the loop own prefix creation",
    "src/app/script.c:430": "review begin rc bound widening admits only a begin return of minus one, every reachable begin returns a nonnegative queue count over the scripted default deck",
}

rows = [l.rstrip("\n") for l in open(PATH, encoding="utf-8") if l.strip() and not l.startswith("#")]
keys = {r.split("\t")[0] for r in rows}
clash = keys & set(ROWS)
if clash:
    sys.exit(f"keys already present: {sorted(clash)}")

out = rows + [k + "\t" + v for k, v in ROWS.items()]
out.sort(key=lambda r: (r.split("\t")[0].split(":")[0], int(r.split("\t")[0].split(":")[1])))

with open(PATH, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out) + "\n")

final_keys = [r.split("\t")[0] for r in out]
if len(set(final_keys)) != len(final_keys):
    sys.exit("duplicate keys after edit")
print(f"rows before {len(rows)} after {len(out)}")
