# 3-style optimality comparison against the blddb reference sheets

Executed 2026-10-11 at HEAD 36c9111 by the ref-compare slot, re-run the same day at HEAD d1d8098 plus the edge slice-preference change by the eslice slot. Config defaults with corner buffer UFR sticker 2 and edge buffer UF sticker 2. Ours is playground/buffer_ufr/chart_buffer2.tsv, the landed UFR + UF fixed-buffer chart, verified current by regenerating 30 random rows (seed 20261011) identically through a fresh HEAD synth probe with 0 fallback rows in the whole chart. Theirs is the blddb reconstruction corpus under playground/ref_ground: of 1077 rows, 796 in-space 3-style rows (471 edge, 325 corner) carry our buffer as first token and land inside our pair space, 0 buffer rows fall outside, 192 rows use another buffer. Mapping runs through position_map_inverse.tsv, benchmark is the best row per distinct position by qtm with htm then score tiebreaks, 471 edge rows collapse to 372 positions and 325 corner rows to 288. Move counts come from the independent python tokenizer in run_compare.py, rotations counted 0, never from our formatter, validated against a 20-case hand-written expected table before the run. The tokenizer qtm equals the probe-simulated qtm on every benchmark row, and a probe_sim rebuild at HEAD reproduces simcheck.tsv, both position maps, and both pair files byte for byte, which re-verifies every benchmark row end to end. Method details in playground/ref_ground/optimality_method.md.

## headline

After the owner decision that edge-orbit slice moves E, S, and M score at 0 subset penalty (corner orbit unchanged at R U D), the whole chart regenerated: 378 corner rows byte-identical, 129 of 440 edge rows re-spelled, 0 fallback rows, and the probe scoring is per orbit from this run on. Edge before and after the change:

| slice | n | win | tie | loss | mean qtm delta | median | mean htm delta | mean score delta |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| edge exact clean, before | 100 | 14 | 40 | 46 | +1.20 | 0 | +0.11 | +0.93 |
| edge exact clean, after | 100 | 16 | 37 | 47 | +1.08 | 0 | +0.13 | -2.01 |
| edge all covered, before | 372 | 38 | 138 | 196 | +1.33 | +1 | +0.55 | +0.88 |
| edge all covered, after | 372 | 36 | 138 | 198 | +1.41 | +1 | +0.64 | -1.63 |

| slice | n | win | tie | loss | mean qtm delta | median | mean htm delta | mean score delta |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| corner exact clean | 31 | 11 | 8 | 12 | +0.84 | 0 | -2.94 | +8.65 |
| corner all covered | 288 | 85 | 102 | 101 | +0.39 | 0 | -1.31 | +10.08 |

Exact clean means both tokens are head stickers, a pure piece 3-cycle on both sides. All covered includes the defect-carrying rows. Positive delta means ours is longer or worse. The exact-clean mean qtm gap improved by 0.12 and the weighted score delta flipped from +0.93 to -2.01: under our own scorer the regenerated edge chart now beats their winners on execution score on average, because the metric prices slice moves at 0 for the edge orbit by the same owner decision. The all-covered mean qtm moved from +1.33 to +1.41, the scoring flip trades raw qtm for slice usage on the defect-carrying rows, consistent with the chart-wide edge qtm sum rising from 4860 to 4888 while slice tokens rose from 636 to 966. Corner rows are identical in content, only their params_hash era changed.

Coverage of our chart by their reconstructions: 372 of 440 edge pairs, 288 of 378 corner pairs. Subset compliance against the preferred set, R U D for corners and R U D plus M E S for edges after the decision: ours 0.870 edge and 0.843 corner, theirs 0.936 and 0.732. Before the change, edge compliance against R U D was 0.811 ours and 0.752 theirs.

## loss pattern, the actionable seed

Their edge winners interleave the E slice directly at 8 to 10 qtm while our generator spells the same physical move as a preferred-face 2U conjugation at 14 to 16 qtm. The first run pinned this on scoring: ct_score_alg mapped slice moves to their base faces, S and M at +2, and the weighted objective avoided slice spellings. The decision packet bounded the scoring flip at +0.28 mean dq if the generator could find their exact spellings, and warned the number was a ceiling, not a forecast. The measured result is +1.08: the scoring change alone re-ranks only candidates the template-plus-outer-setup search already generates, and their 8-to-10-qtm E-interleaved spellings mostly need slice moves inside setups, which the search space still excludes by the decided no-search-change scope. The remaining 47 exact-clean edge losses split by the same tokenizer rules: 36 where their winner uses an E/M/S slice token and ours still spells double face (sum dq 106, mean 2.94), 2 ours-double-face their-plain (sum 4), 9 with no double-face pattern in ours (sum 30). Our own spelling now contains slice tokens in 336 of 372 covered edge rows. The full loss list is losses_exact.tsv and the worst 10 with both algs printed are worst10_exact.tsv and worst10_all.tsv. One rotation-bearing alg won a benchmark slot, cherry UF-BD-BU, counted at 8 qtm with rotations 0, recorded in rot_rows.tsv with the other 2 rotation rows.

## classifier cross-check

ct_classify_alg accepts 410 of 634 defect-carrying rows as exact 3-cycles: cl_orbit_exact_cycle checks sticker-cycle shape only, so a corner 3-cycle with twists summing 0 or an edge 3-cycle with 2 flips still decomposes into sticker 3-cycles and passes the gate. Confirmed independently through simcheck and pysim on cherry corner row 4, UFR-UBL-LUF, where the UBL piece arrives at UFL twisted while the classifier labels it orthogonal. Structure recall on clean human rows is also low, 96 of 162 labeled unknown, mostly conjugated forms at off-table lengths, which is partially by design. Both are ct_classify_alg findings: the orientation gate contradicts the documented boundary that claims orientation included, and the recall question needs a decision on whether conjugated human forms should decompose.

## classification distributions

Their rows by head versus non-head target and our chart, from aggregates.tsv: their corners label unknown 31 of 33 head rows and column 49, A9 33, per special 29, pure 25, orthogonal 18, cyclic shift 6 of non-head rows, their edges label pure 88, orthogonal 51, B9 36, SP9 34, A9 23, SP7 18 of non-head rows, our chart labels corners orthogonal 110, per special 79, A9 64, pure 63, unknown 43, column 19 and edges orthogonal 150, pure 103, unknown 91, SP7 28, A9 24, B9 21, SP9 11, half slice-plane 6, SP5 6.

## artifacts

mapped_rows.tsv 796 in-space ref rows, per_case.tsv 660 benchmark positions, aggregates.tsv the source of every number above, losses_exact.tsv, worst10_exact.tsv, worst10_all.tsv, rot_rows.tsv, run_compare.py, refcmp_probe.c, simfresh a fresh probe_sim reproduction byte-equal to the committed simcheck.
