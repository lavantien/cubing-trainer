import importlib.util
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTDIR = os.path.join(ROOT, "build", "visual_selftest")

RED = (230, 41, 55, 255)
GREEN = (0, 228, 48, 255)
ORANGE = (255, 161, 0, 255)
WHITE = (255, 255, 255, 255)
NEAR_WHITE = (250, 250, 250, 255)
BLUE = (0, 121, 241, 255)

GOLDENS = [
    "golden_solved_3.png",
    "golden_solved_7.png",
    "golden_scramble_3.png",
    "golden_mid_3.png",
]

SWAP_CASES = [
    ("golden_solved_3.png", (RED, GREEN), "solved_3_red_green"),
    ("golden_solved_3.png", (RED, WHITE), "solved_3_red_white"),
    ("golden_solved_3.png", (GREEN, WHITE), "solved_3_green_white"),
    ("golden_solved_3.png", (RED, ORANGE), "solved_3_red_orange"),
    ("golden_scramble_3.png", (RED, GREEN), "scramble_3_red_green"),
    ("golden_mid_3.png", (RED, GREEN), "mid_3_red_green"),
    ("golden_solved_7.png", (RED, GREEN), "solved_7_red_green"),
    ("golden_solved_3.png", (WHITE, NEAR_WHITE), "solved_3_bg_tint"),
]

STICKER_X = 360
STICKER_Y = 160
STICKER_SIZE = 40
GLOBAL_SHIFT = 3
NEAR_SHIFT = 1


def load_module():
    spec = importlib.util.spec_from_file_location(
        "visual_check", os.path.join(ROOT, "tools", "visual_check.py")
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


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


def recolor_sticker(pixels):
    out = [row[:] for row in pixels]
    for y in range(STICKER_Y, STICKER_Y + STICKER_SIZE):
        for x in range(STICKER_X, STICKER_X + STICKER_SIZE):
            if out[y][x] == GREEN:
                out[y][x] = BLUE
    return out


def shift_all(pixels, delta):
    out = []
    for row in pixels:
        new_row = []
        for r, g, b, a in row:
            new_row.append((min(255, r + delta), min(255, g + delta), min(255, b + delta), a))
        out.append(new_row)
    return out


def shift_one(pixels, delta):
    out = [row[:] for row in pixels]
    r, g, b, a = out[0][0]
    out[0][0] = (min(255, r + delta), min(255, g + delta), min(255, b + delta), a)
    return out


def report(name, expect_fail, result):
    ok, dist, deltas, cell = result
    caught = not ok
    verdict = "caught" if caught else "missed"
    status = "PASS" if caught == expect_fail else "FAIL"
    print(
        f"{status} {name}: verdict={verdict} hamming={dist} "
        f"mean_delta_r={deltas[0]:.3f} g={deltas[1]:.3f} b={deltas[2]:.3f} cell_delta={cell:.3f}"
    )
    return caught == expect_fail


def main():
    vc = load_module()
    os.makedirs(OUTDIR, exist_ok=True)
    results = []
    decoded = {}

    for name in GOLDENS:
        path = os.path.join(ROOT, "tests", "golden", name)
        decoded[name] = vc.decode_png(path)
        results.append(report(name, False, vc.compare_pixels(decoded[name][2], decoded[name][2])))

    first = os.path.join(ROOT, "tests", "golden", GOLDENS[0])
    results.append(report("path_api_identity", False, vc.compare(first, first)))

    def run_mutation(source, label, expect_fail, mutate):
        w, h, pixels = decoded[source]
        cand = mutate(pixels)
        path = os.path.join(OUTDIR, label + ".png")
        open(path, "wb").write(vc.encode_png(w, h, cand))
        results.append(report(label, expect_fail, vc.compare_pixels(pixels, cand)))

    for name, swap, label in SWAP_CASES:
        run_mutation(name, "swap_" + label, True, lambda p, s=swap: recolor(p, s))

    run_mutation(
        "golden_scramble_3.png",
        "sticker_recolor_green_blue",
        True,
        recolor_sticker,
    )
    run_mutation(
        "golden_scramble_3.png",
        "global_shift_" + str(GLOBAL_SHIFT),
        True,
        lambda p: shift_all(p, GLOBAL_SHIFT),
    )
    run_mutation(
        "golden_scramble_3.png",
        "near_one",
        False,
        lambda p: shift_one(p, NEAR_SHIFT),
    )

    passed = sum(results)
    total = len(results)
    print(f"visual selftest: {passed}/{total} cases met expectations")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
