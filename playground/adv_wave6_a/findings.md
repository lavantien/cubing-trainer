# adversarial review A findings, wave 208976b..36c9111, 2026-10-11

Reviewer A attacked from the data and domain side. Verdict: seeded data and shipped semantics are clean, 2 defects in wave code, 4 observations. Probes and outputs under playground/adv_wave6_a/. Full suite 28,624,456 asserts and all 6 e2e cases green at HEAD during review.

## defects

| id | severity | location | finding |
| --- | --- | --- | --- |
| A-D1 | medium | src/storage/repo_algs.c:121-174 | ct_alg_canonical silently truncates at CT_ALG_CANON_MAX 8192. gen.penalty_* and gen.finger_cost_* pass through plain assign_double (config.c:243-272, no upper bound) and weights carry only a 0 floor, so 28 fields at 1e300 (~309 chars each as %.6f) push the canonical past 8192 before the letter map segments, which serialize last. probe_hash through public ct_params_hash: map A versus rotated map B under huge doubles hash identically (b2a90c2d5de616f9 both), sane control differs. Reachable via script and db settings rows. Fix direction: fail closed on truncation or bound the doubles. |
| A-D2 | low | src/trainer/selector.c:389-395 | case_group returns group 0, displayed flip, for any unknown prefix. seed_registry merges bigcube_l2e.tsv and bigcube_parity.tsv into one 17-row deck per event under 5X5/6X6/7X7 REDUCTION PARITY, so the 12 l2e rows display as flip. probe_deck on the real seed files with the trainer counting loop renders "list parity cases 17 | flip 12 twist 0 parity 5". Display only, the 3BLD specials deck is unaffected and pinned. |

## observations

| id | finding |
| --- | --- |
| A-O1 | REFUTED at adjudication 2026-10-11: under the correct slot-relative orientation rule from blddb_semantics.md:70 (an edge is oriented iff the piece's higher-priority sticker sits on the slot's higher-priority face), parity.19 UF-BR swaps both edges fully oriented with corner twists only. The repo engine simulation (playground/adv_wave6_fix/probe_parity19_flip.c) and ref-ground's independent simcheck.tsv row 21 agree. The original claim rested on a simulator using absolute face-priority equality, which misclassifies any F/B to L/R sticker transfer as a flip. The landed PROVENANCE was already correct, nothing to change. |
| A-O2 | REFUTED at adjudication 2026-10-11: the source position string is UF-UR, UFR-FDL in the xlsx, the extracted TSV line 12, and the landed PROVENANCE, and the alg swaps UFR with DFL oriented, matching the string. The UFR-FUL spelling appears only in this reviewer's own rejected.tsv extraction artifact. The "second occurrence" reject reason stands, no source erratum exists. A-O2 as originally filed claimed the printed string was UFR-FUL and cited a simulation of a UFR-DFL swap as the contradiction, the simulation was correct and contradicted only the reviewer's transcription. |
| A-O3 | pre-existing from 18f7a9f: the trainer screen caches cfg, scheme, and scheme_id for the app lifetime, the opened flag never resets, dulling the hash fix in the GUI. |
| A-O4 | ct_letter_map_load does not validate letter range on load. Out-of-range letters canonicalize as '.', sharing a hash with unset stickers. UI-unreachable, cache-bloat only. |

## verified clean

Seed fidelity 85/85 byte-identical against an independent xlsx reader, raw pool 89, exactly the 4 recorded rejects dropped, no position duplicates, ordering holds. Semantics by independent 3D sticker simulator with T-perm and (R U)^105 oracles: flips 25/25, twists 23/23 with first token +1 = ccw viewed from outside (the ccw/cw versus +1/-1 wording is the same statement, the xlsx header is display-order only), parity 26/26, ltct 11/11, plus op.39, op.40, m2.23, m2.24 verified. Hash fix covers every ct_gen_cfg field plus scheme, mode, buffers, both maps, NUL-safe dot encoding, the only escape is A-D1. UFR switch: no surviving UBL assumption in code, buffer 2 = UFR piece stickers {2, 9, 12} exactly, chart_buffer2.tsv set-equal to the first-principles 378+440 enumeration, the repinned AB alg simulates as exactly the UFR to UBL to UBR 3-cycle at qtm 18, same as the old BC, an oracle-backed repin. Wiring: 126/88 special counts derived and pinned green with cap 0, negative cap, short cap, invalid orbit, invalid buffer all covered. Allowlist: all 226 rows from 5d84675 are synth.c rows and synth.c is untouched by later wave commits, so no anchor drift is possible, 15 sampled anchors match their justifications, the 4464 versus 6544 capacity claim verified against the synth.h formula C(8,3)*27*2 + C(12,3)*8*2 and the unit pin at tests/unit_synth.c:684.
