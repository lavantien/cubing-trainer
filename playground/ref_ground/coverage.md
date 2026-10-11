# coverage diff

data: all 1077 sheet rows simulated in playground/ref_ground/simcheck.tsv, position map from position_map.tsv, pair spaces dumped by the same probe run (ct_targets_corner buffer 0 = 378 pairs, buffer 2 = 378 pairs, ct_targets_edge buffer 2 = 440 pairs). our pair space = ordered (t1, t2) with the buffer piece and the t1 piece excluded from t2, src/gen/targets.c ct_enumerate_targets. parse failures: 0 of 1077, every exported alg parses in our grammar.

## cherry

- edge: 551 rows, 506 distinct positions, 43 positions with multiple algs (45 extra rows), 149 clean piece 3-cycles
- corner: 328 rows, 318 distinct positions, 10 positions with multiple algs (10 extra rows), 35 clean piece 3-cycles
- flips: 25 rows
- twists: 20 rows
- parity: 26 rows
- ltct: 8 rows
- edge vs our UF buffer2-buffer pair space: 408 rows land inside, 0 rows with the right buffer fall outside, 143 rows use another buffer
- corner vs our UBL buffer0-buffer pair space: 2 rows land inside, 0 rows with the right buffer fall outside, 326 rows use another buffer
- corner vs our UFR buffer2-buffer pair space: 291 rows land inside, 0 rows with the right buffer fall outside, 37 rows use another buffer

- edge first-token (floating buffer) distribution: DB=3, DR=1, FL=7, FR=1, UB=83, UF=408, UL=13, UR=35
- corner first-token (floating buffer) distribution: UBL=2, UBR=10, UFL=25, UFR=291

## eggins

- edge: 71 rows, 71 distinct positions, 0 positions with multiple algs (0 extra rows), 22 clean piece 3-cycles
- corner: 38 rows, 38 distinct positions, 0 positions with multiple algs (0 extra rows), 2 clean piece 3-cycles
- flips: 0 rows
- twists: 5 rows
- parity: 2 rows
- ltct: 3 rows
- edge vs our UF buffer2-buffer pair space: 63 rows land inside, 0 rows with the right buffer fall outside, 8 rows use another buffer
- corner vs our UBL buffer0-buffer pair space: 0 rows land inside, 0 rows with the right buffer fall outside, 38 rows use another buffer
- corner vs our UFR buffer2-buffer pair space: 34 rows land inside, 0 rows with the right buffer fall outside, 4 rows use another buffer

- edge first-token (floating buffer) distribution: FR=1, UB=7, UF=63
- corner first-token (floating buffer) distribution: UFL=4, UFR=34

## categories our app has no case for

flips (two edges flipped in place), twists (two corners twisted in place), parity (edge swap plus corner swap), ltct (edge swap plus corner swap with twists plus in-place twist) have no category in the app's pair space: the trainer generates ordered 3-cycle pairs only. cherry rows: 25 flips, 20 twists, 26 parity, 8 ltct. eggins rows: 0 flips (empty sheet), 5 twists, 2 parity, 3 ltct.

the 780 3-style rows whose tokens include non-head stickers carry piece flips or twists (see blddb_semantics.md defect encoding) and are not exact 3-cycles; the 208 all-head rows are exact piece 3-cycles and are the only direct benchmark set for our generator.

- edge buffer2: their rows cover 372 of 440 ordered pairs in our space (84.5%), 100 of them with exact clean 3-cycles (22.7%)
- corner buffer2: their rows cover 288 of 378 ordered pairs in our space (76.2%), 31 of them with exact clean 3-cycles (8.2%)
- corner buffer0: their rows cover 2 of 378 ordered pairs in our space (0.5%), 0 of them with exact clean 3-cycles (0.0%)
