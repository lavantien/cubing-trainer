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

## bigcube_l2e.tsv

Sources: https://www.speedcube.us/blogs/5x5-last-two-edges, archived at playground/seeds_bigcube/speedcubeus_5x5_l2e.html. Retrieved 2026-10-10, the page was fetched during the original big-cube seed pass and held back then because L2E mixes parity and non parity edge pairing cases; it is seeded now as the reduction trainer content for 5x5 through 7x7 under its own file.

Page contents: the page presents a 12 case last two edges set for the 5x5, cases L2E 01 through L2E 12, each with one algorithm. The cases are the standard dedgel flip, double flip, and slice combinations that finish edge pairing during reduction, and the page states the set is for the 5x5. The same case shapes occur on 6x6 and 7x7 with wider slice depths, and the registration covers all three sizes from this one file, mirroring how the parity file registers.

Case key derivation: keys l2e.01 to l2e.12 in the page's case numbering, front_json carries the case label without the dot, for example l2e.07.

Count: 12 rows.

Enum convention: event CT_EVENT_5X5, CT_EVENT_6X6, and CT_EVENT_7X7, method CT_METHOD_REDUCTION, stage CT_STAGE_PARITY. The stage is PARITY because the reduction trainer's parity list is the seeded deck for all three sizes, and this file merges with the bigcube_parity rows into one 17 row deck per event under the pack table's unique event, method, stage, case key constraint.

Notation: algorithms are stored verbatim in the source formatting, including the parenthesized setup groups in cases 01 and 03. Consistent with the pack seed display-only policy recorded in the README event pack section, the notation grammar rejects these site forms by design and the rows show alg text in the selector without animating until curated.

Discrepancy: none. The page's 12 cases all have text algorithms, none are image only.

## bigcube_parity.tsv

Sources: https://www.speedcube.us/blogs/speedcubing-solutions/4x4-oll-parity-algorithms, https://www.speedcube.us/blogs/speedcubing-solutions/4x4-pll-parity-algorithms, and https://www.speedcube.us/blogs/speedcubing-solutions/5x5-edge-parity-solution-algorithm, archived at playground/seeds_bigcube/speedcubeus_4x4_oll_parity.html, speedcubeus_4x4_pll_parity.html, and speedcubeus_5x5_edge_parity.html. Retrieved 2026-10-10. J Perm's 4x4 OLL and PLL parity pages and their data files (jperm_4x4_oll.html, jperm_4x4pll.js, jperm_4x4oll.js) are archived as cross-checks: his OLL parity definition, the [*] token on his 4x4 OLL page, equals the oll4 row modulo the comma formatting of the speedcube.us page.

Page contents: the OLL parity page gives 2 algorithms, the speedsolve form that also disturbs other pieces and the pure two inner edge flip, the PLL parity page gives 2 cases, opposite edge swap and adjacent edge swap, and the 5x5 edge parity page gives 1 algorithm, single flipped wing pair, and states it works for 4x4 up to 7x7.

Case key derivation: keys parity.oll4, parity.oll4flip, parity.pll4, parity.pll4adj, parity.wing5, front_json carries the same name without the prefix. Pack is loaded as event CT_EVENT_4X4, method CT_METHOD_YAU, stage CT_STAGE_PARITY, the 5x5+ row is keyed by cube size in the suffix and lives in the same pack because the source states the one algorithm covers 4x4 through 7x7 and no 6x6 or 7x7 specific parity algorithm page exists on the sourced site. The registry also binds this file under event CT_EVENT_5X5, CT_EVENT_6X6, and CT_EVENT_7X7 with method CT_METHOD_REDUCTION and stage CT_STAGE_PARITY, feeding the reduction trainer together with bigcube_l2e.tsv into one 17 row deck per event. Applicability across those registrations is uneven and recorded here rather than silently assumed: the wing5 row is the row of record for every size per the source claim, the oll4 and pll4 rows describe dedge parity which occurs on even cubes only, so they apply directly to 4x4 and 6x6, and for 5x5 and 7x7 they ride along because the registry binds whole files and no odd-cube parity page exists on the sourced site; the odd-cube reduction content of record is the bigcube_l2e.tsv set.

Count: 5 rows.

Notation: algorithms are stored verbatim in the source formatting, including comma separators between tokens and the source spacing quirk in oll4 where one token prints as "Rw U2 ,". HTML nonbreaking spaces are replaced with plain spaces.

Discrepancy: the task example keys included a center parity row for 5x5 and larger. No speedsolving center parity case exists on any big cube, centers are freely permutable during reduction, so no center parity row ships and the gap is recorded here. The nearest public content is the speedcube.us commutator page for swapping two centers on big cubes, archived at playground/seeds_bigcube/speedcubeus_commutators.html, and its 4x4 last two centers page, archived at speedcubeus_l2c.html, both technique pages rather than parity cases. Sarah's 5x5 L2C and L2E sets (sarah_5x5x5_l2c.html, sarah_5x5x5_l2e.html) and the speedcube.us 5x5 L2E 12 case set (speedcubeus_5x5_l2e.html) were also fetched and left out of this pack: L2E mixes parity and non parity edge pairing cases and is not parity only content. The 4x4 corner swap page (speedcubeus_4x4_corner_swap.html) presents PLL parity as two corner swap two edge swap steps, its first step algorithm exists only as an image there, so the two text algorithms of the PLL parity page are the rows of record.
## fto_l4t_redux.tsv

Sources: the L4T Redux L4E spreadsheet by James Straughan linked from https://www.speedsolving.com/wiki/index.php/Face_Turning_Octahedron, exported as CSV, archived at playground/seeds_fto/l4t_redux_l4e_sheet1.csv (algorithm sheet), l4t_redux_l4e_g2102380954.csv (notation sheet), l4t_redux_l4e_g1864879911.csv (credits sheet, algorithms by James Straughan, images from fto-image-gen), and the L4T Redux guide document by Vincent Trang, exported as text at playground/seeds_fto/l4t_redux_guide.txt. The wiki page itself is archived at playground/seeds_fto/speedsolving_fto.html and speedsolving_fto.txt, trimmed to the Methods section onward, the page intro before Methods is not content of record for this pack. Retrieved 2026-10-10.

Page contents: the algorithm sheet lists 35 named cases in 8 groups, Last Layer, Last Three Edges, Flipped Edges, Polish Flip, Separated Bar, Connected Bar, No Bar, Double Ugly, each with one algorithm. Two unlabeled alternate algorithms sit in a second row under Good Niky and Super Sledge, the first algorithm is stored per the convention. The guide contributes the reduction parity fix U BR U BR' F U F', applied before the final R move when the swapping direction is wrong.

Case key derivation: keys l4t.01 to l4t.36 in sheet reading order, l4t.36 is the guide parity fix, front_json carries group.case names from the sheet.

Transcription note: the Double Sexy algorithm prints its apostrophes as a Unicode right single quote in the source CSV, stored as ASCII apostrophes.

Enum convention: CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T.

Count: 36 rows. The plan named no count for this pack.

Notation coverage: 35 of 36 algs parse fully. The digit-then-prime suffix order gap closed when a125acc made the parser accept L2' and R2', pinned on rows l4t.08 and l4t.22 in tests/unit_notation.c. The remaining gap, still open, is BR and BR' in l4t.36: the FTO back-right face is outside the NxN token table. FTO notation as a whole is a pending parser surface, flagged rather than rejected.

## bld_oldpoch.tsv

Sources: https://jperm.net/bld for the swap algorithms, the parity algorithm, and the per letter setup moves, archived as extracted text at playground/seeds_bld/jperm_bld.md and refetched live for the parity pass at playground/parity_flip_twist/jperm_bld.md, the page was fetched server side because jperm.net timed out from the build machine. http://www.stefan-pochmann.de/spocc/blindsolving/3x3/old.php, the inventor's original method page, archived at playground/seeds_bld/pochmann_old.html and pochmann_old.txt. https://www.speedsolving.com/wiki/index.php/Old_Pochmann, Classic Pochmann, archived at playground/seeds_bld/speedsolving_oldpochmann.html and speedsolving_oldpochmann.txt. http://www.stefan-pochmann.de/spocc/blindsolving/M2R2/ for the corner twist row, archived at playground/parity_flip_twist/pochmann_m2r2.md. https://bldtrainer.com/parity for the buffer convention parity companion, archived at playground/parity_flip_twist/bldtrainer.md. J Perm's tutorial description at https://www.youtube.com/watch?v=A64Sy4WKiWY reverified the parity algorithm, archived at playground/parity_flip_twist/jperm_op_video.md. Retrieved 2026-10-10.

Page contents: J Perm's tutorial defines the T perm edge swap UR to UL, the Y perm corner swap LBU to DFR, the parity algorithm, and publishes setup moves per letter inside two fully worked example solves. Pochmann's page gives the original T perm plus the direct UF and UB PLL targets and the FR and RF setup examples. The wiki page gives the adjacent corner swap variant R' F R' B2 R F' R' B2 R2 U' with a UFL buffer and URF target. No Old Pochmann origin publishes a twist algorithm, J Perm's page, his description FAQ, the wiki page, and bldtrainer all document cycle break handling only, so the twist row of record is the third entry of Pochmann's M2/R2 orient corners list. bldtrainer's parity lesson publishes the parity companion R U R' F' R U R' U' R' F R2 U' R' U' with the effect claim that it swaps the leftover two edges and two corners with no centers move, and its UF edge buffer and UBL corner buffer match the shipped letter table.

Case key derivation: op.01 to op.08 are the core rows, swap algorithms, Pochmann's direct targets and setup examples, and parity, op.09 to op.24 are edge letter setups, op.25 to op.38 are corner letter setups, op.39 is the corner twist row, op.40 is the bldtrainer parity companion. front_json carries the letter, orbit, and target position for setup rows, and the kind plus simulated effect for the special rows, op.39 twists UBR and UFR in opposite directions with zero permutation, op.40 swaps UF-UR plus UFR-UBR. Letters are Speffz and the direct target rows, edge D which is UL and corner V which is DFR, store the prose "apply swap directly, no setup" because no setup exists.

Decision recorded for op.39: no sourced algorithm twists the buffer corner directly, Pochmann's orient corners entries 1 and 2 twist UBR and DFR, the R2 method's own misoriented buffers per his text, entries 3 and 4 twist the U layer pair UBR and UFR in opposite directions, so entry 3 is stored verbatim as the buffer agnostic twist algorithm and its front_json states what it does rather than presenting a derived conjugation as sourced text.

Corroboration recorded for op.40: the alg is byte-identical to Tommy Cherry's parity sheet row UF-UR, UFR-UBR in ref/Tommy Cherry's 3BLD algs.xlsx, verified by direct sheet extraction, and that row's stated positions equal the simulated effect. Community tutorial text equals the world class solver's reconstructed execution, two independent chains.

Enum convention: CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN, CT_STAGE_PLL as the load time triple, the final stage mapping is decided at registry wiring.

Count: 40 rows, 8 core plus 16 edge letters plus 13 corner letters plus 1 parity plus 1 corner twist plus 1 bldtrainer parity.

Validation and coverage: playground/seeds_bld/bld_probe.c simulates every setup with the real engine. All 15 parsed edge setups land their letter sticker exactly on the U face sticker of UL, all 13 corner setups land exactly on the D face sticker of DFR, and every setup is self inverse. The letter cells come from ct_sticker_cell and therefore reverify that J Perm's lettering equals the shipped speffz.sql letter table. The setup table is partial by design, J Perm publishes setups for 30 of the 46 non buffer letters in his example solves and states the rest are intuitive, the remaining 16 letters are left for in app derivation or a future sourcing pass. Parser gaps: the U2' digit-then-prime tokens in the two Pochmann PLL rows closed with a125acc, pinned as op.04 and op.05 in tests/unit_notation.c, while bracket groups remain outside the token grammar and the probe strips them before parsing. d2 and d' wide turns parse. The special pass playground/parity_flip_twist/parity_flip_probe.c reverified op.08 live and end to end, it swaps UB with UL and UBR with UFR, oriented, centers fixed, odd permutation in both orbits, and replays J Perm's example solve 2 to a solved cube, scramble plus 13 edge swaps plus parity plus 9 corner swaps, output in sim_output.txt. op.39 and op.40 simulate pure and sticker exact respectively and are pinned grammar clean in tests/unit_notation.c.

## bld_m2.tsv

Source: https://www.speedsolving.com/wiki/index.php/M2, the M2/R2 page, archived at playground/seeds_bld/speedsolving_m2.html and speedsolving_m2.txt and refetched live at playground/parity_flip_twist/speedsolving_m2.md. The parity and buffer flip rows source elsewhere because the wiki page documents neither: https://www.youtube.com/watch?v=JTkSQxWk_u8, J Perm's M2 tutorial description, archived at playground/parity_flip_twist/jperm_m2_video.md, and http://www.cubefreak.net/bld/m2_guide.html archived at playground/parity_flip_twist/cubefreak_m2.md, with the base flip algorithm on http://www.stefan-pochmann.de/spocc/blindsolving/M2R2/ archived at playground/parity_flip_twist/pochmann_m2r2.md. Retrieved 2026-10-10.

Page contents: the M2 target table with the DF buffer, 19 standard targets, plus the additional M slice algs for FU, BU, and BD with several options each, the first listed option is stored per the convention. The r2 wing table on the same page is out of scope here, r2 solves 4BLD wings and belongs with the big cube packs. J Perm's M2 description lists two parity algorithms, option 1 M2 U' M U2 M U' M' U2 M' U2 and option 2 D' L2 D M2 D' L2 D, the first listed option is the row of record per the convention, with the usage claim to apply one after solving edges and before solving corners on an odd letter count, and his transcript states the effect, the M slice is off by one after odd edges and the parity algorithm leaves all edges solved except the two that Old Pochmann corner solving then swaps an odd number of times. cubefreak's guide states the M2 endgame rule, orient the flipped inactive edges and orient the DF edge in addition when their count is odd, and its example 2 applies F2 M' U M' U M' U2 M U M U M U2 F2, the base algorithm is Pochmann's orient edges entry, stating it leaves the (DF DR) 2-cycle with correct orientation.

Case key derivation: keys m2.01 to m2.22 in wiki table order followed by the three M slice specials, m2.23 is the parity companion, m2.24 is the buffer edge flip. front_json carries the buffer and target for target rows and the kind, with the DF buffer and flipped pair DF+UB for the flip.

Normalization: m2.24's source prints the step with hyphen separators and compact runs, F2-M'UM'UM'U2MUMUMU2-F2, stored expanded to spaced tokens, tokens untouched. m2.23 is stored verbatim as printed. Both are pinned grammar clean in tests/unit_notation.c.

Enum convention: CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_EP as the load time triple, final mapping at registry wiring.

Count: 24 rows, 19 standard targets plus 3 M slice specials plus 1 parity plus 1 buffer flip.

Validation and coverage: playground/seeds_bld/bld_probe.c applies every algorithm to the engine cube. All target algs parse and simulate sticker exact, the target sticker lands precisely on the D face sticker of the DF buffer. m2.15 LF and m2.22 BD contain L2' and M2' digit-then-prime tokens the parser rejected at capture time, the gap closed with a125acc, which also pins both rows canonicalized in tests/unit_notation.c. The rows are kept verbatim. The special pass playground/parity_flip_twist/parity_flip_probe.c verified both parity options end to end, J Perm's M2 example solve 1, scramble plus 11 targets plus parity, leaves the edges solved except exactly the (UL UB) pair equal to op.02's simulated edge side effect, corners untouched by the edge phase, centers solved. The flip base algorithm purely flips UB and UF, its F2 conjugate m2.24 flips UB and DF, the DF buffer itself, both with zero permutation and centers restored. cubefreak example 1 replays to fully solved edges and centers, example 2 replays to exactly the claimed (DF DR) 2-cycle with no flipped edges remaining. Source erratum recorded: cubefreak example 2's printed pair 4 uses the setup BL'B' where the page's own preparation table defines LD as BLB', and BL'B' is the table's LU, the simulation confirms the printed form cycles the UL piece instead of DL, the replay uses the table corrected form with the erratum printed by the probe, no shipped row is affected.

## fmc_concepts.tsv

Sources: https://www.speedsolving.com/wiki/index.php/Fewest_Moves_techniques, Premoves, Insertions, Skeleton, and Blockbuilding pages, all archived at playground/seeds_fmc/speedsolving_fmctech.html, speedsolving_niss.html (the Premoves page, NISS redirects there), speedsolving_insertions.html, speedsolving_skeleton.html, speedsolving_blockbuilding.html plus extracted .txt twins. Retrieved 2026-10-10.

Page contents: the six Fewest Moves techniques category pages. The pack seeds the three techniques named by the plan, blockbuilding, NISS, commutator insertions, plus the supporting concepts they depend on, premoves, reverse NISS, skeletons, domino reduction, and competition practice.

Case key derivation: keys fmc.01 to fmc.16, the alg column holds the concept answer text following the clock pack prose precedent, front_json carries the concept name. The insertion movecount row reproduces the wiki table credited to Sebastiano Tronto's FMC tutorial v3 page 33.

Enum convention: CT_EVENT_FMC, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT.

Count: 16 rows. The plan named no count for this pack.

Discrepancy: none. The wiki's fewest moves pages are prose technique descriptions, no algorithm tables exist to seed beyond the published example algorithms quoted inside the concept text.

## mbld_concepts.tsv

Sources: https://www.worldcubeassociation.org/regulations/ with the clauses of record H1a, H1a1, H1a2, H1b, 9f2, 9f2+, 9f12, 9f5 extracted verbatim to playground/seeds_mbld/wca_regulations.txt, https://www.speedsolving.com/wiki/index.php/Multi_BLD archived at playground/seeds_mbld/speedsolving_multibld.txt for the event flow, and https://www.speedsolving.com/wiki/index.php/BLD_Memorisation archived at playground/seeds_mbld/speedsolving_bldmemo.txt for the memorisation systems. The raw page HTMLs are not archived, the text extracts are the artifacts of record. Retrieved 2026-10-10.

Page contents: the WCA regulations define the attempt declaration rule, the time limit, 10 minutes times the number of puzzles below 6 puzzles, else 60 minutes, the scoring rule solved minus unsolved, and the seconds truncation with the judge recording solved, attempted, and time. The wiki memorisation page describes nine memorisation systems from direct visualisation to PAO.

Case key derivation: keys mbld.01 to mbld.14, mbld.01 to mbld.05 are the rule rows with the regulation tag in front_json, mbld.06 to mbld.14 are the memo rows with the memo tag. The alg column holds the rule or method answer text.

Enum convention: CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT, CT_STAGE_CONCEPT. Per the plan multi BLD reuses the 3BLD pack for solving content, this pack seeds only the distinct method rows.

Count: 14 rows. The plan named no count for this pack.

Discrepancy: the wiki multi blind page states the time limit as 10 minutes per cube and a maximum of 60 minutes total, the regulations text is the source of record and carries the below 6 puzzles condition, the seeded row quotes the regulation wording.

