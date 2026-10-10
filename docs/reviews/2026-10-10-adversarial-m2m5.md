# adversarial review record, M2 through M5, 2026-10-10

Two independent agents attacked the generator, FSRS, storage, trainer, and event pack layers blind to each other. Reviewer A re-derived oracles from domain sources and probed numerics and geometry, reviewer B attacked storage and reachability boundaries. Verdicts: the generator core, Speffz scheme, clock and square-1 state models, and FSRS formulas meet the correctness bar, the surrounding state machine, cache contract, and selector surface do not at the reviewed HEAD. Probes and outputs are committed under playground/adv_m2m5_a/ and playground/adv_m2m5_b/.

## findings

| id | reviewer | severity | location | finding | owner |
| --- | --- | --- | --- | --- | --- |
| A-D1 | A | defect | src/srs/fsrs.c:52-54 | clamp_difficulty lacks an isfinite guard, NaN or inf difficulty reaches (long)round as float-cast-overflow UB, reachable from srs_state via ct_read_double | m6-fix-fsrs |
| A-D2 | A | defect | src/storage/repo_algs.c:105-129 | params hash is inverted in sensitivity, gen.penalty and gen.finger_cost feed score but are not hashed, srs.w is hashed but affects no generator output | m6-fix-storage |
| A-D3 | A | defect | src/engine/geometry_sq1.c:124-141 | sq1 move_facelets writes the twist permutation direction-inverted relative to the nxn driver behind the same vtable entry, unit tests pin the wrong convention | m6-fix-sq1 |
| A-D4 | A | defect | src/engine/geometry_sq1.c:218-219 | token parser accumulates parsed*10+digit unbounded, signed overflow UB on long digit runs | m6-fix-sq1 |
| A-D5 | A | defect | src/trainer/session.c:125 | ct_session_iso_add_days adds before validating, LONG_MAX days traps, interacts with the due_day saturation in fsrs.c:148 | m6-fix-app |
| A-D6 | A | defect | src/trainer/selector.c:43-47 | ct_lists duplicates two lists and handles only ids 0 and 1, FMC has no event-method pair, every seeded pack row is invisible to the selector surface, 48 of 62 lists dead | m6-fix-selector |
| B-D1 | B | defect | src/storage/repo_algs.c | params hash input set wrong, same class as A-D2 | m6-fix-storage |
| B-D2 | B | defect | src/srs/fsrs.c | state machine never advances past learning, review and relearning transitions of the FSRS-5 spec unimplemented | m6-fix-fsrs |
| B-D3 | B | defect | src/trainer/session.c | session_log stays empty because no exit path calls ct_review_form_end | m6-fix-app |
| B-D4 | B | defect | src/app/app_loop.c | settings writes do not re-merge app config, saved values stay dead until restart | m6-fix-app |
| B-D5 | B | defect | src/ui | ui.color_* settings are write-only, renderer never reads the saved scheme | m6-fix-app |
| B-D6 | B | defect | src/packs seed loader | TSV parsing trusts header positionally instead of by column name | m6-fix-storage |
| B-D7 | B | defect | src/storage | a stub database with user_version 1 and no tables passes startup validation, schema assertion missing | m6-fix-storage |
| B-G1 | B | medium | src/storage/repo_cards.c | case_key not normalized, direction not validated at the write boundary | m6-fix-storage |
| B-G2 | B | medium | src/storage/repo_cards.c | card text fields lack bounded checks, embedded content past the declared length is trusted | m6-fix-storage |
| B-G3 | B | medium | src/storage/repo_cards.c | read path truncates then updates, long fields round-trip asymmetrically through put then get | m6-fix-storage |
| B-G4 | B | medium | src/storage/repo_algs.c | no cache eviction api, stale partitions can never be dropped or regenerated on config change | m6-fix-storage |
| B-G5 | B | medium | src/storage/repo_algs.c | alg put and get disagree on sequence length limits | m6-fix-storage |
| B-G6 | B | medium | src/gen/interchange.c | interchange candidates for n 4 to 6 use orbit labels that are geometrically false | m6-fix-gen |
| B-G7 | B | medium | src/trainer/selector.c | selector hardcodes the identity letter map, active scheme ignored | m6-fix-selector |
| B-G8 | B | medium | src/packs/seed_registry.c | seed COMMIT failure leaves the transaction open, no ROLLBACK on the error path | m6-fix-storage |
| B-G9 | B | high | src/ui | trainer selector screen does not exist, selector and packs unreachable from the app menu, the real M5 T6 | m6-fix-selector |
| B-G10 | B | medium | src/trainer/selector.c | LBL list shared across 3 events, dead and duplicated lists persist | m6-fix-selector |
| B-G11 | B | medium | db/seeds/packs | 5x5 to 7x7 curated rows missing against the README claim, parity list only registered under 4x4 Yau | m6-fix-selector |
| B-G12 | B | low | src/config/config.c | settings merge silently drops bad rows, no log or count of rejected keys | m6-fix-app |
| A-O1 | A | note | src/gen/classify.c:431-445 | classifier stretches the BH taxonomy, 12 move outputs labeled orthogonal and 13 to 16 labeled per special against the fixture, non 3-cycles fall into half slice-plane because effect is never checked | m6-fix-gen |
| A-O2 | A | note | README classification table | README says corner cyclic shift 11 moves, the repo fixture bh_classification.md documents 10 and the classifier emits 10 | m6-fix-gen |
| A-O3 | A | note | src/gen/synth.c:466-486 | preferred subset only gates setups, direct template hits use any face, no best-score selection, README generator step 4 not implemented | m6-fix-gen |
| A-O4 | A | note | src/gen/synth.c:765 | fallback_depth bounds total path length, README says depth 5 per side, straggler_depth exists only in config | m6-fix-gen |
| A-O5 | A | note | src/gen/synth.c:720-723 | fallback allocates about 43 MB per call, zero invocations across the chart, ct_synth_init global state not thread-safe | recorded, worker-thread prerequisite before chart caching goes concurrent |
| A-O6 | A | note | src/gen/score.c:23-83 | regrip model charges between identical consecutive moves, score_check_seq validates face only, quarter 0 or 4 plus scores qtm 0 but still charges finger cost | m6-fix-gen |
| A-O7 | A | note | src/config/config.c | negative gen weights accepted, latent preference inversion once score selection lands | m6-fix-app |
| A-O8 | A | note | db/seeds/packs | pack seed algs are site text the notation grammar rejects, display-only by design per wca_sq1_clock_notation.md, README must state it | m6-fix-selector |
| A-O9 | A | note | src/packs/seed_registry.c:10 | bigcube_parity registered only under 4x4 Yau, README promises 5x5 to 7x7 reduction parity seeds, same root as B-G11 | m6-fix-selector |
| A-O10 | A | note | src/engine/geometry_sq1.c | parser accepts -0, (1,-0) parses as (1,0), leading zeros correctly rejected | m6-fix-sq1 |

## what survived both attackers

Reviewer A independently re-derived the full Speffz letter table from the wiki rules with 0 of 48 mismatches against the identity seed and confirmed both buffers. All 818 generated cases verified against an independently written 3D geometric facelet simulator, exact 3-cycles, correct orientation, exactly 3 pieces moved. All BH fixture algs reproduce their claimed cycles and classifications. FSRS-5 formulas re-implemented from the wiki spec match to 10 decimals. The clock move table is byte-identical to tnoodle ClockPuzzle.java. The square-1 state model matches tnoodle including slash gating and the reversed bottom ring. Civil date arithmetic round-trips the leap centuries. 65 malformed square-1 and clock tokens handled cleanly. Config set rejects non-finite and ERANGE on all probed double keys. Reviewer B confirmed storage stays parameterized with no SQL concatenation and rollback atomicity holds outside the seed error path.

## disposition

Six fix slots partition the table by file ownership. m6-fix-fsrs takes the scheduler state machine and numerics, m6-fix-storage the repo and seed loader boundary cluster, m6-fix-selector the trainer surface and pack reachability including the missing screen, m6-fix-app the settings, session, and color scheme cluster, m6-fix-sq1 the square-1 geometry defects, m6-fix-gen the generator scoring and classification cluster. A-O5 stays recorded as a prerequisite note rather than a slot. After the wave lands: fmt, lint, analyze, full suite, fuzz, mutation over the touched scopes, e2e, then visual, then M6 close.

## resolution

Every defect and every note except A-O5 landed during the fix wave on 2026-10-10, grouped by landing commit:

| ids | commits | outcome |
| --- | --- | --- |
| A-D1, B-D2 | bea663a | fsrs-5 state machine implemented per py-fsrs and ts-fsrs, non-finite difficulty guarded before the round cast |
| A-D2, B-D1, B-G4, B-G5 | 768e24d, bc608c8 | params hash covers penalty, finger_cost, subset, bounds, and candidate_max, srs.w dropped, eviction api added, put and get bounds symmetric |
| A-D3, A-D4, A-O10 | 664abe8 | sq1 twist permutations written destination-indexed, parser accumulation bounded before overflow, minus-zero tokens rejected |
| A-D5 | 775f770 | iso day span validated before the civil date add |
| A-D6, B-G10 | d65a58b | selector list model rebuilt over the registry, 62 lists live, duplication gone |
| B-D3 | 581f411 | review session logged on screen exit and queue exhaustion |
| B-D4 | 8879ebc | app config re-merged on settings save |
| B-D5 | 704bb2a | saved color scheme applied through the renderer |
| B-D6, B-G8 | 34b9a73 | pack tsv parsed by column name, failed seed commit rolls back |
| B-D7 | e7b0f4e | schema asserted at startup, user_version stub fails loudly |
| B-G1, B-G2, B-G3 | d0c6fde | card write boundaries enforced symmetric with reads, exact round-trip |
| B-G6 | 93d7544 | interchange pairing restricted to the 3x3 orbit labels |
| B-G7, B-G9 | 18f7a9f, 8bfca15 | trainer selector screen live, stored letter scheme routed through the lists |
| B-G11, A-O9 | d957e6a, f691aab, 40c37e6 | 5x5 to 7x7 reduction and parity packs seeded with provenance, registry 16 to 22 entries and 574 to 625 rows |
| B-G12 | 3dff0c2 | rejected settings rows counted and surfaced on save |
| A-O1, A-O2 | 0aedc7a | classes gated on exact 3-cycles and fixture move counts, README cyclic shift row reads 10 |
| A-O3 | 292ae14, b127bb8, 9e40674, cf47a2a | all candidates scored and the best kept under the config cap, e2e winner repinned with justification |
| A-O4 | 29c2349 | dead straggler_depth key removed, README pins the fallback as total-path bounded and candidate-empty gated |
| A-O6 | ac03b70 | grip continues through same-face repeats, bad quarters rejected in scoring |
| A-O7 | 0344f90 | negative gen scoring weights rejected |
| A-O8 | eebaa03 | README states pack seed algs are display-only site text |
| A-O5 | none | stays recorded, worker-thread prerequisite before concurrent chart caching |

Beyond the table, the wave also landed the aggregate heap corruption root cause fix (2caa801, missing zero-init before ct_session_begin), the mbld concepts pack mapping per the provenance record (25c7fed, 7882bd7, 68bd368), the alg repo read boundary suite (6379337), and the fixture decomposition span pins (718556a). Full suite green at 28,621,179 pass and 0 fail, make lint tree-wide green over 42 files, all e2e cases green over the real exe.
