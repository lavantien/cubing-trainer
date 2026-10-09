# pack seed provenance

Seed TSVs in this directory hold one row per case: case_key, alg, front_json, source_url, source_at, tab separated, single header line. The loader ct_pack_seed_file computes the per row content hash at load time, hashes are never stored in the TSVs. Retrieval date for every row is 2026-10-10. Where a source page lists several algorithms for one case the first suggested algorithm is stored.

## twoby2_cll.tsv

Source: https://jperm.net/algs/2x2/cll, J Perm's CLL page. The page renders its case table from https://jperm.net/lib/2x2cll.js, fetched and archived at playground/m5t2_jperm_cll.js. Retrieved 2026-10-10.

Page contents: the full 2x2 CLL set, 42 cases named L1 to L6, U1 to U6, T1 to T6, P1 to P6, H1 to H4, AS1 to AS6, S1 to S6, Adjacent, Diagonal, each with probability and one or more algorithm options.

Case key derivation: keys cll.01 to cll.42 follow the page order above, front_json carries the page case name. Generated programmatically from the archived data file by playground/m5t2_gen_seeds.py.

Count: 42 rows. Matches the plan's expected 42.

Discrepancy: the speedsolving wiki page https://www.speedsolving.com/wiki/index.php?title=CLL_algorithms_(2x2x2) is under construction with only the U and H sections filled, 34 of 42 algorithms present, so J Perm's complete list is the source of record.

## skewb_sarah.tsv

Source: https://sarah.cubing.net/skewb/skewb-guide.pdf, Sarah Strong's Speedskewbin guide, with case names cross read from https://sarah.cubing.net/skewb/my-method. Text and page renders archived at playground/m5t2_sarah_guide.txt and playground/m5t2_sarah_p1.png to m5t2_sarah_p11.png. Retrieved 2026-10-10.

Page contents: notation and the three variations. The intermediate variation is step 1 solve a side, step 2 solve the opposite side with 10 cases in 3 categories, step 3 solve the remaining centers with 3 cases. Step 1 lists 2 helper algorithms for the last first-layer corner.

Case key derivation: sarah.01 and sarah.02 are the step 1 corner helpers, the page shows them as two diagram positions without names, derived keys flc-a and flc-b. sarah.03 to sarah.05 are category 1 cases in page order, unnamed diagrams, derived keys cat1.1 to cat1.3. sarah.06 to sarah.09 are category 2 cases, named L2, L4, L3, L5 on the companion page. sarah.10 to sarah.12 are category 3 cases, named Pi5, Pi3, L1. sarah.13 to sarah.15 are the step 3 last 4 centers cases, named l4c-1 to l4c-3 on the companion page. Where a case offers a sledgehammer or hedgeslammer option the first listed option is stored.

Transcription note: the guide writes chains with an arrow glyph, R' F R F' followed by a category reference. Stored as ASCII, for example "R' F R F' -> cat. 1". The alg field is the source text for the case, the chain means do the trigger then finish with the referenced category.

Count: 15 rows, 2 step 1 helpers plus the 10 step 2 cases plus 3 step 3 cases. The intermediate variation itself defines the 10 step 2 cases, helpers and step 3 are included so the pack trains the full method.

Discrepancy: none, the plan named no count for this pack.

## pyraminx_l4e.tsv

Source: http://cubing.mikeg.me.uk/cubing-algs/html/l4e.html, Michael George's L4E page. The page renders from http://cubing.mikeg.me.uk/cubing-algs/data/l4e-min-005.js, fetched and archived at playground/m5t2_mikeg_l4e.js. Retrieved 2026-10-10.

Page contents: the L4E set for V first pyraminx, 36 cases in groups Down-Edge Solved, Down-Edge Flipped, Upper-Edge Solved, Upper-Edge Flipped, Nothing Placed, each with diagram, probability, and a suggested algorithm plus alternatives.

Case key derivation: keys l4e.01 to l4e.35 in page order DS2 to DS6, DF1 to DF6, US1 to US8, UF1 to UF8, NP1 to NP8, front_json carries the page case id. Generated programmatically from the archived data file by playground/m5t2_gen_seeds.py.

Count: 35 rows. The page lists 36 cases, DS1 is the solved state with an empty algorithm list and is excluded. This matches the 35 algs figure on the speedsolving wiki V First page.

Discrepancy: none versus the plan, which named no count.

## megaminx_4lll.tsv

Source: https://www.cubeskills.com/uploads/pdf/tutorials/intermediate-megaminx-techniques.pdf, Megaminx 4 Look Last Layer by Feliks Zemdegs and Andy Klise on CubeSkills. Extracted text archived at playground/m5t2_cubeskills_4lll.txt, page renders at playground/m5t2_4lll_p1.png to m5t2_4lll_p3.png. Retrieved 2026-10-10.

Page contents: a 3 page PDF with four sections, Edge Orientation, Corner Orientation, Edge Permutation, Corner Permutation. Each case is a diagram with one suggested algorithm, round brackets segment triggers.

Case key derivation: keys 4lll.01 to 4lll.39 in page reading order, EO1 to EO3, CO1 to CO16, EP1 to EP5, CP1 to CP15. The PDF prints no case names, the front_json names are section prefix plus position within the section. Algorithms that wrap across printed lines are joined with single spaces between all tokens.

Count: 39 rows, 3 EO plus 16 CO plus 5 EP plus 15 CP.

Discrepancy: the plan expected megaminx algs in R++ D-- style tokens. The sourced page uses R U R' U' style tokens with R2 and U2 doubles and BR wide turns, no ++ or -- tokens appear anywhere in the set. Source notation is stored verbatim. The plan named no count.

## sq1_cubeshape.tsv, sq1_co.tsv, sq1_eo.tsv, sq1_cp.tsv, sq1_ep.tsv

Pack structure: the plan names one square-1 pack. The loader ct_pack_seed_file binds a single stage per file, so the pack ships as 5 per-stage TSVs, all event CT_EVENT_SQUARE1, method CT_METHOD_LBL, stages CT_STAGE_CUBESHAPE, CT_STAGE_CO, CT_STAGE_EO, CT_STAGE_CP, CT_STAGE_EP. No square-1 specific method enum exists, CT_METHOD_LBL is the codebase beginner-method bucket and is the recorded convention for square-1 until a dedicated enum lands.

cubeshape source: https://cubingapp.com/algorithms/SQ1-Cube-Shape, archived at playground/seeds_sq1/cubingapp_cs.html. Retrieved 2026-10-10. Page contents: 169 cases grouped by optimal slash count 1 to 7, each a named shape pair such as Kite / Kite with one or more algorithms; notation is slash separated with bare u,d tuples and no parentheses, alternatives prefixed by bullets. Case key derivation: keys cs.001 to cs.169 in page order, front_json carries the page name. Where a case lists alternatives the first is stored, bullets dropped. Count: 169 rows covering 89 unordered shape pairs (9 self symmetric), the solved cube shape is not listed, and 89 plus the solved shape equals the 90 shape combinations of Lars Vandenbergh's taxonomy on http://www.cubezone.be/square1step1.html, archived at playground/seeds_sq1/cubezone_step1.html. His page lists shapes only, no cubeshape algorithms, which is why a second source was needed.

CO source: https://www.speedcube.us/blogs/square-1-co-corner-orientation, archived at playground/seeds_sq1/speedcubeus_square-1-co-corner-orientation.html. Retrieved 2026-10-10. Page contents: 8 cases CO 01 to CO 08, one textual algorithm each except CO 08, whose algorithm exists only as an image on both the set page and its case page (archived at speedcubeus_co08.html). Case key derivation: keys co.01 to co.07 by case number, front_json carries the page name. Count: 7 rows, the CO 08 gap recorded here. Cross-checks: the 5 corner layering algs of http://www.cubezone.be/square1step2.html (cubezone_step2.html) and the basic plus advanced orient corners union of Andy Klise's sheet at https://www.kungfoomanchu.com/guides/andy-klise-square-1.pdf (klise_square1.pdf, text at klise_square1.txt) are all token equal to rows of this set modulo source spacing.

EO source: https://sarah.cubing.net/square-1/eo, archived at playground/seeds_sq1/sarah_sq1_eo.html. Retrieved 2026-10-10. Page contents: cases sq1-eo0 to sq1-eo7, sq1-eo0 is the skip, one algorithm each, no case names. Case key derivation: keys eo.01 to eo.07, front_json carries the page li id. Count: 7 rows. Cross-check: set equal to the speedcube.us EO set, 7 of 7 algorithms identical, archived at speedcubeus_square-1-eo-edge-orientation.html.

CP source: https://sarah.cubing.net/square-1/cp, archived at playground/seeds_sq1/sarah_sq1_cp.html. Retrieved 2026-10-10. Page contents: a standard tab with sq1-cp0 to sq1-cp8 and a parity tab with sq1-cpp0 to sq1-cpp8, index 0 of each is the skip, one algorithm each. Case key derivation: keys cp.01 to cp.08 and cpp.01 to cpp.08, front_json carries the page li id, the cpp prefix marks the parity section rows, these are the adjacent corner swap parity algs. Count: 16 rows. Cross-check: the 8 textual algs of speedcubeus_square-1-cp-corner-permutation.html are all contained in the 16.

EP source: https://sarah.cubing.net/square-1/ep, archived at playground/seeds_sq1/sarah_sq1_ep.html. Retrieved 2026-10-10. Page contents: cases sq1-ep00 to sq1-ep99 with numeric names and nicknames such as Adj on U, Opp on U, Adj-Adj through W-W, sq1-ep00 is the skip. Page notes: leading (U) or (D) tokens are AUF or ADF adjustments, a trailing * marks an equator flip after the algorithm, red text marks parity cases. Case key derivation: keys ep.01 to ep.99 by page numbering, front_json carries the nickname, or the numeric name where no nickname exists, plus a parity true field for red marked cases. Count: 99 rows.

Normalization recorded for the square-1 pack: HTML nonbreaking spaces and CubingApp bullet markers replaced with spaces, whitespace runs collapsed to single spaces, leading and trailing whitespace trimmed, one stray ) removed in sq1-cpp6 where the source prints (-2,4)). AUF prefixes, trailing * markers, and per source spacing quirks such as co.01 storing (1,0)/(3,0)/(-1,0) without inner spaces are kept verbatim.

Count: 169 cubeshape plus 7 CO plus 7 EO plus 16 CP plus 99 EP, 298 rows total.

Discrepancy: none versus the plan, which named the square-1 stages but no counts. speedcubedb.com, cuberoot.me, and brandonlin.com square-1 sets are client side rendered or partial and were rejected as sources.

## clock_algs.tsv

Source: http://www.jaapsch.net/puzzles/clock.htm, Jaap Scherphuis's Rubik's Clock page, archived at playground/seeds_clock/jaap_clock.html. Retrieved 2026-10-10. The page's human solution uses exactly 14 moves, and Stefan Pochmann's page http://www.stefan-pochmann.de/spocc/speedsolving/clock/, archived at playground/seeds_clock/pochmann_clock.html, calls it the well-known 14-move solution, so this is the canonical standard set.

Page contents: the solution is front cross equalize in 4 moves (steps 1 to 3), front all clocks to 12 in 1 move (steps 5 to 6), flip the puzzle, back cross equalize in 4 moves (step 8), back corner equalize in 4 moves (steps 9 to 11), back all clocks to 12 in 1 move (step 13).

Case key derivation: keys clock.01 to clock.14 in solution order, front_json case labels are derived (front cross 1 to 4, front all to 12, back cross 1 to 4, back corners 1 to 4, back all to 12). No public page writes the standard clock method as fixed notation algorithms because every turn amount depends on the clock state, so each row's alg is the source's verbatim step text, pin setup plus dial action, repeated verbatim for the 4 repeats the source describes. The current WCA clock notation, pin tokens UR DR DL UL U R D L ALL with signed hour amounts, is documented on https://www.speedsolving.com/wiki/index.php?title=Clock_notation, archived at playground/seeds_clock/speedsolving_clock_notation.html, as the reference for later token support.

Count: 14 rows, 12 equalize moves plus 2 final rotations, matching the plan's standard ~14 algs.

Discrepancy: none versus the plan count. J Perm has no clock algorithm page, his site index was checked and archived at playground/seeds_clock/jperm_algs_index.html. Enum convention: event CT_EVENT_CLOCK, method CT_METHOD_LBL, stage CT_STAGE_CONCEPT. LBL is the beginner method placeholder, no clock specific enum exists, and CONCEPT holds the fixed procedure set, mirroring the existing CT_METHOD_LBL CT_STAGE_CONCEPT pairing already used in the selector's stage lists.
