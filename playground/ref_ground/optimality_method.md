# optimality comparison method (draft, no implementation)

status: draft only. the comparison runs in a later commit against the UFR-buffer chart once it lands; this file defines the method so the landing slot can implement it without re-deriving decisions.

## benchmark construction

pool both solvers' sheets (cherry + eggins). per distinct position, benchmark alg = the best row for that position by the primary metric below (qtm), tie broken by htm then by our weighted execution score then by rowid for determinism. only 3-style sheets (edge, corner) feed the benchmark, and only rows whose position tokens are all head stickers are eligible for the exact-3-cycle benchmark: 208 positions pool-wide carry at least one exact clean 3-cycle, the rest of their rows twist or flip pieces (blddb_semantics.md defect encoding) and enter only the labeled secondary benchmark. flips, twists, parity, ltct sheets are out of scope for the first comparison: our app has no category for them (coverage.md).

buffer alignment: their floating first token must equal our buffer sticker for the case to be comparable: edges buffer UF (sticker 2, letter C), corners buffer UFR (sticker 2, letter C) after the planned switch; the current HEAD default UBL leaves only 2 comparable corner rows, so the corner comparison waits for the UFR chart. their row `B-T1-T2` maps to our ordered pair (t1, t2) = (sticker of T1, sticker of T2), direction already verified identical (token1->token2->token3, 988/988).

## metrics

1. qtm, computed by an independent counter, not our formatter and not ct_count_qtm: a python tokenizer over the exported alg string counting each token as 2 if it ends in `2`, else 1, rotations x/y/z as 0 (they are re-grips, and all 1077 rows are center-fixed as written so rotations only appear as explicit re-grips: 9 rows contain them). this honors the direction-of-fit rule: expected values derive from the requirement (move counting is metric definition), never from the implementation under test. the probe already emits an independent qtm column in simcheck.tsv (quarter sums with rotations excluded); the final comparison re-derives it from the string to stay independent of the probe too.
2. htm: token count, rotations counted 0, same tokenizer.
3. our weighted execution score: ct_score_alg from src/gen/score.c on the sequence parsed by ct_notation_parse with default ct_config: qtm + subset penalty + regrip weight x regrips + finger weight x finger penalty. note for the implementer: the scorer scores slice moves (M/E/S are stored as face L/D/F moves at depth 2) and rotations (face R/U/F at depth 1..3) exactly like outer face turns of the same face; 624 of 1077 rows contain slices and 9 contain rotations (2 edge, 2 corner, 5 parity), so slice scoring is a shared baseline across both sides (our generator emits slices too) and the 4 rotation-bearing 3-style rows are flagged individually in the report.
4. classification: run ct_classify_alg on each benchmark alg, record ct_classification label, whether a commutator structure was found, setup/interchange/insertion split, and whether classification succeeded at all. report the share of their rows our classifier accepts as exact 3-cycles, cross-checked against the all-head-token ground truth from simcheck.tsv: the classifier's exact-3-cycle verdict must agree with the simulation's clean flag or the mismatch list gets printed (that cross-check is itself a validator of ct_classify_alg against an external oracle).

## comparison

our side: for each benchmark pair (t1, t2), the alg our generator produces for the same buffer (the UFR-buffer chart when it lands; the existing UF edge chart now). compare on:

- primary: qtm win/tie/loss counts per orbit (ours < theirs = win).
- delta = ours - theirs per pair; report mean and median delta per orbit, both metrics qtm and htm, plus weighted execution score delta.
- worst 10 deltas with both algs printed verbatim, position name, qtm/htm/score of each, and our classification label.
- coverage split: the win/tie/loss table computed twice, once over pairs where their benchmark is an exact clean 3-cycle (208 positions) and once over all their covered pairs with their defect-carrying algs included (labeled as such: their alg solves the sticker cycle, ours solves the piece cycle, so the defect-inclusive table compares different things and is informational only).
- their uncovered pairs: for pairs in our 378+440 space with no sheet row (68 edge, 90 corner at current coverage), no comparison, counted in the report.

## acceptance gates for the comparison run

- every benchmark row re-verified through the probe before scoring (parse, centers fixed, named sticker cycle CYCLE_A), failures listed, none silently dropped.
- qtm counter cross-checked on 20 rows by hand-written expected values in the test table (inline test table exception, centralized constants only).
- the independent python simulator (pysim.py) stays in the repo as the second engine for spot disputes: any row where classification and simulation disagree gets dumped through both.

## artifacts this method will produce

comparison.tsv (pair, position name, our alg, our qtm/htm/score/label, their best alg, their qtm/htm, defect flag, deltas), comparison.md (the tables above), extending playground/ref_ground/.
