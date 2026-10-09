import math

W = [0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046,
     1.54575, 0.1192, 1.01925, 1.9395, 0.11, 0.29605, 2.2698, 0.2315,
     2.9898, 0.51655, 0.6621]

FACTOR = 19.0 / 81.0
DECAY = -0.5
REQUEST_RETENTION = 0.9
MAX_IVL = 36500.0
MIN_S = 0.1
MIN_D = 1.0
MAX_D = 10.0


def clamp(x, lo, hi):
    return lo if x < lo else (hi if x > hi else x)


def s0(g):
    return W[g - 1]


def d0(g):
    return clamp(W[4] - math.exp(W[5] * (g - 1)) + 1, MIN_D, MAX_D)


def d_next(d, g):
    delta = -W[6] * (g - 3)
    shifted = d + delta * (10.0 - d) / 9.0
    return clamp(W[7] * d0(4) + (1.0 - W[7]) * shifted, MIN_D, MAX_D)


def retrievability(s, t):
    return (1.0 + FACTOR * t / s) ** DECAY


def s_recall(s, d, r, g):
    hard = W[15] if g == 2 else 1.0
    easy = W[16] if g == 4 else 1.0
    inc = (math.exp(W[8]) * (11.0 - d) * s ** (-W[9])
           * (math.exp(W[10] * (1.0 - r)) - 1.0) * hard * easy)
    return s * (1.0 + inc)


def s_forget(s, d, r):
    return W[11] * d ** (-W[12]) * ((s + 1.0) ** W[13] - 1.0) * math.exp(W[14] * (1.0 - r))


def s_same_day(s, g):
    return s * math.exp(W[17] * (g - 3 + W[18]))


def interval(s):
    raw = (s / FACTOR) * (REQUEST_RETENTION ** (1.0 / DECAY) - 1.0)
    if raw < 1.0:
        return 1
    if raw > MAX_IVL:
        return int(MAX_IVL)
    return int(round(raw))


failures = 0


def check(label, derived, golden, eps=1e-9):
    global failures
    ok = abs(derived - golden) <= eps
    if not ok:
        failures += 1
    print(f"{'OK  ' if ok else 'FAIL'} {label}: derived={derived!r} golden={golden!r} diff={derived - golden:.3e}")


print("== first review: S0(G), D0(G), due=today ==")
gold_s = [0.40255, 1.18385, 3.173, 15.69105]
gold_d = [7.1949, 6.488305268471453, 5.282434422319005, 3.2245015893713678]
for g in (1, 2, 3, 4):
    check(f"S0({g})", clamp(s0(g), MIN_S, MAX_IVL), gold_s[g - 1])
    check(f"D0({g})", d0(g), gold_d[g - 1])

print("== learning card S=10 D=5 t=4 today=104: recall hard/good/easy ==")
S, D, T = 10.0, 5.0, 4.0
R = retrievability(S, T)
check("R(10,4)", R, 0.9561491614605041)
gold_rs = [11.962060841460307, 19.791582142057077, 43.20991950624265]
gold_rd = [5.7994339073111085, 4.991832707311108, 4.184231507311108]
gold_ri = [12, 20, 43]
for i, g in enumerate((2, 3, 4)):
    d2 = d_next(D, g)
    s2 = clamp(s_recall(S, d2, R, g), MIN_S, MAX_IVL)
    check(f"S'r G={g}", s2, gold_rs[i])
    check(f"D'  G={g}", d2, gold_rd[i])
    ivl = interval(s2)
    print(f"{'OK  ' if ivl == gold_ri[i] else 'FAIL'} I  G={g}: derived={ivl} golden={gold_ri[i]} due={104 + ivl}")
    if ivl != gold_ri[i]:
        failures += 1

print("== learning card forget G=1 ==")
d1 = d_next(D, 1)
s1 = clamp(s_forget(S, d1, R), MIN_S, MAX_IVL)
check("S'f", s1, 1.7994687509165919)
check("D'  G=1", d1, 6.607035107311108)
ivl = interval(s1)
print(f"{'OK  ' if ivl == 2 else 'FAIL'} I G=1: derived={ivl} expect 2 due={104 + ivl} (golden 106)")
if ivl != 2:
    failures += 1

print("== same day S=10 G=1..4, D unchanged ==")
gold_sd = [5.010285241935083, 8.398413743809318, 14.077712147375411, 23.59754893600546]
for g in (1, 2, 3, 4):
    check(f"S'sd G={g}", clamp(s_same_day(S, g), MIN_S, MAX_IVL), gold_sd[g - 1])

print("== interval table at r=0.9 ==")
for s, want in ((1.0, 1), (5.0, 5), (10.0, 10), (50.0, 50), (100.0, 100), (365.0, 365), (3650.0, 3650)):
    ivl = interval(s)
    print(f"{'OK  ' if ivl == want else 'FAIL'} I(S={s}) = {ivl} want {want}")
    if ivl != want:
        failures += 1

print(f"failures: {failures}")
