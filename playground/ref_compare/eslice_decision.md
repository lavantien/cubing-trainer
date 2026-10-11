# E-slice scoring decision packet

Question: should ct_score_alg keep scoring E as face D with the non-preferred penalty, or should the default config treat E (and by symmetry M, S) as preferred? This packet is the measured evidence. The change itself lands after the m6 close campaign, the config and chart artifacts are inside the campaign's verification surface.

## measured baseline

Edge exact-clean benchmark, 100 positions, ours 14W/40T/46L, mean dq +1.20 (playground/ref_compare/report.md). The 58 exact-clean losses split by spelling:

| loss group | n | mean dq | sum dq |
| --- | --- | --- | --- |
| their winner uses an E/M/S slice token, ours spells double face | 42 | 3.33 | 140 |
| ours double face, their winner has no slice | 10 | 4.20 | 42 |
| no double-face pattern in ours | 6 | 2.67 | 16 |

The 42-position group is the mechanism the report named: their winners interleave E at 8 to 10 qtm, ours spells the same maneuver as a 2U conjugation at 14 to 16 qtm because ct_score_alg maps E to face D at +2 subset penalty while 2U maps to face U at 0. Typical pair from losses_exact.tsv, UF-DR-BL: ours "U R' 2U R2 2U' L' 2U R2 2U' L R U'" 14 qtm, theirs "U' R' E' R2 E R' U" 8 qtm.

Upper bound on the flip: if the generator finds the same E-spelled family for all 42, edge exact mean dq drops from +1.20 toward about +0.28 (140 qtm removed over 100 positions), and the exact win/tie/loss record moves toward 14W/82T/4L. The true number needs a chart regeneration under the changed scoring, the bounded fallback search may miss some of their spellings, so treat +0.28 as the ceiling, not a forecast.

## the trade

Option A, keep the R U D default: the chart stays as landed, subset compliance stays 0.811 edge and 0.843 corner against the R U D definition, and raw qtm optimality concedes about 1.4 quarter turns on the 42 percent of exact positions where the world 1 freely uses the slice.

Option B, score E preferred (widen the default preferred set to R U D E, or score slice moves at 0 penalty): expected edge qtm parity with the reference corpus on most of the 42, at the cost of the ergonomic property the plan pinned, preferred-subset drilling to R U D. Cherry's own sheets use E without restriction, so B matches the world 1 style by evidence. M and S follow the same argument but appear rarely in the corpus, E carries almost all of it.

Consequences of B: one config default change in src/config/config.h, params_hash changes for every generated row so the whole chart regenerates (the cache key includes the subset), chart repins and the UFR chart artifact refresh, score-parts and subset-compliance tests repinned, README scoring section updated. No search, template, or classification code changes. Roughly one focused dev slot with its own verification chain.

## artifacts

losses_exact.tsv 58 rows, per_case.tsv 660 positions, aggregates.tsv, report.md. Split computed by the independent tokenizer rules over the two alg columns, slice tokens E M S with primes and doubles, double-face tokens U2 D2 F2 B2.

## measured outcome, 2026-10-11, HEAD d1d8098 plus the change

Option B generalized to E S M for the edge orbit landed as per-orbit preferred masks with the corner orbit pinned to R U D. Regenerated chart: 378 corner rows byte-identical, 129 of 440 edge rows re-spelled, 0 fallback rows, independent verify_chart 818/818 pass. Benchmark: edge exact-clean 16W/37T/47L at mean dq +1.08, down from 14W/40T/46L at +1.20, against the +0.28 ceiling. The gap to the ceiling is search space, not scoring: their winners interleave E inside setups and insertions, which the no-search-change scope keeps outside the generator's candidate set. Full before/after and the residual loss split live in report.md.
