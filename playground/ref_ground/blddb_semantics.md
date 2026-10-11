# blddb semantics

sources fetched 2026-10-10 from github.com/nbwzx/blddb branch v2, directory scripts:

- https://raw.githubusercontent.com/nbwzx/blddb/v2/scripts/youtube.py (saved as blddb_youtube.py, 349 lines)
- https://raw.githubusercontent.com/nbwzx/blddb/v2/scripts/tracer.py (saved as blddb_tracer.py, 196 lines)
- repo README https://github.com/nbwzx/blddb (landing page only)
- edge orientation rule cross-checked against https://www.speedsolving.com/wiki/index.php?title=Edge_Orientation (retrieved 2026-10-10)

the repo README documents none of the conventions below. every statement here is either derived from the export pipeline source and then verified empirically in our engine over all 1077 rows (simcheck.tsv), or purely empirical. verification summary: corner 366/366 CYCLE_A, edge 622/622 CYCLE_A, flips 25/25, twists 25/25, parity 28/28, ltct 11/11 legal (twist sum 0 mod 3), 0 parse failures.

## pipeline (youtube.py main)

1. scrapes a solver's youtube channel, reads each video description, follows alg.cubing.net, bit.ly, tinyurl.com links (resolved, must land on alg.cubing.net, 3x3 only).
2. url-decodes, maps `_` to space and `-` to `'`, splits description lines, splits each line further on ` if `, ` or `, ` and ` (youtube.py:28).
3. each fragment goes through commutator.expand (pypi package commutator, expands `[A,B]` and `setup:[A,B]`, canonicalizes tokens so `M2'` becomes `M2`), then add_rotation, then expand again.
4. tracer.get_code_auto classifies; MAX_STM 20, MAX_CELL_LEN 60, dedup by exact alg string per case. when a raw cell does not classify, the crawler retries with each of R U D and their primes and 2s appended or prepended (youtube.py:259-266), so some rows are pipelinesynthesized variants of the solver's fragment.
5. rows written per sheet with columns Position, Alg, Commutator (commutator.search output, `Not found.` when absent), Reconstruction Link (the original alg.cubing.net url, bit.ly shortened in the sheet), Youtube Link.

spot check against the source reconstructions: cherry edge row 5 `UF-UB-LB` alg `U' R E' R' U2 R E R' U'` expands from https://bit.ly/2Ymdwpw whose alg text contains the literal line `U-_R_E-_R-_U2_R_E_R-_U-_ // [U- : [R_E-_R-,_U2]]` from a successful 12.81 single. the exported alg is the solver's executed fragment verbatim, not normalized beyond token canonicalization and the appended rotation below.

## position naming convention

names are sticker-level. first letter names the face whose sticker is the target. remaining letters follow axis priority U/D before F/B before R/L. the 24 corner names: UFR RUF FUR, UFL FUL LUF, UBR BUR RUB, UBL LUB BUL, DFR FDR RDF, DFL LDF FDL, DBR BDR RDB, DBL BDL LDB. the 24 edge names: UF FU, UB BU, UR RU, UL LU, FR RF, FL LF, DF FD, DB BD, DR RD, DL LD, BR RB, BL LB (tracer.py cornerCodeToPos and edgeCodeToPos against solved_333, verified 48/48: solved_333 is a 54-char string in kociemba ULFRBD face order, row-major per face, U row 0 = B side col 0 = L side, L row 0 = U col 0 = B, F row 0 = U col 0 = L, R row 0 = U col 0 = F, B row 0 = U col 0 = R, D row 0 = F col 0 = L; that grid orientation is identical to our ct_frames grid, so name to our sticker index is face*4+pos with pos from ct_corner_grid / ct_edge_grid, see position_map.tsv).

each piece has a head sticker: the U/D-face sticker for pieces that have one, else the F/B-face sticker (the equator edges FR FL BR BL). in letter-scheme terms the head is the group head of youtube.py edge_ch/corner_ch.

## cycle direction (edge and corner sheets), empirically verified 100%

position `A-B-C` means: the named sticker of A moves to the named sticker position of B, B's named sticker to C's, C's named sticker to A's. a pure sticker 3-cycle on exactly the three named pieces, nothing else affected. direction is fixed, no alternative orientation exists in the data (988/988 rows CYCLE_A).

derivation: tracer.get_raw_code traces the arrival map track(c) = sticker now sitting at c's home, starting at the head of the first unsolved scheme group; a 3-cycle yields raw `h X Y h` with X = P^-1(h), Y = P^-2(h), and get_code_auto exports raw[0]+raw[2]+raw[1] = (h, Y, X), which satisfies P(h) = Y, P(Y) = X, P(X) = h. token 1 is the head sticker of the first unsolved group in scheme order, i.e. the floating buffer, never an arbitrary rotation of the cycle.

## defect encoding: many rows are not pure piece 3-cycles

the tracer follows ONE sticker per piece and never checks the orientation of the other stickers. consequence, verified empirically: a row where all three tokens are head stickers (208 of 988 rows) is a clean piece 3-cycle with no twists or flips; a row where a token names a non-head sticker (780 of 988) carries a piece twist (corners) or flip (edges) on exactly the pieces named by non-head tokens, with the twist/flip direction readable from which sticker is named. correlation is exact: 0 mismatches across 988 rows. example `UF-UB-LB`: LB names the L sticker (non-head, head is BL), and the simulation shows the UB and LB pieces flipped inside the cycle while the named stickers still cycle UF to UB to LB to UF.

this is not data corruption: the source reconstruction of that row is a fragment of a successful 12.81 single. floating-buffer 3-style execution is sticker-following, so the solve remains correct while individual target algs twist or flip the pieces they move; the next target is read off the displaced sticker. for comparison against our piece-level 3-cycle generator only the 208 all-head rows are exact matches; the 780 defect rows need the defect folded in or must be excluded, see optimality_method.md.

## floating order

the readme lines (edge `UF UB UR UL FR FL DF DB DR DL`, corner `UFR UFL UBR UBL DFR DFL`) are the first 10 of 12 edge and 6 of 8 corner letter-scheme groups (full: edges + BR BL, corners + DBR DBL, youtube.py edge_order_buffer/corner_order_buffer). it is the priority order that picks which unsolved group starts the raw code trace, i.e. the order the buffer floats through when the buffer piece returns home mid-solve. empirically the sheets' 3-style first tokens follow exactly this priority.

## flips sheet, verified 25/25

two edges flipped in place: both stickers of each named edge swap within the slot, nothing else moves, centers fixed.

## twists sheet, verified 25/25

two corners twisted in place in opposite directions (3-cycle of the slot's stickers each), nothing else moves. the header `(cw & ccw)` describes the pair, but youtube.py sorts the two letters before display, so which printed corner is the cw one is not recoverable from the position string alone; simcheck notes carry each corner's cycle and signed direction (twist_dir: +1 = the piece's U/D sticker steps counterclockwise viewed from outside the corner).

## parity sheet, verified 28/28

one edge 2-swap plus one corner 2-swap, nothing else moves. token order within each pair is scheme priority. the swapped pieces may carry flips/twists in the same defect encoding as 3-style rows: parity positions name head stickers only in the clean rows; simcheck notes report per-piece flip/twist status (for example `UF-UL, UFR-FUL` shows UFR +1 UFL -1 twists, i.e. a twisted corner swap).

## ltct sheet, empirical on all 11 rows

every ltct row is: the two named edges swapped in place oriented (all 11 rows name UF-UR, the floating edge buffer endgame), plus the two named corners c1-c2 swapped with twists, plus the bracketed corner c3 twisted in place, total corner twist sum 0 mod 3 (verified 11/11). the bracket token names the sticker of the in-place twisted corner (a non-head sticker), and non-head tokens among c1/c2 encode the swap twists exactly as in 3-style rows. the name ltct is not expanded anywhere in the repo; the state class in tracer terms is branch 1/2 of get_code_auto's 11-char corner code (a 7-char swap-with-twist signature plus a 4-char twist signature).

## notation dialect

tracer.sequence_to_state accepts face turns U D L R F B with `'` and `2`, lowercase wide `r u d l f b` (slice + face), inner slices `2U 2D 2L 2R 2F 2B`, slices `M E S` in siGN semantics (M follows L, E follows D, S follows F; our engine implements the same, verified by simulation agreement), rotations `x y z` with `'` and `2`. tokens like `M2'` in raw reconstruction text are canonicalized to `M2` by commutator.expand before use. our ct_notation_parse accepted every one of the 1077 exported algs, zero failures.

## orientation guarantee

every exported alg passes centersolved: applied to a solved cube it leaves the U center sticker at the U center position and the F center at the F center position. add_rotation appends a compensating rotation when the raw fragment left a different center pair up front. verified: all 1077 simulated rows had centers fixed as written, no rotation normalization was needed.

## orientation rules used by the verification (standard, pinned against generators)

corner twist: 0 iff the piece's U/D sticker sits on a U/D face; signed direction from the slot's U/D face to the landing face, cross product against the slot's body diagonal (+1 = counterclockwise viewed from outside the corner). edge flip: oriented iff, ordering the piece's two stickers and the slot's two faces by axis priority U/D, F/B, L/R, the higher-priority sticker sits on the higher-priority face; equivalent to the speedsolving wiki ZZ rule (on the U face, D face, or F/B face of an E-slice slot: L/R-colored sticker means bad; F/B-colored sticker means bad iff the other side is U/D-colored). both rules were validated against single generator turns (L, R, F from solved) and against the sheets' own flips/twists rows before use.
