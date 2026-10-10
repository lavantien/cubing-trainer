# adversarial review record, M0 plus M1, 2026-10-10

Two independent agents attacked M0 and M1 blind to each other. Reviewer A probed from the engine outward, reviewer B rederived oracles from the domain facts and attacked the boundaries. Both verdicts: the M1 engine core meets the correctness bar, the M0 chain gates do not at the reviewed HEAD. Probes and logs are committed under playground/adv_a/ and playground/adv_b/.

## findings

| id | reviewer | severity | location | finding | owner | status |
| --- | --- | --- | --- | --- | --- | --- |
| A-D1 | A | defect | tests/golden/PROVENANCE.md:12-13 | 2 of 4 sha256 records are typoed, 63 and 65 hex chars, goldens themselves regenerate byte identical | m7-gates-sweep | fixed in 6162db1 |
| A-D2 | A | defect | tools/visual_check.py | equal area face color swaps pass, hamming 0 and mean delta 0, red and green swap invisible in solved scenes | queued, targets slot | fixed in 33085a1 |
| A-D3 | A | defect | src/srs/fsrs.c:95 | grade 0 indexes w[-1], out of bounds read, traps under test flags, grades 5 to 9 read in bounds nonsense | m-recon-1 | fixed in eba3268 |
| A-D4 | A | defect | src/srs/fsrs.c:73-116 | negative elapsed yields NaN stability through clamps into interval, traps | m-recon-1 | fixed in eba3268 |
| B-F1 | B | high | src/srs/fsrs.c, src/trainer/session.c:222 | same NaN class with reachability: clock setback or edited DB makes elapsed negative, NaN binds NULL, NOT NULL rejects, card permanently ungradeable, sub NaN band silently shrinks stability | m-recon-1 | fixed in f5d6ac5 and f712867 |
| A-D5 | A | defect | repo wide | make lint red, 28 stray constants plus 11 to 12 tidy narrowing, make fmt-check red, 300 plus violations across 23 files | m7-gates-sweep | fixed in 9fef988 and 71491b2 |
| B-F2 | B | medium | renderer.h renderer.c anim.h anim.c headless.h headless.c | same lint failure list, M1 files landed literals outside the config hub | m7-gates-sweep | fixed in 9fef988 and 71491b2 |
| B-F3 | B | medium | src/app/app_loop.c:30, screen_review.c:96 | settings merge does not exist, saved retention and anim speed are dead, CT_RENDERER_MOVE_MILLIS 350 disagrees with CT_ANIM_MOVE_SECONDS 250 | queued, startup slot | fixed in 0730ead |
| B-F4 | B | medium low | Makefile:101-186 | build-headless, e2e, visual, db-verify are not-implemented stubs, headless exe built by an ad hoc command | queued behind m6-mutate | fixed in 6162db1 |
| A-G1 | A | gap | src/engine/transform.c:96-97 | ct_transform_simplify treats cap as a stack bound not an output bound, simplify(U R R', 1) fails though the result is 1 move | queued, engine slot | fixed in 8f92dfe |
| A-G2 | A | gap | src/storage/migrate.c | no gap or continuity detection, embedded NUL truncates silently and records applied, duplicates both apply, saturation skipped | m-recon-1 | fixed in 7e44c9b |
| B-F5 | B | low | src/storage/migrate.c:16-36,179-246 | version 0 never runs, strtoul saturation silent, BOM aborts leaving the app on an empty schema | m-recon-1 | fixed in 7e44c9b |
| A-G3 | A | gap | Makefile, PROVENANCE.md:16 | visual regeneration not reproducible, recorded build line exists nowhere | queued behind m6-mutate | fixed in 6162db1 and b1083ce |
| A-G4 | A | gap | tools/memguard.ps1:22-31 | short path processes missed, command line path mentions falsely counted | m7-gates-sweep | fixed in 848c00a |
| A-G5 | A | gap | src/app/app_loop.c:48, ct_mkdirs:13 | migrations dir is cwd relative, LOCALAPPDATA unset never creates the data dir, app runs db-less | queued, startup slot | fixed in de06402 |
| B-F6 | B | low | src/config/config.c:24-50,166-274 | preferred_moves empty string zeroes the mask, rotation bits accepted, key aliases bind via strtol space skip | m7-gates-sweep | fixed in 750798b and a438db0 |
| B-F7 | B | info | src/trainer/session.c:102-120 | iso_add_days copies garbage suffixes verbatim, year 9999 wraps to a rejected 5 digit year | m-recon-1 | fixed in f712867 |
| A-G6 | A | observation | src/engine/notation.c:20 | n=1 accepted by the parser while ct_cube_init clamps to n at least 2, deliberate, goldens cover it | docs note | recorded in ab19af0 |
| A-G7 | A | observation | renderer.c:12-19 vs cube.c:11-18 | frames table duplicated, divergence risk, vendor hash covers sqlite only | queued, engine slot | fixed in 2f659e0 and f72e4cf, baseline probe c9389a1 |

## what survived both attackers

Independent differential implementation of ct_apply agrees for every n in 2 to 7, every face, span, and quarter. Physical cubing oracles match on all 6 faces. Group identities hold, (R U)^105, (R U R' U')^6, (M E S)^12, wide inverse depth identities. 126 adversarial notation checks and 11662 transform checks clean under trap sanitizers. All 28 FSRS goldens rederived to diff 0.0 by two independent derivations. Renderer winding and slab membership exact for 22 probed move classes, max position error 2.24e-07. Civil date arithmetic exact over every valid date 0001 to 9999 against an independent leap oracle. Speffz letter scheme derived independently from the README rule, 0 mismatches against the seed in both orbits, buffers confirmed. Storage parameterized everywhere, no SQL concatenation, LIKE and GLOB metacharacters stay exact, rollback atomicity holds.

## disposition

Every confirmed finding is routed to an owning slot. After the fix wave lands: re-run fmt, lint, analyze, full test, fuzz, and visual with the hardened comparator, then M0 and M1 close together. M3 signoff additionally gates on the fsrs cluster B-F1, A-D3, A-D4.

The fix wave landed 2026-10-10. Every defect and gap above carries its landing commit in the status column, A-G6 is recorded as a deliberate domain note in the README, and A-G7 closed in two halves, the frames dedup in 2f659e0 and the vendor hash extension to raylib and raygui in f72e4cf. The reviews also triggered follow-on fixes outside the table: 2cadaf9 zeroes the review form before use instead of freeing a garbage session, c1b91f9 drops rotation letters from the preferred moves staging and grid, 1437dab raises the test suite registry capacity after the suite count outgrew it, and the visual gate surfaced machine-level nondeterminism where golden_mid_3 flips between two byte-stable rasterization outputs independent of source and binary, gated by b1083ce with per-run self-agreement, a lock against concurrent runs, and split flake versus regression exits, with the drifted sample preserved in 76cb636 and the phenomenon recorded in tests/golden/PROVENANCE.md and playground/m7_visual/.
