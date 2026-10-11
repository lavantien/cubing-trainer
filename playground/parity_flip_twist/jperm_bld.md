# source: https://jperm.net/bld
# retrieved: 2026-10-10, live fetch this session via web reader (direct fetch from build machine timed out, reader succeeded)
# title: How To Solve a Rubik's Cube Blindfolded
# stored: page text minus site navigation and image placeholders, blob cube renders not capturable

Solving a cube blindfolded sounds impossible at first, but I guarantee you anybody is capable of it.

You only need to know 3 algorithms, and memorize around 20 letters each solve.

Here are the steps:

- Memorize where each piece should go
- Solve 1 piece at a time
- The other pieces don't move while you do this

## Edge Solving Method

### Hold the cube with the white center on top, and the green center on front.

### Edge Swap Algorithm: R U R' U' R' F R2 U' R' U' R U R' F'

#### Swaps UR (buffer) with UL (target)

### Look at the sticker in the buffer (UR) position, and swap that sticker to its solved position using the swapping algorithm.

### The edge swap algorithm can only swap with the target (UL), but you can add _setup moves_ to instead swap with any sticker on the cube.

#### Example: Swapping the buffer with FL

Setup: L'

Swap

Undo Setup: L

#### Example: Swapping the buffer with LF

Setup: Dw' L

Swap

Undo Setup: L' Dw

#### Note: Focus on the buffer (UR) to see where to swap with. If you pick the wrong sticker to swap with, you will get a flipped edge.

### Repeat this process until all of the edges are solved.

### If the buffer piece (white/red) ends up in the buffer position, swap with any other _unsolved_ piece to continue.

### Tips for setup moves:

- As shown above, swapping to FL and LF are two different sticker cases.
- Do not use R/U/F/B moves to setup, since they disturb the other pieces moved by the swapping algorithm.
  - Only use L/Lw/Dw moves.
- For D face stickers, do some D-turn followed by L2 to setup.
- For E layer stickers, do some Dw-turn followed by L or L' to setup.
- For M layer stickers, do some Lw-turn to move it to the D face and continue setup.
  - Alternatively: do some Lw-turn to move it to the U face and use a J Perm PLL algorithm to swap it with the buffer.
- For S layer stickers, do 1 move that puts it in the E/M layer and setup from there.

#### Note: A full list of setup moves is in the description of the tutorial video if you're stuck. However, you should not memorize these as they are intuitive.

## Corner Solving Method

### Hold the cube with the white center on top, and the green center on front.

### Corner Swap: R U' R' U' R U R' F' R U R' U' R' F R

#### Swaps LBU (buffer) with DFR (target)

### The concept is identical to solving edges, with minor differences:

- The buffer sticker is LBU, and the target sticker is DFR. Setup moves will bring pieces to DFR.
- The corner swap algorithm is different.

#### Example: Swapping the buffer with FRU

Setup: R'

Swap

Undo Setup: R

#### Example: Swapping the buffer with UFR

Setup: F2 D

Swap

Undo Setup: D' F2

#### Note: Focus on the buffer (LBU) to see where to swap with. If you pick the wrong sticker to swap with, you will get a twisted corner.

### Tips for setup moves:

- Do not use U/B/L moves to setup, since they disturb the other pieces moved by the swapping algorithm.
  - Only use D/F/R moves.
- For D face stickers, do some D-turn to setup.
- For F face stickers, do some F-turn followed by R' to setup.
- For R face stickers, do some R-move followed by F to setup.
- For other stickers, do 1 move that puts it in the D/F/R face and setup from there.

#### Note: A full list of setup moves is in the description of the tutorial video if you're stuck. However, you should not memorize these as they are intuitive.

## Memorization

Letters for Edges

Letters for Corners

### Instead of naming stickers by faces (i.e. UF edge, UFR corner), it is easier to memorize a single letter for each sticker.

### Example Scramble: B2 R2 B2 L U2 R2 B2 R2 B2 R' U' B' F L' B D R B2 D B' D' U

### 1. First Cycle (edges)

- Look at the sticker in the buffer location (UR), and find where this sticker goes. Memorize this location.
  - In the example scramble, blue/white is at the buffer, so I memorize Q.
- At the location you just memorized, memorize where this piece goes. Stop when you reach the buffer piece (white/red).
  - Yellow/green is at Q, so I memorize U.
  - Blue/yellow is at U, so I memorize S.
  - Red/white is at S, and red/white is the buffer piece so I stop here.
- If every unsolved piece has been visited, memorization is complete. If not, continue to new cycles.

#### Note: You will not need a new cycle if your first cycle includes every unsolved piece.

---

### 2. New Cycles (edges)

- Memorize any unsolved piece that you have not visited yet (if any exist).
  - In this example I will memorize R, but there are many possibilities.
- Continue memorization the same way as before. Stop after you memorize the same _piece_ you began with.
  - Red/blue is at R, so I memorize N.
  - Yellow/orange is at N, so I memorize X.
  - Green/white is at X, so I memorize I.
  - Yellow/red is at I, so I memorize V.
  - Red/green is at V, so I memorize P.
  - Blue/orange is at P, so I memorize R.
  - R is the same piece that I began the new cycle with, so I stop here.

#### Note: It is possible for a new cycle to begin and end on _different letters_, as long as they are still the _same piece_ (example: R and H). You may have to do multiple new cycles to reach every unsolved piece on the cube.

---

### 3. Flipped Pieces (edges)

- Memorize any sticker on a flipped piece (if any exist).
  - In this example the UL edge is flipped. I will memorize D, but I could also memorize E.
- Memorize where this sticker belongs
  - Orange/white is at D, so I memorize E.

#### Note: There may be multiple flipped pieces.

### Repeat all of the above, but for corners. Here is a summary of corner memorization using the same concepts:

- 1. First cycle (corners)
  - Before memorizing anything, I have already reached the buffer.
- 2. New cycles (corners)
  - New cycle: Memorize MWQXM (begins and ends on the same piece).
  - New cycle: Memorize DUI (begins and ends on the same piece).
- 2. Flipped pieces (corners)
  - The DFL corner is flipped. Memorize VP, as the sticker at V belongs at P.

---

### Letters memorized for edges: QU SR NX IV PR DE

### Letters memorized for corners: MW QX MD UI VP

#### Note: The number of letters for corners/edges must always be both even or both odd.

---

### To memorize a sequence of letters, pair them up as shown above, and memorize an image for each letter pair. Next, make a story out of all the images.

### Example for edges: QUick StaR NeXus IVy PRint DEad.

### A very quick star falls out of the sky, takes out its Nexus phone to photograph an ivy plant, prints the photo, and realizes the plant was actually dead. This is much easier to memorize than a sequence of 12 random letters!

### Another example is shown at the end of the tutorial video.

#### Note: If you are just starting out, you can write down the letters to avoid memorization mistakes.

## Solving After Memorization

### Solve the edges, possibly solve parity*, and then solve the corners.

### *If the number of letters memorized for edges/corners are both odd, then do the parity algorithm between solving edges and solving corners.

#### Note: The number of letters for corners/edges must always be both even or both odd.

### Parity Algorithm: R U' R' U' R U R D R' U' R D' R' U2 R' U'

#### Use this between solving corners and solving edges, if you have memorized an odd number of letters for both.

### For each letter you have memorized:

- Do setup moves to bring the desired sticker to the target location.
- Do the swapping algorithm.
- Undo the setup moves.

#### While solving, you do not need to think about the buffer piece, new cycles, or flipped pieces. As long as you solve each letter in your memorization, the cube will be solved in the end.

## Example Solves

1. B2 R2 B2 L U2 R2 B2 R2 B2 R' U' B' F L' B D R B2 D B' D' U

Edges: QU SR NX IV PR DE

Corners: MW QX MD UI VP

- Solving Edges:
  - Q: Lw' D L2 [swap] L2 D' Lw
  - U: D' L2 [swap] L2 D
  - S: Lw' D' L2 [swap] L2 D Lw
  - R: L [swap] L'
  - N: Dw L [swap] L' Dw'
  - X: L2 [swap] L2
  - I: Lw D' L2 [swap] L2 D Lw'
  - V: D2 L2 [swap] L2 D2
  - P: Dw' L' [swap] L Dw
  - R: L [swap] L'
  - D: [swap]
  - E: L Dw' L [swap] L' Dw L'
- Solving Parity:
  - No parity (even edges and even corners).
- Solving Corners:
  - M: F [swap] F'
  - W: D' [swap] D
  - Q: R D' [swap] D R'
  - X: D2 [swap] D2
  - M: F [swap] F'
  - D: F2 [swap] F2
  - U: D [swap] D'
  - I: F R' [swap] R F'
  - V: [swap]
  - P: R F [swap] F' R'

2. U' L2 D' U R2 B' D' U' L2 B2 R' U' B' F' L U2 F R2 U'

Edges: KD IE XO WJ HL QN X

Corners: OU SC VN QI F

- Solving Edges:
  - K: Lw D L2 [swap] L2 D' Lw'
  - D: [swap]
  - I: Lw D' L2 [swap] L2 D Lw'
  - E: L Dw' L [swap] L' Dw L'
  - X: L2 [swap] L2
  - O: D' Lw D L2 [swap] L2 D' Lw' D
  - W: D L2 [swap] L2 D'
  - J: Dw2 L [swap] L' Dw2
  - H: Dw L' [swap] L Dw'
  - L: L' [swap] L
  - Q: Lw' D L2 [swap] L2 D' Lw
  - N: Dw L [swap] L' Dw'
  - X: L2 [swap] L2
- Solving Parity:
  - There is parity (odd edges and odd corners).
  - [parity algorithm]
- Solving Corners:
  - O: R2 F [swap] F' R2
  - U: D [swap] D'
  - S: D F' [swap] F D'
  - C: F2 D [swap] D' F2
  - V: [swap]
  - N: R' F [swap] F' R
  - Q: R D'[swap] D R'
  - I: F R' [swap] R F'
  - F: F' D [swap] D' F

## Next Steps

Another walkthrough solve is in this video, showing new cycles and flipped pieces. Also, check the description of the tutorial video for answers to common questions.

The M2 method is a much faster way to solve edges, but it has more special cases to deal with. Once you can do blindfolded solves consistently, I recommend learning the M2 method, as well as other advanced blindfolded techniques.

### How To Get Faster at Blindfolded
