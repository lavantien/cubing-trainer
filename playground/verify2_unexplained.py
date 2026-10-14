import re

LOG = "playground/gates/verify_2_script.log"

surv = []
acc = set()
for line in open(LOG, encoding="utf-8", errors="replace"):
    m = re.match(r"SURVIVED\s+m(\d+) (\S+) \[([^\]]+)\]", line)
    if m:
        surv.append((int(m.group(1)), m.group(2), m.group(3)))
    m = re.match(r"ACCEPTED\s+m(\d+)", line)
    if m:
        acc.add(int(m.group(1)))
for mid, loc, mut in surv:
    if mid not in acc:
        print(f"m{mid:04d}\t{loc}\t{mut}")
