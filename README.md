# cubing-trainer

Speedsolving trainer for all WCA events. This README is the source of truth for the technical tables, the architecture, and the diagrams.

## Overview

cubing-trainer trains algorithms across all WCA events through flashcards with spaced repetition, auto-animates any algorithm on a 3D cube, and auto-generates 3-style commutator algorithms with structural classification, constrained to a preferred move subset and scored for fingertrick friendliness, including full floating-buffer mode.

The stack is raylib 6, C23, and SQLite, built with clang 23 targeting mingw-w64 over the winlibs sysroot, driven by GNU make.

The 3-style generator is the distinguishing feature. It emits commutators labeled by structure (setup, interchange, insertion, pure, A9, B9, and the rest of the BH taxonomy), searches under a preferred move subset that defaults to R, U, D, and runs in fixed-buffer or floating-buffer mode.

## Build and make targets

Prerequisites, verified on this machine 2026-10-09: LLVM clang 23.1.3, winlibs UCRT gcc 16.2.0 as the mingw-w64 sysroot, GNU make 4.4.1, cmake, ninja, sqlite3 CLI 3.53.4, python 3 for the vendor, db-verify, and visual targets, and node with npx and mmdc (mermaid-cli). No MSYS2, no pkg-config.

Run `make vendor` first. It HEAD-checks each URL, downloads, extracts the pinned third party sources under third_party/, and verifies every archive by SHA3-256 through tools/vendor_verify.py: the raylib 6.0 and raygui 5.0 tag tarballs and the SQLite 3.53.4 amalgamation zip. Each stamp recipe verifies before extracting and the vendor target re-verifies the kept archives in build/vendor on every run, so a corrupted or truncated download fails the target and needs build/vendor removed to re-download. The zip extracts through python's zipfile module instead of tar because GNU tar on the make shell cannot read zip while the Windows bsdtar can, which made the old tar line depend on which tar won PATH order. raylib compiles from the flat third_party/raylib/src/*.c set of 7 TUs with -DPLATFORM_DESKTOP, a recursive glob over-collects platform files and breaks on duplicate symbols. raygui 5.0 ships only raygui.h, so the Makefile generates build/raygui_impl.c holding RAYGUI_IMPLEMENTATION plus the include as its single TU. sqlite3.c compiles with -Wno-error for two unused parameters in its Windows VFS code.

The toolchain spelling is decided, evidence in playground/spike_link/REPORT.md. Compile lines carry `--target=x86_64-w64-mingw32` alone and link lines add `-fuse-ld=lld`, which resolves crt2.o, crtbegin.o, and the gcc 16.2.0 library directories from the winlibs install. The `--gcc-install-dir` spelling is inert on clang 23.1.3, fails under -Werror as an unused argument, and is not used. One environment hazard: this account exports C_INCLUDE_PATH and CPLUS_INCLUDE_PATH at a foreign gcc, so the Makefile clobbers both to empty for every recipe. The link set for anything touching raylib is -static-libgcc -lopengl32 -lgdi32 -lwinmm, and those Windows libraries appear on no other link line.

| target | what it does |
| --- | --- |
| vendor | download, HEAD-check, SHA3-256 verify, and extract raylib 6.0, raygui 5.0, and the sqlite amalgamation into third_party/, re-verifying the kept archives every run |
| vendor-verify-selftest | exercise the archive hash checker against synthetic matches, corruptions, and malformed hashes |
| build | compile app, raylib, raygui, and sqlite sources and emit build/bin/cubing-trainer.exe |
| build-headless | compile tools/render_scene.c against the full library set and emit build/bin/render_scene.exe |
| spike | build and run the M0 toolchain spike from playground/spike_link |
| test | run the full unit suite |
| test-\<suite\> | run suites whose name matches the filter |
| fmt | format the tree |
| fmt-check | check formatting without writing |
| lint | clang-tidy plus the -Wall -Wextra -Werror -Wpedantic -std=c23 gate plus zero-comment and stray-constant checks |
| analyze | run clang --analyze over the sources |
| fuzz | run the 16 in-process fuzz suites, CT_FUZZ_LOOPS multiplies the iteration count |
| fuzz-\<suite\> | run one fuzz suite |
| mutate | run the token mutation harness over the SCOPE directory or file with the optional SUITE filter |
| e2e | run every tests/e2e/*.txt through cubing-trainer.exe --headless --script |
| visual | render the 4 golden scenes fresh into build/visual and compare against tests/golden through tools/run_visual.py and tools/visual_check.py |
| db-verify | build a migrated DB through tools/make_db.c, then check integrity, user_version, and 35 schema assertions through the sqlite3 CLI |
| diagrams | render every docs/diagrams/*.mmd to PNG through tools/gen_diagrams.mjs |
| memguard | run tools/memguard.ps1 to enforce the combined working set cap on project processes |
| memguard-selftest | verify memguard process matching against a spawned sleeper exe |
| run | build then launch the trainer window |
| release | assemble build/release/cubing-trainer, zip it as build/cubing-trainer-\<git-describe\>.zip, and verify layout, zip bytes, a headless run, and fatal exits through tools/release_verify.py |
| release-verify-selftest | exercise the release verifier's layout, zip, and database assertions against synthetic trees |
| clean | remove build outputs |

make build-headless emits build/bin/render_scene.exe, the headless scene renderer that make visual drives. The e2e script runner is the one pending piece in this set: while tests/e2e holds no script cases, make e2e exits 1 with a message naming the pending runner, so the gate fails honestly.

make release packages the trainer as build/cubing-trainer-\<git-describe\>.zip holding one cubing-trainer folder with dist/bin/cubing-trainer.exe plus db/migrations and db/seeds/packs. The exe resolves both resource trees exactly two directories up from its own location through the CT_MIGRATIONS_EXE_REL and CT_SEEDS_PACKS_EXE_REL defaults in the config hub, so it sits under dist/bin, the same two-level shape as build/bin inside the repo, and any shallower or deeper placement breaks both resolutions. tools/release_verify.py compares the packaged file sets against the repo trees, compares every zip entry byte for byte, runs the packaged exe headless from a foreign working directory against a fresh LOCALAPPDATA and asserts the database migrates to the latest user_version and event_pack holds all 574 seed rows, then asserts the two fatal paths, LOCALAPPDATA unset and a missing migrations directory, both exit 1 with their startup errors.

make visual locks against concurrent runs through build/visual.lock and, for the animated mid scene, renders twice per gate and requires the pair to self-agree before comparing against the committed golden, retrying once on disagreement. Persistent pair disagreement exits 3 as an infra flake, self-agreement that mismatches the golden exits 2 as a regression signal with evidence copied under build/visual_evidence. This split exists because golden_mid_3 flips between two byte-stable rasterization outputs on this machine on a minutes timescale, independent of source and binary, while the static scenes never flip; the phenomenon, proof, and drifted sample are recorded in tests/golden/PROVENANCE.md and playground/m7_visual/.

## Architecture

Module tree:

```
Makefile README.md
db/migrations/001_init.sql  db/seeds/speffz.sql  db/seeds/speffz_source.md  db/seeds/packs/*.tsv (16 seed packs)  db/seeds/packs/PROVENANCE.md
docs/diagrams/*.mmd  docs/diagrams/*.png (generated)  docs/reviews/*.md
src/config/config.h config.c
src/engine/cube.h cube.c notation.h notation.c transform.h transform.c geometry.h geometry.c geometry_sq1.h geometry_sq1.c geometry_clock.h geometry_clock.c
src/render/renderer.h renderer.c anim.h anim.c
src/ui/ui.h widgets.h widgets.c screen_menu.c screen_cards.c screen_review.c screen_settings.c screen_stats.c
src/storage/db.h db.c migrate.c repo_cards.c repo_reviews.c repo_settings.c repo_stats.c repo_algs.c
src/srs/fsrs.h fsrs.c
src/trainer/session.h session.c cfop.h cfop.c selector.h selector.c
src/gen/targets.h targets.c interchange.h interchange.c synth.h synth.c score.h score.c classify.h classify.c
src/stats/stats.h stats.c
src/packs/packs.h packs.c provenance.h provenance.c seed_registry.h seed_registry.c
src/app/main.c app.h app_loop.c headless.h headless.c paths.h paths.c
tools/gen_diagrams.mjs run_lint.ps1 run_analyze.ps1 check_zero_comments.ps1 check_stray_constants.ps1 stray_constants_allow.txt memguard.ps1 memguard_sleeper.c mutate.c make_db.c db_schema_check.sql db_verify.py render_scene.c run_visual.py visual_check.py visual_selftest.py
tests/framework.h framework.c test_main.c unit_*.c fuzz_engine.c fuzz_srs.c fuzz_db.c fuzz_config.c fuzz_main.c fixtures/ golden/ mutant_allowlist.txt
```

Landed since the last sync: structural classification in src/gen (85f18e1), the startup settings merge (0730ead), exe-relative path resolution (de06402), and startup seeding of the registered event packs (f0e8db9). Queued: the e2e script runner.

Layering, enforced by review: config depends on nothing. engine, srs, and storage depend on config. gen and render depend on engine. trainer depends on storage and srs. ui depends on trainer, render, and storage. app depends on ui. No upward or lateral edges.

Two patterns keep the code testable without a window. UI screens split a pure model from a thin draw layer: ct_cards_form and ct_settings_form in src/ui/ui.h carry form state and staged mutations with no raylib types, covered by tests/unit_cards_form.c, tests/unit_review_form.c, and tests/unit_settings_form.c, while the screen update and draw functions only translate app state into calls. The renderer splits the same way, ct_renderer_quads emits the sticker quad list as plain data and ct_renderer_draw is the only function that builds raylib meshes. ct_renderer_draw takes the animation duration as move_seconds (40c5a86): the headless golden path pins CT_RENDERER_MOVE_MILLIS from the config hub for deterministic output, while the review screen feeds the user's anim.move_seconds setting through ct_anim_update into the same draw call end to end.

raylib 6 notes from the renderer work: matrix types and math live in raymath.h and the mesh buffer layout lives in rlgl.h, whose stream order renderer.h records as position 0 and color 3. The draw path fills mesh.vertices and mesh.colors and hands them to UploadMesh, then UnloadMesh frees the CPU-side arrays along with the GPU upload, so the dynamic cube mesh is rebuilt and re-uploaded every frame, which stays cheap at 6 * n * n quads and keeps animation out of GPU lifetime management. LoadMaterialDefault is the unlit vertex color path, which is what flat sticker quads need.

The config hub is one header, src/config/config.h, holding ct_config, every enum (grade, orbit, buffer_mode, classification, event, method, stage, direction, ui_screen, ease, config status), and every tunable with its default: FSRS weights, requested retention, interval clamps, preferred move subset and per-face penalties, regrip and fingertrick weights, animation durations and easing, window defaults, DB filename, generator search bounds, and fuzz iteration counts. Face letters map to single bit flags through ct_move_face, CT_MOVE_BIT_U through CT_MOVE_BIT_Z covering faces, slices, and rotations, so a preferred subset is one unsigned mask and per-face penalties index by bit position. The hub is loaded once at startup through ct_config_defaults and passed by pointer, and user settings merge over the defaults through ct_config_set, which parses dotted setting names like srs.request_retention, gen.preferred_moves, and gen.penalty_R with range clamps and rejects unknown names. The lint target fails on numeric literals in src/ outside config.h, inline test tables are exempt, and values that recur across test tables get centralized too.

Path resolution landed in src/app/paths.c (de06402). The data directory is %LOCALAPPDATA%/cubing-trainer, read from the environment at runtime and created with mkdir semantics when missing, and an unset LOCALAPPDATA or uncreatable directory aborts startup instead of running db-less (79ffeb5). The migrations directory comes from the CT_MIGRATIONS_DIR environment override when set, else from the executable location at ../../db/migrations, so the installed exe finds the schema next to itself regardless of the working directory. Event pack seeds resolve the same way at ../../db/seeds/packs (CT_SEEDS_PACKS_EXE_REL, f0e8db9), and an override path too long for the buffer is reported at startup instead of being silently truncated (5291338).

## Cube engine

World axes are x right, y up, z front. Each of the 6 faces carries a (normal, row, col) frame in src/engine/cube.c, and a sticker center is normal * (n-1) plus row and col offsets where grid position i maps to the coordinate 2*i - (n-1), so centers lie on the integer range -(n-1) to n-1 with stride 2 and geometry, moves, and letters share one addressing scheme. A move selects its slab by the dot product of the sticker center with the face normal against the depth range, then rotates centers and normals by steps = -sign * quarter right hand quarter turns about the face axis, where sign is the normal's sign on that axis.

Sticker indexing for BLD work runs 0 to 23 per orbit: face order U L F R B D, corner sticker index is face * 4 + p with p running clockwise from (0, 0), edge sticker index is face * 4 + p with p running clockwise from the (0, 1) edge.

src/engine/transform.c rewrites move sequences as algebra: inverse, mirror, rotate_y, conjugate, and simplify. Mirror swaps R and L and inverts quarter counts, rotate_y cycles the side faces around U for k = 1, 2, 3. Simplify greedily folds adjacent moves that share a face and depth span, merging quarter counts modulo 4 and dropping zeros. Its cap bounds the returned sequence: the call fails only when the folded result exceeds cap or the input exceeds CT_TRANSFORM_ASSEMBLY_MAX, the assembly buffer that bounds intermediate work, and out is left untouched on failure. Both mirror and rotate_y carry simulation oracles in tests/unit_transform.c: mirroring the cube state after an algorithm equals applying the mirrored algorithm to the mirrored solved cube, and appending a y rotation to an algorithm equals applying the rotated algorithm after the y rotation.

src/engine/geometry.h landed the puzzle driver vtable ct_geometry in 2e22289: sticker_count, move_count, move_facelets, sticker_geometry, turn_axis, and move_from_token, all keyed by n, with ct_geometry_apply composing decode and apply. The landed nxn driver addresses moves as depth pairs, each id decodes to face, depth_from, depth_to, and quarter over every contiguous span inside the outer half of the cube, and move_count(n) is 18P with P = half(half + 1) / 2 and half = (n + 1) / 2, so 54 moves at n = 3 and 180 at n = 7. Whole-cube rotations span depth 1 through n and encoding rejects spans deeper than half, so rotations sit outside the id space by design. move_facelets writes the permutation into uint8_t values, so the compact convention caps at n = 6 (CT_GEOMETRY_PERM_MAX_N) while counts, decoding, sticker geometry, and turn axes stay valid to n = 7. tests/unit_geometry.c covers decode and encode round trips, counts, permutations, sticker geometry, turn axes, token encoding, and guard paths.

Two single-puzzle drivers joined the vtable in 19fedfc, both valid only at n = 1 (CT_GEOMETRY_SINGLE_N). The square-1 driver models 48 fixed slots over the 2x12 unit rings, one side slot per 30 degree unit plus one face anchor per piece, with 144 move ids covering the 143 WCA (x,y) twist pairs in [-5,6] excluding (0,0) plus the slash at id 143. Twists are pure shape-independent permutations, the slash is legal only when both rings carry piece boundaries at the cut, so the pure move_facelets entry returns CT_GEOMETRY_E_STATE for the slash while the state-aware move_facelets_state entry, the vtable addition the nxn and clock drivers leave null, checks legality against the ring state. Piece orientation carried by slash flips is outside the permutation abstraction and not modeled, the recorded gap. The clock driver holds the official 18 hand positions mod 12 with the regulation pin rule vectors from the WCA scrambler, 109 move ids from 9 wheel names UR DR DL UL U R D L ALL times 12 canonical amounts plus y2 at id 108. Wheel turns change hand angles and never permute positions, so move_facelets returns CT_GEOMETRY_E_PERM for them and serves the y2 permutation only. Model conventions plus the WCA regulation and official scrambler provenance live in tests/fixtures/wca_sq1_clock_notation.md, coverage in tests/unit_geometry_sq1.c and tests/unit_geometry_clock.c.

## Notation grammar

A move is ct_move: face, depth range, quarter count. The landed parser and formatter in src/engine/notation.c cover this token grammar:

| token element | forms | notes |
| --- | --- | --- |
| digit prefix | optional leading digit 2 to 7 | inner layer depth on big cubes |
| face | U D L R F B | outer layer turn at the given depth |
| wide | w suffix, or a lowercase face | layers 1 through the digit, 2 when no digit |
| slice | M E S | single middle layer at depth (n + 1) / 2, M follows L, E follows D, S follows F |
| rotation | x y z | whole cube, depth 1 through n, x follows R, y follows U, z follows F |
| prime | ' | counterclockwise, quarter 3 |
| half turn | 2, '2, and 2' | 180 degrees, quarter 2, both suffix orders parse, the prime on a half turn is absorbed because a half turn is its own inverse, formatting writes 2 |

Canonical formatting rules: a single inner layer spells the digit prefix only above depth 1, a wide turn spells Rw at depth 2 and 3Rw above, a wide spanning every layer collapses to the rotation letter so 3Rw formats as x on n = 3, and the middle layer spells as its slice letter. Rotations parse and format as whole cube turns, depth 1 through n. Parse failures return the CT_NOTATION_E_* codes: input, token, suffix, digit, depth, cap, and move.

Half turn suffix order: WCA regulation 12a defines the prime forms as counterclockwise 90 degree turns and the trailing 2 as 180 degrees, and the counterclockwise 180 equals the clockwise 180, so U2', U'2, and U2 denote one move. The regulation spells only letter, prime, and trailing digit forms, the digit then prime spelling comes from community sources, verified 2026-10-10: https://www.speedsolving.com/wiki/index.php/M2 prints L2' in the LF row and M2' in the BD section, and http://www.stefan-pochmann.de/spocc/blindsolving/3x3/old.php prints U2' in its PLL scripts.

Per-event token tables: square-1 slash notation and clock turns parse through their geometry drivers' move_from_token (19fedfc), restricted to the WCA scramble grammar, strict (X,Y) integer pairs and / for square-1 and the canonical pin plus amount forms and y2 for clock, while site notations beyond that grammar such as Sarah's (U) setup markers, trailing * markers, and cubingapp's paren-less 3,3 form are rejected. Megaminx and FTO face tables remain planned and outside the parser.

Round-trip properties are a fuzz surface: format(parse(s)) normalizes, and parse(format(a)) == a.

## Commutator classes

Structural classification of 3-style commutators landed in src/gen/classify.c (85f18e1). The class set is the ct_classification enum in the config hub, and ct_classify_alg returns a ct_classify_result decomposition struct: class label, setup, interchange, and insertion move spans, insert_first and cyclic flags, move and qtm counts, a canonical label, and a rationale string naming the recognition logic, so user-entered algorithms decompose the same way as generated ones. Class definitions and example rows are pinned in tests/fixtures/bh_classification.md, sourced from the speedsolving wiki Beyer-Hardwick page and the speedcubing.com/chris bhcorners and bhedges pages:

| orbit | class | moves |
| --- | --- | --- |
| corner | pure | 8 |
| corner | A9 | 9 |
| corner | orthogonal | 10 |
| corner | cyclic shift | 10 |
| corner | column | 11 |
| corner | per special | 12 |
| edge | half slice-plane | 4 |
| edge | slice-plane SP5, SP7, SP9 | 5, 7, 9 |
| edge | pure | 8 |
| edge | A9 | 9 |
| edge | B9, edges only | 9 |
| edge | orthogonal | 10 |

Cyclic shift count discrepancy, recorded in the fixture: every bhcorners.html cyclic shift row and the A B A' C B C' formula give 10 moves, while the speedsolving wiki page text and the earlier plan say 11, and the fixture follows the algorithms at 10. Alternate labels (perpendicular, collision, simultaneous) ship as display aliases, the canonical labels are the fixture table. The classifier also labels parity, edge flip, and corner twist cases outside this table, and tests/unit_classify.c covers labels, aliases, the 21 fixture rows, synth table agreement, and stress rounds over random sequences.

## Case tables and letters

With a fixed buffer there are 378 corner targets (21x18) and 440 edge targets (22x20), per the BH wiki infobox. The counts are asserted in tests/unit_targets.c against ct_targets_corner and ct_targets_edge, with buffers pinned at corner sticker UBL = 0 and edge sticker UF = 2.

Speffz letter scheme summary: face order U L F R B D, each face lettered clockwise from its start corner, so U=A-D, L=E-H, F=I-L, R=M-P, B=Q-T, D=U-X with D read after x2. Corner buffer UBL = A, edge buffer UF = C. The app rule is one line, sticker index i maps to letter 'A' + i in both orbits. Verification is recorded in db/seeds/speffz_source.md against the speedsolving wiki Speffz page and bldtrainer.com, whose glossary pins the buffer letters. blddb.net was the plan's preferred fixture source but its pages are a JavaScript application with no textual letter table, so bldtrainer.com served as the independent reference. The scheme is user-editable data in SQLite, seeded by db/seeds/speffz.sql, and tests/unit_letter_scheme.c asserts the C seed and the SQL file produce identical maps.

CFOP case tables landed in src/trainer/cfop.c with counts asserted in tests/unit_cfop.c: PLL 21 (Aa Ab E F Ga Gb Gc Gd H Ja Jb Na Nb Ra Rb T Ua Ub V Y Z), OLL 57 as oll.01 through oll.57, and F2L 41 per slot across the fr, fl, br, and bl slots for 164 cards. Source URLs for the 3 counts sit in the source records table.

App definition, floating buffer mode: fixed-buffer mode generates 3-cycles that include the configured buffer sticker, which gives the 378 and 440 tables. Floating mode may emit any 3-cycle (x, t1, t2) that solves the target pair, since the buffer drifts during a real solve. The case table stays at 378 and 440, and the admissible solution space grows about 24x.

## Event packs

17 official events as of 2026-10-09. FTO joins officially on 2027-01-02 and clock retires after Worlds 2027, both per the WCA post linked under source records. The packs storage layer landed in src/packs with a hand-rolled SHA-256 over a canonical row form feeding the content_hash column, asserted in tests/unit_packs.c. Seed capture is done: 16 TSV packs hold 574 rows in db/seeds/packs/, the seed registry in src/packs/seed_registry.c binds each file to its event, method, and stage triple, and startup seeds every registered pack (f0e8db9). The startup contract is degraded-not-fatal for seeds: a missing seeds directory or an incomplete pass logs to stderr and the app continues, while a failed database open, migration, or settings merge aborts startup (79ffeb5). The per-event method plan of record:

| event | method | seed source approach | status |
| --- | --- | --- | --- |
| 2x2 | CLL | 42 algorithm CLL set | curated |
| 3x3 | CFOP | starts empty, the user enters cards | user entered |
| 3x3 OH | CFOP variant | CFOP decks carry the OH variant field | user entered |
| 3x3 FMC | concept cards | blockbuilding, NISS, commutator insertions | curated |
| 3BLD | 3-style | generated by the app, with Old Pochmann and M2 fallback seeds | generated |
| 4BLD | 3-style | generated 3-style plus U2 and r2 center and wing seeds | generated and curated |
| 5BLD | 3-style | generated 3-style plus U2 and r2 center and wing seeds | generated and curated |
| multi-BLD | 3-style | reuses the 3BLD pack | generated |
| 4x4 | Yau | Yau plus parity algorithms | curated |
| 5x5, 6x6, 7x7 | reduction | reduction plus parity algorithms | curated |
| clock | standard | about 14 algorithms | curated |
| megaminx | 4LLL | 4 look last layer | curated |
| pyraminx | L4E | L4E with LBL fallback | curated |
| skewb | Sarah intermediate | Sarah intermediate set | curated |
| square-1 | cubeshape and last layer | cubeshape, CO, EO, CP, and EP sets | curated |
| FTO | L4T Redux | L4T Redux | curated |

Landed pack inventory, summary level, with per-pack row counts asserted at startup in tests/unit_pack_startup.c:

| pack | event | method | stage | rows |
| --- | --- | --- | --- | --- |
| twoby2_cll.tsv | 2x2 | CLL | CLL | 42 |
| skewb_sarah.tsv | skewb | Sarah intermediate | Sarah | 15 |
| pyraminx_l4e.tsv | pyraminx | L4E | L4E | 35 |
| megaminx_4lll.tsv | megaminx | 4LLL | 4LLL | 39 |
| sq1_cubeshape.tsv | square-1 | LBL | cubeshape | 169 |
| sq1_co.tsv | square-1 | LBL | CO | 7 |
| sq1_eo.tsv | square-1 | LBL | EO | 7 |
| sq1_cp.tsv | square-1 | LBL | CP | 16 |
| sq1_ep.tsv | square-1 | LBL | EP | 99 |
| clock_algs.tsv | clock | LBL | concept | 14 |
| bigcube_parity.tsv | 4x4 | Yau | parity | 5 |
| bld_m2.tsv | 3BLD | M2 | concept | 22 |
| bld_oldpoch.tsv | 3BLD | Old Pochmann | concept | 38 |
| fmc_concepts.tsv | FMC | concept | concept | 16 |
| fto_l4t_redux.tsv | FTO | L4T Redux | L4T | 36 |
| mbld_concepts.tsv | multi-BLD | 3-style | concept | 14 |

16 packs, 574 rows total. The square-1 pack ships as 5 per-stage files because the loader binds one stage per file. Events still without seed rows by design: 3x3 and 3x3 OH are user entered, 3BLD 3-style and the 4BLD and 5BLD core are generated by the app, and the 5x5 through 7x7 reduction algs remain to source. Sources, retrieval dates, case key derivations, and per-pack discrepancies are the detailed record in db/seeds/packs/PROVENANCE.md. Every curated seed row records its source URL, retrieval date, and content hash, and the selector exposes the seeded packs for study, square-1, clock, and parity among them (e75a3f5, tests/unit_selector_packs.c).

## Database schema

The SQLite amalgamation is compiled into the exe. The DB lives at %LOCALAPPDATA%/cubing-trainer/trainer.db. Forward-only migrations in db/migrations/NNN_*.sql self-apply at startup inside a transaction, the version sits in PRAGMA user_version, and a pre-migration backup goes to trainer.db.bak. The landed migration is db/migrations/001_init.sql with all 9 tables and 5 indexes below. The repo_*.c files hold prepared-statement CRUD, 1 file per aggregate, repo_cards, repo_reviews, repo_settings, and repo_stats so far. DDL sketch:

```
settings(key, value)
letter_scheme(id, name, is_default)
letter_map(scheme_id, orbit, sticker, letter)
flashcard(id, deck, case_key, direction, front_json, alg_3x3, alg_big, alg_oh, notes, created_at, updated_at, unique(deck, case_key, direction))
srs_state(card_id PK, stability, difficulty, due, last_review, reps, lapses, state)
review_log(id, card_id, rating, reviewed_at, stability, difficulty, elapsed_days, scheduled_days)
generated_alg(id, orbit, target_pair, buffer_mode, category, sequence, classification, setup, interchange, insertion, qtm, score, score_parts_json, params_hash, created_at, unique(orbit, category, target_pair, buffer_mode, params_hash))
session_log(id, deck, started_at, ended_at, n_cards, n_again, n_hard, n_good, n_easy)
event_pack(id, event, method, stage, case_key, alg, front_json, source_url, source_at, content_hash, unique(event, method, stage, case_key))
```

The flashcard table carries the CFOP variant fields through alg_3x3, alg_big, and alg_oh.

## FSRS scheduler

FSRS-5 with default parameters, no ML fitting initially. An FSRS-6 upgrade later is additive: 2 weights and 2 branches. Spec source: https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm

Default weights w0 to w18:

```
[0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046, 1.54575, 0.1192, 1.01925, 1.9395, 0.11, 0.29605, 2.2698, 0.2315, 2.9898, 0.51655, 0.6621]
```

Formulas:

```
S0(G) = w[G-1]
D0(G) = w4 - e^(w5*(G-1)) + 1
R(t, S) = (1 + 19t/(81S))^(-0.5)
I(r, S) = (S/FACTOR) * (r^(1/DECAY) - 1)
S_same_day = S * e^(w17*(G-3+w18))
```

The difficulty update is damped with mean reversion to D0(4), the recall and forget stability updates follow the wiki, and DECAY and FACTOR are the wiki constants. Clamps are recorded design choices: difficulty D in [1, 10], stability S in [0.1, 36500] days. The input domain contract, landed in f5d6ac5: negative elapsed days clamp to 0 inside ct_fsrs_retrievability, and due_day saturates at LONG_MAX when today plus the interval would overflow instead of wrapping. Same-day reviews apply the same-day stability formula and leave difficulty unchanged. Golden unit tests derive expected values by hand from the wiki formulas before implementation.

The C surface sits in src/srs/fsrs.h: ct_grade (again 1, hard, good, easy), ct_fsrs_card, ct_fsrs, and the functions ct_fsrs_init, ct_fsrs_review, ct_fsrs_retrievability, ct_fsrs_next_interval_days.

## Generator

src/gen/ runs this pipeline per case. Landed so far: target enumeration and interchange detection, template synthesis with setup search and fallback (3179098), scoring with the regrip model (0db720d), the generated_alg cache with params hash invalidation (576a526), and structural classification (85f18e1), so every step below is implemented:

1. Enumerate targets through the active letter scheme: 24 corner and 24 edge stickers, the 378 corner and 440 edge ordered pairs in fixed-buffer mode, and parity, flip, and twist categories from small tables.
2. Detect interchanges: move m interchanges stickers (x, y) iff m(x)==y and m(y)==x on the sticker permutations. The landed candidate set in ct_candidate_moves enumerates every face, every slab span contained in the outer half of the cube since deeper spans are complements of shallower ones, and quarters 1 to 3, which is 54 moves on n = 3 and 180 on n = 7, kept when the computed permutation swaps the pair.
3. Propose insertions from templates: 1-move, 3-move X Y X' over face pairs, and 5-move per special. Simulation disposes: a candidate insert is admitted only if it moves exactly 1 of the interchange pair's pieces into the interchange layer.
4. Synthesize: BFS setups S over the allowed subset (default R, U, D) to the config depth bound, keeping only setups that create an interchange among cycle members. Assemble [S: [A, I]], simplify, simulate on a solved cube, validate that the resulting sticker cycle equals the target exactly including orientation, then score and keep the best. Template misses fall back to bidirectional BFS over the subset to depth 5 per side, which covers optima to 10 moves, with a full-metric straggler path of 6 per side.
5. Classify: structural rules label the sequence and emit an explanation naming setup, interchange, insertion, and the recognition rationale. User-entered algorithms get classified and decomposed the same way.
6. Score and cache: results land in generated_alg under params_hash, the hash of letter scheme, buffer mode, subset, weights, and search bounds, so config changes regenerate only stale partitions. Batch runs go through a worker thread with UI progress, and make gen-chart runs headless for e2e.

Scoring: score = a*len_qtm + b*subset_penalty + c*regrips + d*finger_penalty. subset_penalty sums per-move weights, 0 for preferred moves. regrips walks the sequence against a hand-position table and counts switches. finger_penalty sums per (face, direction) weights. All 4 weights live in the config hub, and the decomposition is stored in score_parts_json for UI explanation.

Tractability: interchange pruning cuts 9^4 raw setups to a handful, there are ~500 insert families, validation costs microseconds at 10^3 to 10^4 simulations per case, 818 cases total, and ~10^7 simulations overall. A full fixed-buffer chart takes seconds to a minute single-core and parallelizes across cases.

Optimality caveat: results are optimal only within the template-plus-bounded-fallback search space under the configured subset, and fixed-buffer output is cross-checked against BH reference fixtures. No global optimality claim is made.

## Testing

Test framework: hand-rolled tests/framework.h plus framework.c, about 300 SLOC. CT_SUITE registration, CT_ASSERT, CT_ASSERT_EQ, CT_ASSERT_DBL_EQ, CT_ASSERT_STR_EQ, a setjmp and longjmp failure model, console output on failure with file and line, and sfc64 PRNG helpers for randomized suites. The runner is tests/test_main.c: an argument filters suites by name, so make test-\<suite\> runs one suite, and the binary also carries a spawn selftest mode. Zero vendoring, with full exit-code control for the mutation harness.

Coding rule from the FSRS work: ct_grade is an all-nonnegative enum, so clang gives it an unsigned underlying type, and bare g - CT_GOOD for grades below GOOD wrapped to about 4.29e9 instead of going negative. That silently corrupted the difficulty and same-day formulas for AGAIN and HARD while every bounds-only property test still passed, and only the independently derived goldens caught it. Cast to signed before enum arithmetic, write (int)g - CT_GOOD, and src/srs/fsrs.c applies the cast at every grade arithmetic site.

Domain notes from the adversarial review: the notation parser accepts n = 1 while ct_cube_init clamps to n at least 2, deliberate and covered by goldens, and a state 0 card's first rating sets due_day to today with the interval applying from the second review, consistent with the FSRS goldens.

The M0 and M1 adversarial review record with both reviewers' findings and dispositions is docs/reviews/2026-10-10-adversarial-m0m1.md.

Fuzzing: hand-rolled in-process fuzz loops, one deterministic seeded sfc64 generator per iteration with the seed printed on failure. 16 suites now run across tests/fuzz_engine.c, tests/fuzz_srs.c, tests/fuzz_db.c, and tests/fuzz_config.c: notation tokens, garbage strings, random algorithms, transform inverse, simplify, and rotate_y, cube invariants and mirror, FSRS state walks, elapsed values, and grade space, repo card, review, and settings string handling, and config keys and move masks. The iteration count is the config default fuzz.iterations (10000) multiplied by CT_FUZZ_LOOPS (make default 10, capped), so make fuzz runs 100000 iterations per suite and CT_FUZZ_LOOPS=1 gives the config base. Sanitizer gap, stated plainly: libFuzzer and ASan are unavailable on the windows-gnu target with this LLVM, because compiler-rt ships only MSVC-ABI .lib files there. UB is caught via -fsanitize=undefined -fsanitize-trap=all, which needs no runtime. Coverage-guided fuzzing and ASan are a recorded gap, with an MSVC-target build as the future path. Fuzz surfaces still planned: generator validation and the e2e script runner.

Mutation testing: Mull has no native Windows support, so tools/mutate.c does token-level source mutation, landed in 74d29fc. The operator set: arithmetic swaps (+ against -, * against /), comparison swaps (< against <=, > against >=, == against !=), logic swap (&& against ||), integer literal plus and minus one, bool literal flip, bang deletion, and return literal swap between 0 and 1. make mutate SCOPE=src/srs SUITE=\<suite\> links each mutant against the build/tobj object set with the mutated TU swapped in, runs a baseline pass first and aborts if it fails, then runs the chosen suite or the full unit binary per mutant and prints the kill rate. Per-run artifacts sit under build/mutants/p\<pid\>/ with pid-suffixed binaries and are cleaned unless CT_MUTATE_KEEP=1 (4921aca), so concurrent runs never collide, and CT_MUTATE_LIST=1 lists sites without running. Survivors must match tests/mutant_allowlist.txt, one line per accepted equivalent mutant as a file glob with optional :line, a tab, and a justification, and any unexplained survivor fails the run. Site counts reproduce only over a quiet tree, with no concurrent source edits or rebuilds, and the harness aborts on a missing build/tobj object, so scoped mutation gates run in quiet build windows after a complete make test.

E2E: make e2e runs every tests/e2e/*.txt through cubing-trainer.exe --headless --script, the real binary in the headless flavor with UI actions scripted through an input-injection hook and final DB state asserted. The script runner is still pending, and while tests/e2e holds no cases the target exits 1 naming the pending runner.

Visual regression: landed with M1 T7. The headless path opens a hidden GLFW window (FLAG_WINDOW_HIDDEN before InitWindow) on OpenGL 3.3, renders offscreen to a RenderTexture2D at 960x540, and reads back through LoadImageFromTexture plus ImageFlipVertical into raylib's PNG encoder, in src/app/headless.c behind tools/render_scene.c. rlsw was evaluated and rejected, its 3D rasterization is broken in the vendored raylib 6.0 snapshot, evidence in playground/headless_spike/REPORT.md. 4 goldens are versioned in tests/golden/ (solved 3 and 7, scramble 3, mid animation 3) with SHA-256, scene parameters, generation commands, and machine dependence recorded in tests/golden/PROVENANCE.md. Comparison runs through tools/visual_check.py, a dependency-free Python aHash over an 8x8 grid with a hamming distance threshold of 4 and a mean delta threshold of 0.25, verified to pass regenerated goldens and fail both a 40x40 sticker recolor and a one token alg edit. The goldens encode GPU rasterization output, so a different GPU or driver requires regenerating them.

## Source records

Policy: every sourcing action leaves a physical artifact with the source URL, retrieval date, and content hash. Curated event pack seed rows carry the source_url, source_at, and content_hash columns, vendor archives are pinned by hash, and visual goldens get records in tests/golden/PROVENANCE.md. Current records, URLs HEAD-checked 2026-10-09, vendor archives re-downloaded and re-verified 2026-10-10:

| artifact | source | verification |
| --- | --- | --- |
| raylib 6.0 source tarball | https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz | SHA3-256 87fadf1148a547fda4a40dd9bcef059787085deb70515ef80a45839e27188cac, derived from the live tag archive 2026-10-10, no published checksum upstream |
| raygui 5.0 source tarball | https://github.com/raysan5/raygui/archive/refs/tags/5.0.tar.gz | SHA3-256 723b0b86b71a5df7163ee48fcdf4afd0e44abfcf547326f848161b6c5682d6b7, derived from the live tag archive 2026-10-10, no published checksum upstream |
| sqlite 3.53.4 amalgamation zip | https://sqlite.org/2026/sqlite-amalgamation-3530400.zip | SHA3-256 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e, published by sqlite.org for the then-current release and re-derived from the live artifact 2026-10-10 |
| BH commutator pages | https://www.speedsolving.com/wiki/index.php/BH | domain facts for classes and case counts |
| Speffz scheme text | https://www.speedsolving.com/wiki/index.php/Speffz | face order and clockwise rule, db/seeds/speffz_source.md |
| Speffz buffer letters | https://bldtrainer.com/glossary and https://bldtrainer.com/letters | UBL = A and UF = C buffer facts, db/seeds/speffz_source.md |
| PLL case count 21 | https://www.speedsolving.com/wiki/index.php?title=PLL | page fetched directly 2026-10-10 |
| OLL case count 57 | https://www.speedsolving.com/wiki/index.php/OLL | count confirmed via search results citing the page 2026-10-10, page not fetched line by line |
| F2L 41 per slot | https://www.speedsolving.com/wiki/index.php/First_Two_Layers | page fetched directly 2026-10-10 |
| WCA events change post | https://www.worldcubeassociation.org/posts/changes-to-the-wca-s-list-of-official-events-june-2026 | FTO official 2027-01-02, clock retired after Worlds 2027 |
| FSRS algorithm spec | https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm | formulas and default weights |

## Diagrams

All 5 diagrams are mermaid flowchart LR rendered to PNG by make diagrams through the installed mmdc. Sources are docs/diagrams/*.mmd and the rendered PNG files are committed alongside them:

| diagram | shows | status |
| --- | --- | --- |
| app_loop | app main loop, fixed-timestep update plus render | landed |
| review_session_flow | review session from due queue through reveal, grade, and FSRS update to the session log | landed, matches src/trainer/session.c |
| generator_pipeline | src/gen stages from target enumeration to cached algs | landed, every stage implemented, classification in 85f18e1 |
| data_flow | ui screens through the repo layer to sqlite, settings rows merging over config defaults | landed, startup merge in 0730ead, matches src/app/app_loop.c |
| milestone_map | the M0 to M6 sequence with status | landed, status as of 2026-10-10 |
