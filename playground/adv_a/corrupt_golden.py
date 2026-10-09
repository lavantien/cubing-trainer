import importlib.util
import sys

spec = importlib.util.spec_from_file_location("vc", "tools/visual_check.py")
vc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(vc)

RED = (230, 41, 55, 255)
GREEN = (0, 228, 48, 255)
ORANGE = (255, 161, 0, 255)
WHITE = (255, 255, 255, 255)


def recolor(pixels, swap):
    a, b = swap
    out = []
    for row in pixels:
        new_row = []
        for px in row:
            if px == a:
                new_row.append(b)
            elif px == b:
                new_row.append(a)
            else:
                new_row.append(px)
        out.append(new_row)
    return out


def run(golden, swap, name):
    w, h, gp = vc.decode_png(golden)
    cp = recolor(gp, swap)
    path = "playground/adv_a/regen/corrupt_" + name + ".png"
    open(path, "wb").write(vc.encode_png(w, h, cp))
    ok, dist, deltas, cell = vc.compare(golden, path)
    print(f"{golden} swap {name}: verdict={'PASS (MISSED)' if ok else 'FAIL (caught)'} hamming={dist} deltas={deltas[0]:.3f}/{deltas[1]:.3f}/{deltas[2]:.3f} cell={cell:.3f}")


run("tests/golden/golden_solved_3.png", (RED, GREEN), "red_green")
run("tests/golden/golden_solved_3.png", (RED, WHITE), "red_white")
run("tests/golden/golden_solved_3.png", (GREEN, WHITE), "green_white")
run("tests/golden/golden_solved_3.png", (RED, ORANGE), "red_orange")
run("tests/golden/golden_scramble_3.png", (RED, GREEN), "red_green")
run("tests/golden/golden_mid_3.png", (RED, GREEN), "red_green")
run("tests/golden/golden_solved_7.png", (RED, GREEN), "red_green")
run("tests/golden/golden_solved_3.png", (WHITE, (250, 250, 250, 255)), "bg_tint")
