# speffz seed source verification

Retrieval date for all sources: 2026-10-09.

## sources

https://www.speedsolving.com/wiki/index.php/Speffz

Verified verbatim: "The face order to move around the cube is ULFRBD". "Pieces are lettered around the face clockwise, when looking at the face directly". "Starting corners are U face - UBL; L face - LBU; F face - FLU; R face - RFU; B face - BRU; D face - DLF". "When lettering the D face, first perform an x2 rotation from the initial cube state, then start at what is now the UBL corner". The page text enumerates corner letters only through the starting corner table plus the clockwise rule, edge letters appear in its images only, and the page designates no buffers. The corner table derived below matches this source row for row.

https://bldtrainer.com/glossary

Verified verbatim: buffer entries "edges UF (C, I)" and "corners UBL (A, R, U)". This pins the letter facts: the U face sticker of the UBL corner is A and the U face sticker of the UF edge is C. The remaining stickers in each triple belong to other faces' letter blocks and are listed under divergences below. App buffer note, owner decision 2026-10-10: the edge buffer stays UF, letter C, and the corner buffer moved from UBL, letter A, to UFR, letter C, which follows from the same wiki clockwise rule as the derived mapping below.

https://bldtrainer.com/letters

The interactive letters lesson. Its markup confirms the U block is A to D and the F block is I to L with edges and corners sharing the per-face block, and no per-sticker table exists outside its interactive cube net.

https://blddb.net/ and its corner.html and edge.html pages

Consulted as the plan's preferred fixture source. The site is a JavaScript application whose static markup carries only position dropdown lists (UBL through DBL, UB through DL) and no letter scheme text, so it could not serve as the textual fixture. bldtrainer.com served as the independent reference instead.

## divergences

bldtrainer's own default scheme orders the face blocks differently from Speffz on the L and D faces (its letters page fragment reads U A-D, R E-H, F I-L, D M-P, L Q-T, B U-X). The speedsolving wiki starting corner table governs the L, R, B, D blocks here. No source conflicts with the pinned letter facts, UBL equals A and UF equals C, so no stop condition was reached. The app's buffer stickers are a configurable choice, UF for edges and UFR for corners since the 2026-10-10 owner decision, and do not affect the scheme rows below. Both orbits below are the standard Speffz scheme, not bldtrainer's variant.

## sticker indexing convention

This is the frozen app convention for addressing 3x3 sticker positions, matching the face frames src/engine/cube.h defines.

Face order is U L F R B D with face indices 0 to 5. Each face is a 3x3 grid with row 0 col 0 at the top left of the face's own frame: F reads row down and col right, U reads row towards B and col right so row 0 is the B adjacent row, and the remaining faces follow the standard cross net, L left of F, R right of F, B right of R, D below F, each with row 0 at the U adjacent edge.

Corner position p in 0 to 3 runs clockwise within the face grid starting at (0, 0): p0 at (0, 0), p1 at (0, 2), p2 at (2, 2), p3 at (2, 0). Edge position p in 0 to 3 runs clockwise starting with the edge containing (0, 1): p0 at (0, 1), p1 at (1, 2), p2 at (2, 1), p3 at (1, 0).

Sticker index = face index times 4 plus p, range 0 to 23 per orbit. Orbit values come from src/config/config.h: CT_ORBIT_CORNER is 0 and CT_ORBIT_EDGE is 1. The clockwise sweep in these net frames equals the clockwise when looking at the face directly rule on all six faces, and the Speffz letter for sticker index i is A plus i in both orbits.

## face grids

Each cell below names the piece sticker at that grid position.

U: row 0 is UBL, UB, UBR. row 1 is UL, U, UR. row 2 is UFL, UF, UFR.

L: row 0 is LBU, LU, LFU. row 1 is LB, L, LF. row 2 is LDB, LD, LFD.

F: row 0 is FLU, FU, FRU. row 1 is FL, F, FR. row 2 is FLD, FD, FRD.

R: row 0 is RFU, RU, RBU. row 1 is RF, R, RB. row 2 is RFD, RD, RBD.

B: row 0 is BRU, BU, BLU. row 1 is BR, B, BL. row 2 is BRD, BD, BLD.

D: row 0 is DLF, DF, DFR. row 1 is DL, D, DR. row 2 is DBL, DB, DBR.

## D face after x2

The x2 rotation maps F to B, U to D, and fixes L and R, so reading D after x2 starts at the corner that was DLF, which is D's own (0, 0) cell, and the after-x2 UB, UR, UF, UL edges land on DF, DR, DB, DL, exactly the D grid positions (0, 1), (1, 2), (2, 1), (1, 0). The engine D frame therefore already equals the read-after-x2 orientation and the plain grid positions above apply with no extra rotation.

## derived corner mapping

| face | row | col | orbit | letter | sticker |
| ---- | --- | --- | ----- | ------ | ------- |
| U | 0 | 0 | 0 | A | 0 |
| U | 0 | 2 | 0 | B | 1 |
| U | 2 | 2 | 0 | C | 2 |
| U | 2 | 0 | 0 | D | 3 |
| L | 0 | 0 | 0 | E | 4 |
| L | 0 | 2 | 0 | F | 5 |
| L | 2 | 2 | 0 | G | 6 |
| L | 2 | 0 | 0 | H | 7 |
| F | 0 | 0 | 0 | I | 8 |
| F | 0 | 2 | 0 | J | 9 |
| F | 2 | 2 | 0 | K | 10 |
| F | 2 | 0 | 0 | L | 11 |
| R | 0 | 0 | 0 | M | 12 |
| R | 0 | 2 | 0 | N | 13 |
| R | 2 | 2 | 0 | O | 14 |
| R | 2 | 0 | 0 | P | 15 |
| B | 0 | 0 | 0 | Q | 16 |
| B | 0 | 2 | 0 | R | 17 |
| B | 2 | 2 | 0 | S | 18 |
| B | 2 | 0 | 0 | T | 19 |
| D | 0 | 0 | 0 | U | 20 |
| D | 0 | 2 | 0 | V | 21 |
| D | 2 | 2 | 0 | W | 22 |
| D | 2 | 0 | 0 | X | 23 |

Corner buffer check: the UFR corner is face U, (2, 2), sticker 2, letter C, matching the app buffer set by the 2026-10-10 owner decision, which moved it from UBL, sticker 0, letter A.

## derived edge mapping

| face | row | col | orbit | letter | sticker |
| ---- | --- | --- | ----- | ------ | ------- |
| U | 0 | 1 | 1 | A | 0 |
| U | 1 | 2 | 1 | B | 1 |
| U | 2 | 1 | 1 | C | 2 |
| U | 1 | 0 | 1 | D | 3 |
| L | 0 | 1 | 1 | E | 4 |
| L | 1 | 2 | 1 | F | 5 |
| L | 2 | 1 | 1 | G | 6 |
| L | 1 | 0 | 1 | H | 7 |
| F | 0 | 1 | 1 | I | 8 |
| F | 1 | 2 | 1 | J | 9 |
| F | 2 | 1 | 1 | K | 10 |
| F | 1 | 0 | 1 | L | 11 |
| R | 0 | 1 | 1 | M | 12 |
| R | 1 | 2 | 1 | N | 13 |
| R | 2 | 1 | 1 | O | 14 |
| R | 1 | 0 | 1 | P | 15 |
| B | 0 | 1 | 1 | Q | 16 |
| B | 1 | 2 | 1 | R | 17 |
| B | 2 | 1 | 1 | S | 18 |
| B | 1 | 0 | 1 | T | 19 |
| D | 0 | 1 | 1 | U | 20 |
| D | 1 | 2 | 1 | V | 21 |
| D | 2 | 1 | 1 | W | 22 |
| D | 1 | 0 | 1 | X | 23 |

Edge buffer check: the UF edge is face U, (2, 1), sticker 2, letter C, matching the pinned buffer.

## artifacts

db/seeds/speffz.sql holds the same 48 rows as plain SQL with ON CONFLICT DO NOTHING, ordered by orbit then sticker. tests/unit_letter_scheme.c asserts the C seed and the SQL file produce identical maps, so the two must change together.
