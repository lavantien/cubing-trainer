# corner buffer UBL to UFR, phase 1 ripple

owner decision 2026-10-10: defaults green front, white top, Speffz, UF edge buffer, UFR corner buffer. Color scheme and Speffz already landed. Scope here is the corner buffer and the alg cache defect it exposes.

## derivation

face order U L F R B D is face indices 0 to 5 (src/engine/cube.h:15, tables in src/gen/targets.c:5-15). corner sticker index is face*4+pos with pos clockwise from (0,0): pos0 (0,0), pos1 (0,2), pos2 (2,2), pos3 (2,0) (src/gen/targets.c:17-36). the U frame runs row toward +Z (F) and col toward +X (R) (src/engine/cube.c:6), so the U sticker of UFR sits at row 2 col 2 = pos 2 = sticker index 0*4+2 = 2. speffz letter for corner sticker 2 is C (db/seeds/speffz.sql:4, seed rule 'A'+sticker in src/storage/repo_settings.c:281). classic Speffz cross-check: U block A=UBL, B=UBR, C=UFR, D=UFL, so UFR = C on the U face, confirmed by the repo's own derived table db/seeds/speffz_source.md:63.

piece membership: UBL piece = stickers {0,4,17} (pinned tests/unit_targets.c:55), UFR piece = stickers {2,9,12} (9 = F(0,2), 12 = R(0,0)), independently pinned by tests/unit_targets.c:118-119 which asserts the buffer-2 chart excludes exactly 2, 9, 12.

change: CT_3STYLE_CORNER_BUFFER 0 (UBL, letter A) to 2 (UFR, letter C) at src/config/config.h:223. edge CT_3STYLE_EDGE_BUFFER 2 (UF, letter C) already correct.

## hash verdict

defect, must fix in the same change. the buffer is runtime-editable: settings keys bld.buffer_corner and bld.buffer_edge, written by the settings screen (src/ui/screen_settings.c:356,361), parsed back in ct_selector_scheme_load (src/trainer/selector.c:158-172) with the config macro as fallback. ct_alg_canonical (src/storage/repo_algs.c:108-147) hashes scheme_id, mode, preferred_moves, penalties, finger costs, depths, weights, candidate_max, and no buffer. generated_alg primary key is (orbit, category, target_pair, buffer_mode, params_hash) with target_pair being the letter pair, and a stored row is the 3-cycle buffer to t1 to t2, so the same key under a different buffer is different content. reads do not revalidate: src/ui/screen_trainer.c:198-204 serves the cached sequence as-is, src/app/script_expect.c:286 same, and algcount at src/app/script_expect.c:270 counts rows across eras. fix: mix both buffer stickers into ct_alg_canonical and widen ct_params_hash, callers src/app/script.c:749 and src/ui/screen_trainer.c:198. both buffers, not per-orbit: one params_hash value spans both orbits and the count/stale/drop helpers are per hash, so per-orbit buffers would split one generation era into two hashes.

adjacent defect, live-path verdict returned 2026-10-10, folded into commit 1: letter map contents are mutable under an existing scheme_id through the settings screen save path. ct_settings_form_stage_letter and ct_settings_form_swap_letter stage into f->staged (src/ui/screen_settings.c:529-555 draw and click path), ct_settings_form_save writes every staged letter with ct_letter_map_set(db, f->scheme_id, ...) in the same transaction as the buffer keys (src/ui/screen_settings.c:329-337). scheme_id is unchanged by those writes, so pair key meanings drift under a stable hash. no other live mutation path exists: ct_letter_scheme_create and ct_letter_scheme_set_default have no src callers outside tests, script.c only reads the default id or seeds a missing scheme, and script_expect.c table names are read-only count checks. fix: the two 24-letter map arrays join the two buffer stickers in the canonical string, CT_ALG_CANON_MAX 8192 has ample headroom, NULL map returns an empty hash so callers fail loudly instead of serving a false hit.

## ripple, changes needed

| site | change |
| --- | --- |
| src/config/config.h:223 | CT_3STYLE_CORNER_BUFFER 0 to 2 |
| src/storage/repo_algs.c:108-147,338-348 | ct_alg_canonical and ct_params_hash gain corner and edge buffer inputs |
| src/app/script.c:749 | pass rt->scheme.buffer_corner and buffer_edge |
| src/ui/screen_trainer.c:198 | pass f->scheme.buffer_corner and buffer_edge |
| tests/unit_algs.c:51-160,464 | 23 hash call sites gain buffer args, new red-first assertions that buffer changes change the hash |
| tests/unit_settings_form.c:66,120 | default pin f.buffer_corner 0 to 2, fallback pin likewise |
| tests/unit_selector.c:449 | corner first chart key "BC" to "AB", identity map t1=0 t2=1 after buffer-2 exclusion |
| tests/unit_selector_screen.c:123,128,132 | f.front "BC" to "AB", hash call signature, seeded pair "BC" to "AB" |
| tests/e2e/trainer_generate.txt:9-12 | genalg BC to AB rows, sequence/classification/qtm re-derived for cycle UFR to UBL to UBR and oracle-validated before pinning |
| tests/unit_e2e_script.c:244-247 | same repins as the e2e mirror |
| README.md:148 | "corner sticker UBL = 0 and edge sticker UF = 2" to "corner sticker UFR = 2 and edge sticker UF = 2" |
| README.md:150 | "Corner buffer UBL = A, edge buffer UF = C" to "Corner buffer UFR = C, edge buffer UF = C" |
| README.md:259 | hash description gains the buffer stickers |
| README.md:316 | buffer letters source row reworded, UFR = C follows from the wiki clockwise rule already recorded |
| db/seeds/speffz_source.md:13,25,86 | dated owner-decision note, scheme facts unchanged, corner buffer check line moves to UFR = face U (2,2) sticker 2 letter C |
| tests/fixtures/bh_reference.md:17 | reword line 17 to name sticker 0 as the fixture harness rotation anchor, not the app default |

## reviewed, no change

tests/unit_targets.c: explicit buffer args 0/2/16, buffer 2 partition already pinned at 118-126, mask pin at 55 is geometry. tests/unit_synth.c: explicit buffers, 4464 synth table is buffer-agnostic, bh suite must stay on buffer 0 because only 1 of 5 fixture corner cycles contains UFR, the rotation loop would run past its 3-entry array otherwise. tests/unit_letter_scheme.c, tests/unit_selector_packs.c: letter and pack surfaces unchanged. src/gen/synth.c, classify.c, targets.c: buffer is a parameter everywhere. src/ui/screen_settings.c: renders runtime scheme values. src/trainer/selector.c: reads the macro as fallback, no edit. visual goldens: sticker colors only. db/seeds/speffz.sql: letter map unchanged. docs/diagrams, docs/reviews: no buffer pins. tests/e2e other cases: pack and review paths only.

## phase 2 plan

commit 1 fix: hash includes buffers and letter map content. red-first in unit_algs.c (corner buffer differs, hash differs; edge buffer differs, hash differs; one map letter differs, hash differs), then repo_algs.c canonical and signature change carrying buffer_corner, buffer_edge, and a const ct_letter_map pointer (repo_algs.h gains the repo_settings.h include, same layer, no cycle), the two callers pass rt->scheme/f->scheme fields, and test call args follow. empty-hash boundary verdict, checked 2026-10-10: ct_repo_text_ok (src/storage/repo_cards.h:48-50) requires s[0] != '\0' and a NUL within cap, and all seven repo_algs entry points validate params_hash through it (put 174, get 212, count 238, list 254, stale 283, drop 308, drop_orbit 323), so the empty hash from a NULL map is already rejected with CT_REPO_ERR_ARG and no tightening is needed. boundary pins for put, get, and count with the empty hash land in unit_algs.c and are observed green in the commit 1 test run, evidence in the run output. chain: unit_algs and unit_selector_screen, fmt, lint, analyze, full suite, scoped mutation over src/storage/repo_algs.c, e2e.

commit 2 feat: default UFR. red-first: unit_settings_form.c:66 to 2, unit_selector.c:449 to "AB", then config.h 0 to 2, then unit_selector_screen.c repins. expectations derived from the enumeration rule (t1 ascending from 0, t2 ascending from 1, buffer piece 2,9,12 excluded) not from output. chain: feature suites, fmt, lint, analyze, full suite, e2e.

commit 3 test: trainer_generate. capture the deterministic output for pair AB = cycle UFR to UBL to UBR, then validate before pinning: playground probe copied from playground/adv_m2m5_a/probe_synth.c with the corner buffer parametrized to 2 sweeps all 378 pairs through ct_synth_target, and the verify_synth.py independent python simulator confirms each emitted sequence is exactly that 3-cycle with piece locality, plus independent qtm and classification checks. artifacts under playground/buffer_ufr/. repin tests/e2e/trainer_generate.txt and the unit_e2e_script.c mirror. e2e green.

commit 4 docs: README and speffz_source.md and bh_reference.md wording rows above.

## phase 2 results, 2026-10-11

commit 1 8928008, commit 2 08291f1. full suite 28624456 pass 0 fail at each commit. scoped mutation over repo_algs.c after the retargeted allowlist: 316 generated, 316 resolved, 0 unexplained. the sentinel and null-check mutants were killed by test and simplification rather than allowlisted.

chart revalidation under buffer 2: probe_chart.c sweeps all 378 corner and 440 edge pairs through ct_synth_target with the landed defaults, chart_buffer2.tsv holds the 818 rows with 0 synthesis misses. verify_chart.py re-simulates every emitted sequence with an independent python cube model, its own face frames and ring ordering, and asserts for every row the exact 3-cycle buffer to t1 to t2, piece locality over the three touched pieces, and an independent qtm count: 818 rows, 0 failures. chart_buffer0.tsv is the pre-change control run proving the harness against the old buffer-0 chart, 818 rows 0 failures, first row the then-pinned BC case.

the AB case, first corner pair under buffer 2, is t1=0 UBL and t2=1 UBR, cycle UFR to UBL to UBR. it is the same permutation as the old BC cycle UBL to UBR to UFR rotated to the new buffer, so the generated sequence U' R2 Uw R2 Uw' F2 Uw R2 Uw' F2 R2 U, per special class, and qtm 18 carried over unchanged. ref_ground cross-check: verify_refcheck.py maps the world 1 and 2 reconstruction rows to sticker indexes, 325 UFR-buffer corner cycles are shared with the sweep and all 325 are present in the generated chart, 0 missing. the remaining simcheck rows are UF and target special pairs or other-buffer cycles, outside the chart pair space.

screen_settings.c scoped mutation surfaced a pre-existing never-gated surface: 648 mutants, 47.5 percent kill rate, 340 unexplained, unchanged by this work since the edit replaced identifiers with config macros and adds no mutation sites. recorded here for routing to the settings file owner.
