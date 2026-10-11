# source: https://bldtrainer.com/parity, https://bldtrainer.com/algs, https://bldtrainer.com/corners
# retrieved: 2026-10-10, live fetch this session via web reader, site nav discovered from https://bldtrainer.com/ raw HTML
# title: BLD Trainer - Learn 3x3 Blindfolded (Old Pochmann Method, SiGN Notation, Speffz Letter Scheme)
# stored: the three lesson pages verbatim, site chrome stripped

## https://bldtrainer.com/corners (Lesson 4, Corner Solving)

Corners work just like edges, but the buffer is UBL and we use a Y-perm to swap UBL with UFR. Setup moves bring the target corner to UFR without disturbing UBL.

The Corner Cycle:
1. Look at the UBL corner. Read its letter.
2. Find the target corner and set it up to UFR.
3. Execute the Y-perm to swap UBL and UFR.
4. Undo the setup.
5. Repeat until corners are solved.

Corner Swap Algorithm (Y-Perm): F R U' R' U' R U R' F' R U R' U' R' F R F'

Swaps UBL and UFR corners. It also swaps two edges, but edges are already solved at this point, which is why we do corners last.

Corner Setup Examples: UFR (C): none. UBR (B): R2. UFL (D): F2.

Special Case: Twisted Corners

Just like a flipped edge, a corner that sits in its own slot but twisted is not solved. Tracing skips only fully solved corners, so a twisted-in-place corner never appears in your main cycle, you pick it up afterwards as a new cycle (cycle break).

- Start a new cycle on the twisted corner and read through its stickers. Shooting them rotates the corner back into its solved orientation.
- Which sticker you start from changes how many shots it takes, the simulator on this site always starts at the alphabetically-first sticker, which is one valid choice, not always the shortest.
- Beginner trap: corners are the last step, so it's easy to forget that a corner can be home but mis-twisted. Always check orientation before removing the blindfold.

Example: scramble the cube and look at the UFL corner (its stickers are D / L / Q). It is in its slot but twisted. Resulting corner memo: CBJPDLQHTWKOM. The twisted UFL corner appears as its own run through the piece's stickers, D, L, Q, instead of being skipped as "already home."

## https://bldtrainer.com/parity (Lesson 5, Parity)

Sometimes edges have an odd number of targets. That leaves your corners in an odd permutation, a single two-corner swap that 3-cycles can never fix on their own. You resolve it with one extra J-perm between edges and corners, then adjust your corner memo.

Rule: Odd number of edge targets means parity exists. Apply the parity alg between edges and corners.

Parity Algorithm: R U R' F' R U R' U' R' F R2 U' R' U'

This is the same J-perm you use for edges. Run it once after your edge targets and before corners: it swaps the leftover two edges and two corners, and leaves the centers untouched. Because it moves two corners, you adjust your corner memo to match.

Compensating Your Corner Memo: When you have parity, swap your first two corner targets before solving corners. The parity alg's corner swap then cancels cleanly. Exactly which two corners the alg disturbs depends on your buffer and setup conventions.

Full Solve Flow with Parity:
1. Memorize edges and corners separately, counting your edge targets.
2. If the edge count is odd, you have parity, swap your first two corner targets in memo before you start.
3. Solve all edge targets using J-perms.
4. If you have parity: apply the parity J-perm R U R' F' R U R' U' R' F R2 U' R' U' once, now, between edges and corners.
5. Solve all corner targets using Y-perms.
6. Remove blindfold and celebrate.

## https://bldtrainer.com/algs (Reference, Algorithm Library)

Search, preview, and play back every algorithm used in the Old Pochmann method.

R U R' F' R U R' U' R' F R2 U' R' U'

Swaps the buffer edge (UF) with the target edge (UR). Also swaps two corners, which is fine because corners are solved afterward.

Corner Swap (Y-Perm): F R U' R' U' R U R' F' R U R' U' R' F R F'

Swaps the buffer corner (UBL) with the target corner (UFR).

Parity: R U R' F' R U R' U' R' F R2 U' R' U'

Used when you have an odd number of edge targets. It is the same algorithm you use for edges: applied once between edges and corners, it swaps the leftover two edges and two corners (no centers move). Swap two of your corner targets to compensate.
