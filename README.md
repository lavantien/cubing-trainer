# cubing-trainer

Speedsolving trainer for all WCA events. This README is the source of truth for the technical tables, the architecture, and the diagrams.

## Overview

cubing-trainer trains algorithms across all WCA events through flashcards with spaced repetition, auto-animates any algorithm on a 3D cube, and auto-generates 3-style commutator algorithms with structural classification, constrained to a preferred move subset and scored for fingertrick friendliness, including full floating-buffer mode.

The stack is raylib 6, C23, and SQLite, built with clang 23 targeting mingw-w64 over the winlibs sysroot, driven by GNU make.

The 3-style generator is the distinguishing feature. It emits commutators labeled by structure (setup, interchange, insertion, pure, A9, B9, and the rest of the BH taxonomy), searches under a preferred move subset that defaults to R, U, D, and runs in fixed-buffer or floating-buffer mode.

## Build and make targets

Prerequisites, verified on this machine 2026-10-09: LLVM clang 23.1.3, winlibs UCRT gcc 16.2.0 as the mingw-w64 sysroot, GNU make 4.4.1, cmake, ninja, sqlite3 CLI 3.53.4, and node with npx and mmdc (mermaid-cli). No MSYS2, no pkg-config.

Run `make vendor` first. It HEAD-checks each URL, downloads, verifies, and extracts the pinned third party sources under third_party/: the raylib 6.0 tarball, raygui 5.0 as a single .c and .h pair, and the SQLite 3.53.4 amalgamation zip verified by SHA3-256. raylib is compiled from third_party/raylib/src/*.c with -DPLATFORM_DESKTOP directly by the Makefile, and the app links -static-libgcc -lopengl32 -lgdi32 -lwinmm.

The cross compiler flag spelling is not frozen yet, the M0 build spike recorded in playground/spike_link/REPORT.md decides it. The current Makefile uses the primary candidate, `clang --target=x86_64-w64-mingw32 --gcc-install-dir=<winlibs gcc lib dir> -fuse-ld=lld`. Documented fallbacks: the `--sysroot=<winlibs root>` spelling, or dropping `-fuse-ld=lld` so clang uses the winlibs GNU ld.

| target | what it does |
| --- | --- |
| vendor | download, HEAD-check, verify, and extract raylib 6.0, raygui 5.0, and the sqlite amalgamation into third_party/ |
| build | compile app, raylib, raygui, and sqlite sources and emit build/bin/cubing-trainer.exe |
| build-headless | build the headless flavor used by e2e and visual runs |
| test | run the full unit suite |
| test-\<suite\> | run one named suite |
| fmt | format the tree |
| lint | clang-tidy plus the -Wall -Wextra -Werror -Wpedantic -std=c23 gate plus zero-comment and stray-constant checks |
| analyze | run clang --analyze over the sources |
| fuzz | run the in-process fuzz loops over every exposed surface |
| mutate | run token-level mutation for SCOPE=src/\<dir\> and fail on survivors outside the allowlist |
| e2e | run the real headless binary through tests/e2e scripts and assert final DB state |
| visual | render defined scenes headless and compare against goldens by perceptual hash |
| diagrams | render every docs/diagrams/*.mmd to PNG through tools/gen_diagrams.mjs |
| run | build then launch the trainer window |
| db-verify | run PRAGMA integrity_check plus schema assertions through the sqlite3 CLI against a copy |
| memguard | run tools/memguard.ps1 to enforce the combined working set cap on project processes |
| clean | remove build outputs |

## Architecture

Module tree:

```
Makefile README.md
db/migrations/001_init.sql..00N.sql  db/seeds/wca_packs.sql db/seeds/bld_fallback.sql
docs/diagrams/*.mmd                  docs/diagrams/*.png (generated)
src/config/config.h config.c
src/engine/notation.h notation.c cube.h cube.c transform.h transform.c geometry.h geometry.c
src/render/renderer.h renderer.c anim.h anim.c
src/ui/ui.h screen_menu.c screen_trainer.c screen_cards.c screen_review.c screen_settings.c screen_stats.c widgets.c
src/storage/db.h db.c migrate.c repo_cards.c repo_reviews.c repo_algs.c repo_settings.c repo_stats.c
src/srs/fsrs.h fsrs.c
src/trainer/session.h session.c cfop.c selector.c
src/gen/generate.h targets.c interchange.c synth.c classify.c score.c simplify.c
src/stats/stats.h stats.c
src/packs/packs.h packs.c provenance.c
src/app/main.c app_loop.c headless.c
third_party/raylib raygui sqlite
tools/mutate.c gen_diagrams.mjs
tests/framework.h unit_*.c fuzz_*.c golden/ e2e/
```

Layering, enforced by review: config depends on nothing. engine, srs, and storage depend on config. gen and render depend on engine. trainer depends on storage, srs, and gen. ui depends on trainer, render, and storage. app depends on ui. No upward or lateral edges.

The config hub is one header, src/config/config.h, holding ct_config, every enum (grade, orbit, buffer_mode, classification, event, method, stage, direction, ui_screen), and every tunable with its default: FSRS weights, requested retention, interval clamps, preferred move subset and per-face penalties, regrip and fingertrick weights, animation durations and easing, window defaults, DB filename, generator search bounds, and fuzz iteration counts. It is loaded once at startup and passed by pointer, with user settings merged over the defaults from the settings table. The lint target fails on numeric literals in src/ outside config.h, inline test tables are exempt, and values that recur across test tables get centralized too.

The data directory is %LOCALAPPDATA%/cubing-trainer, read from the environment at runtime.

## Notation grammar

A move is ct_move: face, depth range, quarter count. The parser and formatter cover this token grammar:

| token element | forms | notes |
| --- | --- | --- |
| digit prefix | optional leading digit | inner layer depth on big cubes |
| face | U D L R F B | outer layer turns |
| slice | M E S | middle slices |
| rotation | x y z | whole cube rotations |
| wide suffix | w, or a lowercase face | lowercase means wide |
| prime | ' | counterclockwise |
| half turn | 2 and '2 | 180 degrees |
| signed repeat | ++ -- + - | megaminx and clock turns |
| per-event extensions | square-1 slash notation, megaminx R++ D--, clock turns, FTO faces | event specific token tables |

Round-trip properties are a fuzz surface: format(parse(s)) normalizes, and parse(format(a)) == a.

## Commutator classes

Structural classification of 3-style commutators, from the speedsolving wiki BH (Beyer-Hardwick) pages:

| orbit | class | moves |
| --- | --- | --- |
| corner | pure | 8 |
| corner | A9 | 9 |
| corner | orthogonal | 10 |
| corner | cyclic shift | 11 |
| corner | column | 11 |
| corner | per special | 12 |
| edge | half slice-plane | 4 |
| edge | slice-plane SP5, SP7, SP9 | 5, 7, 9 |
| edge | pure | 8 |
| edge | A9 | 9 |
| edge | B9, edges only | 9 |
| edge | orthogonal | 10 |

Alternate labels (perpendicular, collision, simultaneous) ship as display aliases, the canonical labels are the wiki table. The generator also labels parity, edge flip, and corner twist cases outside this table.

## Case tables and letters

With a fixed buffer there are 378 corner targets (21x18) and 440 edge targets (22x20), per the BH wiki infobox.

Speffz letter scheme summary: face order U L F R B D, each face lettered clockwise from its start corner, so U=A-D, L=E-H, F=I-L, R=M-P, B=Q-T, D=U-X with D read after x2. Corner buffer UBL = A, edge buffer UF = C. The letter scheme is user-editable data in SQLite, and the seed scheme gets fixture-verified against blddb.net at M2 with the source trail recorded.

App definition, floating buffer mode: fixed-buffer mode generates 3-cycles that include the configured buffer sticker, which gives the 378 and 440 tables. Floating mode may emit any 3-cycle (x, t1, t2) that solves the target pair, since the buffer drifts during a real solve. The case table stays at 378 and 440, and the admissible solution space grows about 24x.

## Event packs

17 official events as of 2026-10-09. FTO joins officially on 2027-01-02 and clock retires after Worlds 2027, both per the WCA post linked under source records. Seed methods per event:

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

Every curated seed row records its source URL, retrieval date, and content hash.

## Database schema

The SQLite amalgamation is compiled into the exe. The DB lives at %LOCALAPPDATA%/cubing-trainer/trainer.db. Forward-only migrations in db/migrations/NNN_*.sql self-apply at startup inside a transaction, the version sits in PRAGMA user_version, and a pre-migration backup goes to trainer.db.bak. The repo_*.c files hold prepared-statement CRUD, 1 file per aggregate. DDL sketch:

```
settings(key, value)
letter_scheme(id, name, is_default)
letter_map(scheme_id, orbit, sticker, letter)
flashcard(id, deck, case_key, direction, front_json, alg_3x3, alg_big, alg_oh, notes, created_at, updated_at, unique(deck, case_key, direction))
srs_state(card_id PK, stability, difficulty, due, last_review, reps, lapses, state)
review_log(id, card_id, rating, reviewed_at, stability, difficulty, elapsed_days, scheduled_days)
generated_alg(id, orbit, target_pair, buffer_mode, category, sequence, classification, setup, interchange, insertion, qtm, score, score_parts_json, params_hash, created_at, unique(orbit, category, target_pair, buffer_mode, params_hash))
session_log(id, deck, started_at, ended_at, n_cards, n_again, n_hard, n_good, n_easy)
event_pack(id, event, method, stage, case_key, alg, front_json, provenance_url, provenance_at, content_hash, unique(event, method, stage, case_key))
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

The difficulty update is damped with mean reversion to D0(4), the recall and forget stability updates follow the wiki, and DECAY and FACTOR are the wiki constants. Clamps are recorded design choices: difficulty D in [1, 10], stability S in [0.1, 36500] days. Same-day reviews apply the same-day stability formula and leave difficulty unchanged. Golden unit tests derive expected values by hand from the wiki formulas before implementation.

The C surface sits in src/srs/fsrs.h: ct_grade (again 1, hard, good, easy), ct_fsrs_card, ct_fsrs, and the functions ct_fsrs_init, ct_fsrs_review, ct_fsrs_retrievability, ct_fsrs_next_interval_days.

## Generator

src/gen/ runs this pipeline per case:

1. Enumerate targets through the active letter scheme: 24 corner and 24 edge stickers, the 378 corner and 440 edge ordered pairs in fixed-buffer mode, and parity, flip, and twist categories from small tables.
2. Detect interchanges: move m interchanges stickers (x, y) iff m(x)==y and m(y)==x on the facelet permutations. Candidates are the 18 outer turns plus M, E, S, and doubles, precomputed per run.
3. Propose insertions from templates: 1-move, 3-move X Y X' over face pairs, and 5-move per special. Simulation disposes: a candidate insert is admitted only if it moves exactly 1 of the interchange pair's pieces into the interchange layer.
4. Synthesize: BFS setups S over the allowed subset (default R, U, D) to the config depth bound, keeping only setups that create an interchange among cycle members. Assemble [S: [A, I]], simplify, simulate on a solved cube, validate that the resulting sticker cycle equals the target exactly including orientation, then score and keep the best. Template misses fall back to bidirectional BFS over the subset to depth 5 per side, which covers optima to 10 moves, with a full-metric straggler path of 6 per side.
5. Classify: structural rules label the sequence and emit an explanation naming setup, interchange, insertion, and the recognition rationale. User-entered algorithms get classified and decomposed the same way.
6. Score and cache: results land in generated_alg under params_hash, the hash of letter scheme, buffer mode, subset, weights, and search bounds, so config changes regenerate only stale partitions. Batch runs go through a worker thread with UI progress, and make gen-chart runs headless for e2e.

Scoring: score = a*len_qtm + b*subset_penalty + c*regrips + d*finger_penalty. subset_penalty sums per-move weights, 0 for preferred moves. regrips walks the sequence against a hand-position table and counts switches. finger_penalty sums per (face, direction) weights. All 4 weights live in the config hub, and the decomposition is stored in score_parts_json for UI explanation.

Tractability: interchange pruning cuts 9^4 raw setups to a handful, there are ~500 insert families, validation costs microseconds at 10^3 to 10^4 simulations per case, 818 cases total, and ~10^7 simulations overall. A full fixed-buffer chart takes seconds to a minute single-core and parallelizes across cases.

Optimality caveat: results are optimal only within the template-plus-bounded-fallback search space under the configured subset, and fixed-buffer output is cross-checked against BH reference fixtures. No global optimality claim is made.

## Testing

Test framework: hand-rolled tests/framework.h, ~150 SLOC. CT_SUITE registration, CT_ASSERT, CT_ASSERT_EQ, CT_ASSERT_DBL_EQ, CT_ASSERT_STR_EQ, a longjmp failure model, console plus JSON output, timing, and sfc64 PRNG helpers for fuzz targets. Zero vendoring, with full exit-code control for the mutation harness.

Fuzzing: hand-rolled in-process fuzz loops with a deterministic seeded sfc64 generator per surface, N iterations, nonzero exit on assertion failure, and a watchdog thread for hangs. Sanitizer gap, stated plainly: libFuzzer and ASan are unavailable on the windows-gnu target with this LLVM, because compiler-rt ships only MSVC-ABI .lib files there. UB is caught via -fsanitize=undefined -fsanitize-trap=all, which needs no runtime. Coverage-guided fuzzing and ASan are a recorded gap, with an MSVC-target build as the future path. Fuzz surfaces: notation parser, transform algebra, FSRS numerics, repo SQL, generator validation, config parsing, and the e2e script runner.

Mutation testing: Mull has no native Windows support, so tools/mutate.c does token-level source mutation: swap arithmetic and comparison ops, swap and-or, flip integer and boolean literals by 1, delete negations, and mutate return literals. make mutate SCOPE=src/srs compiles each mutant with identical flags, runs the unit suite, and fails on survivors. tests/mutant_allowlist.txt holds 1 justification line per accepted equivalent mutant.

E2E: cubing-trainer.exe --headless --script tests/e2e/\<case\>.txt runs the real binary in the headless flavor, scripts UI actions through an input-injection hook, and asserts final DB state.

Visual regression: headless renders of defined scenes (solved 3x3 through 7x7, mid-animation frames, trainer screens) dumped as PNG and compared by perceptual hash with a threshold. Goldens are versioned in tests/golden/ with a record file holding URL, date, and hash per fixture. The exact headless renderer is not decided yet, rlsw is the M1 spike candidate with RenderTexture2D frame hashing as the fallback.

## Source records

Policy: every sourcing action leaves a physical artifact with the source URL, retrieval date, and content hash. Curated event pack seed rows carry the provenance_url, provenance_at, and content_hash columns, vendor archives are pinned by hash, and visual goldens get records in tests/golden/PROVENANCE.md. Current records, URLs HEAD-checked 2026-10-09:

| artifact | source | verification |
| --- | --- | --- |
| raylib 6.0 source tarball | https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz | reachable, verified live |
| raygui 5.0 source tarball | https://github.com/raysan5/raygui/archive/refs/tags/5.0.tar.gz | reachable |
| sqlite 3.53.4 amalgamation zip | https://sqlite.org/2026/sqlite-amalgamation-3530400.zip | SHA3-256 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e |
| BH commutator pages | https://www.speedsolving.com/wiki/index.php/BH | domain facts for classes and case counts |
| Speffz page | https://www.speedsolving.com/wiki/index.php/Speffz | domain facts for the letter scheme |
| WCA events change post | https://www.worldcubeassociation.org/posts/changes-to-the-wca-s-list-of-official-events-june-2026 | FTO official 2027-01-02, clock retired after Worlds 2027 |
| FSRS algorithm spec | https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm | formulas and default weights |

## Diagrams

All 5 planned diagrams are mermaid flowchart LR rendered to PNG by make diagrams through the installed mmdc. Sources are docs/diagrams/*.mmd and the rendered PNG files are committed alongside them. app_loop is the first to land, the rest arrive with their milestones:

| diagram | shows | status |
| --- | --- | --- |
| app_loop | app main loop, fixed-timestep update plus render | landed, docs/diagrams/app_loop.mmd |
| review flow | review session from card front to FSRS update | planned, lands with M4 |
| generator pipeline | the src/gen stages from target enumeration to cached algs | planned |
| data flow | UI to storage through the repo layer | planned |
| milestone map | the M0 to M6 sequence | planned |
