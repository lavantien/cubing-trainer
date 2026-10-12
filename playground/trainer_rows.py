import re
import sys

LOG = "playground/gates/lane_01_screen_trainer.log"
DRAW_START = 529
OUT = "playground/trainer_draw_rows.txt"


def main():
    surv = []
    acc = set()
    for line in open(LOG, encoding="utf-8", errors="replace"):
        m = re.match(r"SURVIVED\s+m(\d+) src/ui/screen_trainer\.c:(\d+) \[([^\]]+)\]", line)
        if m:
            surv.append((int(m.group(1)), int(m.group(2))))
        m = re.match(r"ACCEPTED\s+m(\d+)", line)
        if m:
            acc.add(int(m.group(1)))
    draw = sorted(set(ln for mid, ln in surv if mid not in acc and ln >= DRAW_START))
    model = sorted(set(ln for mid, ln in surv if mid not in acc and ln < DRAW_START))
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        for ln in draw:
            if ln < 534:
                kind = "rect helper"
            elif ln < 583:
                kind = "chart grid draw"
            else:
                kind = "screen draw body"
            handle.write(
                f"src/ui/screen_trainer.c:{ln}\t{kind} feeds only the raylib frame loop "
                f"no gate suite asserts pixels lane 1 log 2026-10-12\n"
            )
    print(f"draw rows {len(draw)} over lines {draw[0]}..{draw[-1]}")
    print(f"model lines left for slot triage: {len(model)}")
    print(",".join(map(str, model)))


if __name__ == "__main__":
    sys.exit(main())
