# source: https://jperm.net/bld
# retrieved: 2026-10-10 via server-side reader, direct fetch from build machine timed out
# title: How To Solve a Rubik's Cube Blindfolded
# stored: full text minus site navigation and image placeholders

Edge Swap Algorithm: R U R' U' R' F R2 U' R' U' R U R' F'

Swaps UR (buffer) with UL (target). Hold the cube with the white center on top, and the green center on front. Setup: L' swaps buffer with FL. Setup: Dw' L swaps buffer with LF. Do not use R/U/F/B moves to setup, only L/Lw/Dw moves. For D face stickers, do some D-turn followed by L2. For E layer stickers, do some Dw-turn followed by L or L'. For M layer stickers, do some Lw-turn to move it to the D face and continue setup.

Corner Swap: R U' R' U' R U R' F' R U R' U' R' F R

Swaps LBU (buffer) with DFR (target). Setup moves bring pieces to DFR. Example: setup R' swaps buffer with FRU. Example: setup F2 D swaps buffer with UFR. Do not use U/B/L moves to setup, only D/F/R moves.

Parity Algorithm: R U' R' U' R U R D R' U' R D' R' U2 R' U'

Use between solving corners and edges if an odd number of letters was memorized for both.

Example solve 1: scramble B2 R2 B2 L U2 R' B2 R2 B2 R' U' B' F L' B D R B2 D B' D' U
Edges: QU SR NX IV PR DE
Q: Lw' D L2 [swap] L2 D' Lw
U: D' L2 [swap] L2 D
S: Lw' D' L2 [swap] L2 D Lw
R: L [swap] L'
N: Dw L [swap] L' Dw'
X: L2 [swap] L2
I: Lw D' L2 [swap] L2 D Lw'
V: D2 L2 [swap] L2 D2
P: Dw' L' [swap] L Dw
R: L [swap] L'
D: [swap]
E: L Dw' L [swap] L' Dw L'
Corners: MW QX MD UI VP
M: F [swap] F'
W: D' [swap] D
Q: R D' [swap] D R'
X: D2 [swap] D2
M: F [swap] F'
D: F2 [swap] F2
U: D [swap] D'
I: F R' [swap] R F'
V: [swap]
P: R F [swap] F' R'

Example solve 2: scramble U' L2 D' U R2 B' D' U' L2 B2 R' U' B' F' L U2 F R2 U'
Edges: KD IE XO WJ HL QN X
K: Lw D L2 [swap] L2 D' Lw'
D: [swap]
I: Lw D' L2 [swap] L2 D Lw'
E: L Dw' L [swap] L' Dw L'
X: L2 [swap] L2
O: D' Lw D L2 [swap] L2 D' Lw' D
W: D L2 [swap] L2 D'
J: Dw2 L [swap] L' Dw2
H: Dw L' [swap] L Dw'
L: L' [swap] L
Q: Lw' D L2 [swap] L2 D' Lw
N: Dw L [swap] L' Dw'
X: L2 [swap] L2
Parity: odd edges and odd corners, apply parity algorithm.
Corners: OU SC VN QI F
O: R2 F [swap] F' R2
U: D [swap] D'
S: D F' [swap] F D'
C: F2 D [swap] D' F2
V: [swap]
N: R' F [swap] F' R
Q: R D' [swap] D R'
I: F R' [swap] R F'
F: F' D [swap] D' F

Memorization: letter pair images along a story, example edges QU SR NX IV PR DE as QUick StaR NeXus IVy PRint DEad. The number of letters for corners and edges must always be both even or both odd. The M2 method is recommended after consistency for faster edge solving.

Letter schemes: images edgeletters.png and cornerletters.png on the page, standard Speffz.
