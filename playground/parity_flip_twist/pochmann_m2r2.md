# source: http://www.stefan-pochmann.de/spocc/blindsolving/M2R2/
# retrieved: 2026-10-10, live fetch this session via web reader, page still live (HTTP 200), last modified November 08 2007 per the page footer
# cross-check: https://web.archive.org/web/20090202214856/http://www.stefan-pochmann.de/spocc/blindsolving/M2R2/ fetched this session, content identical
# title: M2/R2 blindcubing methods
# stored: full page text minus header images and the history section is kept, tables flattened to text

M2/R2 blindcubing methods

Update 2: I added more examples.

Update 1: I fixed some minor issues and added the history section.

In January 2006, I developed a new method for solving the 3x3 edges blindfolded and briefly described early versions in the blindcubing yahoo group. Since then I have improved the method, adapted it for solving corners (first it was only for edges) and used it in competitions. Sighted, i.e. using the method in regular speedsolving, I average a little under 50 seconds. I'm convinced with memorization and good recall a 40 seconds real average for execution is possible. This page briefly describes the two methods which I call M2 (for edges) and R2 (for corners).

Here's an example solve, the scramble is B' R' B' U' F L' U F2 L F R' D' R' B D F2 L2 D' R2 D' and the solution is on the right. This is an easy solve just to show the core idea. It does not include odd parity, breaking into new cycles, and cleaning up misoriented pieces.

Example target algorithms: FR: U R U' M2 U R' U', UL: L U' L' U M2 U' L U L', RB: l U' R' U M2 U' R U l', DB: M U2 M U2, LD: x' U L U' M2 U L' U' x, RU: x' U' R U M2 U' R' U x, UB: M2, DR: U R2 U' M2 U R2 U', FL: U' L' U M2 U' L U.

The main idea is the same as for my older method, namely solving one piece at a time, with "solving" meaning the whole piece, i.e., I don't separate orienting and permuting. Let me first talk about edges. Notice you should think in terms of stickers. When I say LU, I mean the L part, when I say UL, I mean the U part.

Edges: To solve one edge at a time, I use place DF as "buffer" and swap the edge currently in that place to where it belongs. I use one swap algorithm which swaps DF with UB. To swap DF with another edge, I first bring that edge to UB with setup moves, then apply the swap, then undo the setup moves. Since it's impossible to just swap two pieces and not change anything else on the cube, I do have to have some side effect. My swap algorithm actually is only an M2 move, so the side effect is a rotation of the M-slice centers and swapping the FU and BD edges, though I just think of solving these two relative to the centers, so they're not really swapped (relative to the centers they stay where they are). It only matters for choosing the algorithms for these two edges, think of the "target" telling you where the edge belongs before the algorithm. So for example if you target FU then after the alg the edge will actually end up at BD. For the M-slice edges I use the same algorithm for both orientations. I might end up with some flipped M-edges but that's not too bad (as you'll see later). The alternative is to use longer algorithms for half of the cases.

Target table: UB BU: M2 M2. FR DR BR UR: U R U' M2 U R' U' / U R2 U' M2 U R2 U' / U R' U' M2 U R U' / R' U R U' M2 U R' U' R. FL DL BL UL: U' L' U M2 U' L U / U' L2 U M2 U' L2 U / U' L U M2 U' L' U / L U' L' U M2 U' L U L'. RU RF RD RB: x' U' R U M2 U' R' U x / x' U' R2 U M2 U' R2 U x / x' U' R' U M2 U' R U x / l U' R' U M2 U' R U l'. LU LF LD LB: x' U L' U' M2 U L U' x / x' U L2' U' M2 U L2 U' x / x' U L U' M2 U L' U' x / r' U L U' M2 U L' U' r. DB BD UF FU: M U2 M U2 / M U2 M U2 / U2 M' U2 M' / U2 M' U2 M'.

Corners: This is really very much like the edges, only that things are happening in the R-slice instead of the M-slice. I solve the corner at DFR by swapping it to UBR with the "algorithm" R2. The side effect is a rotation of the R-slice "centers" (which are edge pieces), and URF and DRB stay where they were relative to the R-slice centers. Targets outside the R-slice (i.e., in the L-slice) get swapped with DFR after bringing them to UBR with setup moves. Targets inside the R-slice have special algs, and for UBR I simply use R2 for every orientation so that it'll likely end up misoriented which I fix at the end (you'll see it soon).

Memorization: ... for the M2 and R2 methods it is very helpful to memorize in pairs ... this automatically takes care of the current state of the centers of the M-slice (during edge solving) or the R-slice (during corner solving), which would otherwise have to be considered for UF/DB/URF/DRB because their "correct" place is relative to the centers which toggle between solved and not solved. ... memorizing in pairs also lets you determine parity easily, as in case of odd parity you simply end up with a single unpaired item at the end of memorizing the edges.

Special cases: If there's another cycle when I'm done with one, I break into the new one just like with my older method, choosing a target with a fast algorithm (usually M2 or R2). After solving the edges, I might end up with some M-slice edges flipped and some non-M-slice edges flipped because they were like that at the start of the solve. I flip them all together, usually with one of the algs on the right side after a setup move. Similarly corners, though mostly DFR and UBR are misoriented. In case of odd parity, the M and R crosses need to be rotated back and the parity algorithm does just that. I'm actually dreaming of having one big clean-up step at the end, fixing misoriented edges and corners and the parity all at once. Until now I know two algorithms combining parity with corner orientation.

Orient edges:
(M' U)*4
M' U M' U M' U2 M U M U M U2
(M' U M' U M' U M' U')*2
(U L U' L') (M' U)*4 (L U L' U')

Orient corners:
(R' U R2' U' R2 U' R' U) (R U R' U') (R2 U' R2 U)
(R U R2' U' R2 U' R' U) (R U R' U') (R2 U' R2 U R2)
(L' U' L U' L' U2 L) (R U R' U R U2 R')
(R U2 R' U' R U' R') (L' U2 L U L' U L)

Odd parity:
(r2' U' r2) (R' U) (L' U2') (R U' R' U2 R) (L U') (r2' U)

Combinations:
(r2 y') (R' U' l' L U' L U) (L' r' U) (l U') (F R U)
(L2 y') (R' U' l' L U' L U) (L' r' U) (l U') (F R U)

Example solves (special case demonstrations):

Solve 1, scramble: L U L' B' D2 U R' F2 B' D' U B' R2 U' D' F R D' F' U R B2 D B' D'
DB: M U2 M U2. UB: M2. DL: U' L2 U M2 U' L2 U. UR: R' U R U' M2 U R' U' R. LF: x' U L2' U' M2 U L2 U' x. DB: M U2 M U2. BR: U R' U' M2 U R U'. RF: x' U' R2 U M2 U' R2 U x. BU: M2.
Orient edges: L F (M' U M' U M' U2 M U M U M U2) F' L'
LUF: L' U' L' U R2 U' L U L. UBR: R2. DLF: U' L2 U R2 U' L2 U. BDR: (R U R' D r2 U') (R U r2' U') (D' R). RBD: (R' U R2 U' R' F' R U R2 U' R' F). BRU: R2. BLD: U' L U R2 U' L' U. LBU: U' L' U R2 U' L U. DBL: L U' F' L2' F U R2 U' F' L2' F U L'.
Orient corners and fix parity: (L2 y') (R' U' l' L U' L U) (L' r' U) (l U') (F R U)

Solve 2, scramble: R B2 L' R2 D2 F2 R2 F2 R U2 F U' F2 D' B' D2 U' R' B2 D' F'
DB: M U2 M U2. UB: M2. DL: U' L2 U M2 U' L2 U. RU: x' U' R U M2 U' R' U x. FL: U' L' U M2 U' L U. BD: M U2 M U2. BR: U R' U' M2 U R U'. RF: x' U' R2 U M2 U' R2 U x. BU: M2.
Orient edges: R F (M' U)*4 F' R'
LUF: L' U' L' U R2 U' L U L. UBR: R2. DLF: U' L2 U R2 U' L2 U. BDR: (R U R' D r2 U') (R U r2' U') (D' R). RBD: (R' U R2 U' R' F' R U R2 U' R' F). BRU: R2. BLD: U' L U R2 U' L' U. LBU: U' L' U R2 U' L U. DBL: L U' F' L2' F U R2 U' F' L2' F U L'.
Orient corners and fix parity: (L2 y') (R' U' l' L U' L U) (L' r' U) (l U') (F R U)

Stefan Pochmann
Last modified: November 08 2007, 15:37:21
