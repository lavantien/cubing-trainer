import re
import sys
import glob

KILLED = {}
SURV = {}

LOGS = [
    "playground/gen_fix/mutate_synth_shard0.log",
    "playground/gen_fix/mutate_synth_shard1.log",
    "playground/gen_fix/mutate_synth_shard2.log",
    "playground/gen_fix/mutate_synth_shard3.log",
    "playground/gen_fix/mutate_synth_shard4.log",
    "playground/gen_fix/mutate_synth_shard5.log",
    "playground/gen_fix/mutate_synth_shard6.log",
    "playground/gen_fix/mutate_synth_shard7.log",
    "playground/gen_fix/mutate_synth_shard0_resume3.log",
    "playground/gen_fix/mutate_synth_shard4_resume4.log",
    "playground/gen_fix/mutate_synth_gap540.log",
    "playground/gen_fix/mutate_synth_extra_ids.log",
    "playground/gen_fix/mutate_synth_v2_aa.log",
    "playground/gen_fix/mutate_synth_v2_ab.log",
    "playground/gen_fix/mutate_synth_v2_ac.log",
    "playground/gen_fix/mutate_synth_v2_ad.log",
]

for path in LOGS:
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except FileNotFoundError:
        print("MISSING LOG", path)
        continue
    for m in re.finditer(r"^KILLED\s+m(\d+)\s+(\S+)", text, re.M):
        KILLED[int(m.group(1))] = m.group(2)
    for m in re.finditer(r"^SURVIVED\s+m(\d+)\s+(\S+)", text, re.M):
        mid = int(m.group(1))
        SURV[mid] = m.group(2)
        KILLED.pop(mid, None)

rows = set()
for line in open("tests/mutant_allowlist.txt", encoding="utf-8"):
    key = line.split("\t")[0].strip()
    if key:
        rows.add(key)

HANGS = {115, 179, 349, 551, 559, 666, 772, 878}
unexplained = []
no_verdict = []
for mid in SURV:
    if mid in HANGS:
        continue
    if SURV[mid] not in rows:
        unexplained.append((mid, SURV[mid]))

for mid in range(1, 929):
    if mid in SURV or mid in KILLED or mid in HANGS:
        continue
    no_verdict.append(mid)

print("killed with verdict:", len(KILLED))
print("survivors recorded:", len(SURV))
print("hang ids skipped:", sorted(HANGS))
print("UNEXPLAINED survivors:", unexplained)
print("ids with no verdict anywhere:", no_verdict)
