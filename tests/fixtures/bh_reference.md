# BH 3-style reference fixtures

Source pages:

- Corners: https://www.speedcubing.com/chris/bhcorners.html (Beyer-Hardwick corner algorithms, Daniel Beyer and Chris Hardwick)
- Edges: https://www.speedcubing.com/chris/bhedges.html (Beyer-Hardwick edge algorithms, Daniel Beyer and Chris Hardwick)

Retrieved: 2026-10-10.

Page conventions, verified by simulation in playground/synth_sweep/bh_probe.c on 2026-10-10:

- Corner buffer is URB, edge buffer is UR. Every listed cycle starts at the buffer.
- A cycle (X Y Z) means the alg sends the sticker content at X to position Y, at Y to Z, and at Z back to X, matching our ct_sticker_perm forward direction.
- A piece name's first letter names the sticker, the remaining letters name the piece (URB is the U sticker of the URB corner, FD is the F sticker of the DF edge).
- Each alg moves exactly the 3 pieces of its cycle, 9 corner or 6 edge stickers, and nothing else.

Our translation: this fixture suite anchors its cycles at corner sticker 0, UBL, Speffz letter A, and edge sticker 2, UF, letter C, both explicit parameters of the suite rather than the app default. Sticker letter is 'A' plus sticker index. The case (t1, t2) for the chart under test is the listed cycle rotated to start at the anchor buffer.

| orbit | cycle | alg |
| --- | --- | --- |
| corner | URB UBL ULF | L F' L B2 L' F L B2 L2 |
| corner | URB UBL UFR | R2 B2 R F R' B2 R F' R |
| corner | URB UBL FRU | R B L B' R' B L' B' |
| corner | URB UBL FLD | U' B D' B' U B D B' |
| corner | URB BRD UBL | U B' D' B U' B' D B |
| edge | UR UF FD | U F E F' U' F E' F' |
| edge | UR UF BR | U F E' F' U' F E F' |
| edge | UR UF BL | R' E2 R U' R' E2 R U |
| edge | UR UF LU | L F' L' S' L F L' S |
| edge | UR UF DR | D M2 D' R2 D M2 D' R2 |
