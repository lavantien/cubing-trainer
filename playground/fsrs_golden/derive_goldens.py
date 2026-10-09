import math

W = [
    0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046,
    1.54575, 0.1192, 1.01925, 1.9395, 0.11, 0.29605, 2.2698, 0.2315,
    2.9898, 0.51655, 0.6621,
]

FACTOR = 19.0 / 81.0
DECAY = -0.5
RETENTION = 0.9
MAX_IVL = 36500.0
MIN_S = 0.1
MIN_D = 1.0
MAX_D = 10.0


def clamp(x, lo, hi):
    if x < lo:
        return lo
    if x > hi:
        return hi
    return x


def clamp_s(x):
    return clamp(x, MIN_S, MAX_IVL)


def clamp_d(x):
    return clamp(x, MIN_D, MAX_D)


def d_new(g):
    return clamp_d(W[4] - math.exp(W[5] * (g - 1)) + 1.0)


def d_base_easy():
    return clamp_d(W[4] - math.exp(W[5] * 3.0) + 1.0)


def d_next(d, g):
    delta = -W[6] * (g - 3)
    d_dash = d + delta * (10.0 - d) / 9.0
    return clamp_d(W[7] * d_base_easy() + (1.0 - W[7]) * d_dash)


def retrievability(s, t):
    return pow(1.0 + FACTOR * t / s, DECAY)


def s_recall(s, d, r, g):
    hard = W[15] if g == 2 else 1.0
    easy = W[16] if g == 4 else 1.0
    inc = math.exp(W[8]) * (11.0 - d) * pow(s, -W[9]) * (math.exp(W[10] * (1.0 - r)) - 1.0) * hard * easy
    return clamp_s(s * (1.0 + inc))


def s_forget(s, d, r):
    return clamp_s(W[11] * pow(d, -W[12]) * (pow(s + 1.0, W[13]) - 1.0) * math.exp(W[14] * (1.0 - r)))


def s_same_day(s, g):
    return clamp_s(s * math.exp(W[17] * (g - 3 + W[18])))


def interval(s):
    x = (s / FACTOR) * (pow(RETENTION, 1.0 / DECAY) - 1.0)
    return int(clamp(math.floor(x + 0.5), 1.0, MAX_IVL))


def main():
    print("[first review]")
    for g in range(1, 5):
        print(f"G={g} S={W[g - 1]!r} D={d_new(g)!r}")
    print("[card S=10 D=5 elapsed=4]")
    r410 = retrievability(10.0, 4.0)
    print(f"R={r410!r}")
    print("[recall review]")
    for g in (2, 3, 4):
        dn = d_next(5.0, g)
        sp = s_recall(10.0, dn, r410, g)
        print(f"G={g} S={sp!r} D={dn!r} I={interval(sp)}")
    print("[forget review]")
    dn1 = d_next(5.0, 1)
    sf = s_forget(10.0, dn1, r410)
    print(f"G=1 S={sf!r} D={dn1!r} I={interval(sf)}")
    print("[same day review]")
    for g in range(1, 5):
        print(f"G={g} S={s_same_day(10.0, g)!r}")
    print("[interval table retention=0.9]")
    for s in (1.0, 5.0, 10.0, 50.0, 100.0, 365.0, 3650.0):
        print(f"S={s!r} I={interval(s)}")


if __name__ == "__main__":
    main()
