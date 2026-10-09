# BH commutator classification fixtures

Source pages:

- Corners: https://www.speedcubing.com/chris/bhcorners.html (Beyer-Hardwick corner algorithms, Daniel Beyer and Chris Hardwick)
- Edges: https://www.speedcubing.com/chris/bhedges.html (Beyer-Hardwick edge algorithms, Daniel Beyer and Chris Hardwick)
- Definitions: https://www.speedsolving.com/wiki/index.php/Beyer-Hardwick_Method (Beyer-Hardwick Method, speedsolving.com wiki)

Retrieved: 2026-10-10.

## Class table

Structural definitions, verbatim where quoted from the speedsolving wiki page:

| orbit | class | alias | moves | structure |
| --- | --- | --- | --- | --- |
| corner | pure | | 8 | [A, B] = A B A' B', "A is always a 3 move insert. B is the interchange move and always 1 move". Wiki case names: Direct Insert, Drop and Catch, Toss Up |
| corner | A9 | | 9 | "Conjugated Pure Comms with a cancellation resulting in 9 moves instead of 10", S A B A' B' S', "All setup moves or S moves must cancel with A". Corners have no B9 |
| corner | orthogonal | perpendicular | 10 | "Conjugated Pure Comms with no cancellations" |
| corner | cyclic shift | collision | 10 | "[A: B] [C: B]" = A B A' C B C', A is 2 moves, B is a half turn, C is the reverse of A (not inverse). All bhcorners.html rows are 10 HTM. The speedsolving wiki page text and the project plan say 11 moves, both inconsistent with the listed algorithms and the formula |
| corner | column | simultaneous | 11 | "a setup into an A9 with no cancellation or a setup into a Cyclic Shift with a cancellation" |
| corner | per special | | 12 | "the A part is a 5 move insert and B is just a half turn", example [L U2 R' U2 L', U2] |
| edge | half slice-plane | | 4 | "A special type of commutator that makes its interchanges on the same outer layer", single-move interchange plus single-move insert |
| edge | SP5 | | 5 | "Slice-Planes are conjugated Half Slice-Planes commutators" |
| edge | SP7 | | 7 | conjugated half slice-plane, 7 moves |
| edge | SP9 | | 9 | conjugated half slice-plane, 9 moves. The bhedges.html SP9 form U' M U2 M' U2 M U2 M' U' has a 3-move part whose middle move equals the interchange |
| edge | pure | | 8 | "Same as BH Pure Comms". Wiki case names: Direct Insert, Drop and Catch, Toss Up |
| edge | A9 | | 9 | "Same as BH A9s except B9s exists" |
| edge | B9 | | 9 | cancellation lands on the interchange side instead of the insert side |
| edge | orthogonal | perpendicular | 10 | "Same as BH Orthogonals". bhedges.html labels these rows Setup into Drop and Catch and similar |

Non-commutator categories from the project plan taxonomy and the generated_alg classification column: parity, flip, twist.

## Example rows

Alg and cycle conventions match tests/fixtures/bh_reference.md. The class column is the canonical label expected from the classifier, derived from the wiki case name on the source row, not from the classifier.

| orbit | class | cycle | alg |
| --- | --- | --- | --- |
| corner | pure | URB UBL FUL | L F R' F' L' F R F' |
| corner | pure | URB UBL FLD | U' B D' B' U B D B' |
| corner | pure | URB UFR DLB | D F2 D' B2 D F2 D' B2 |
| corner | pure | URB UBL FRU | R B L B' R' B L' B' |
| corner | pure | URB BRD UBL | U B' D' B U' B' D B |
| corner | A9 | URB UBL ULF | L F' L B2 L' F L B2 L2 |
| corner | A9 | URB UBL UFR | R2 B2 R F R' B2 R F' R |
| corner | orthogonal | URB FUL RFD | U R' B2 R F R' B2 R F' U' |
| corner | cyclic shift | URB FUL BLU | F R' U2 R F' R' F U2 F' R |
| corner | cyclic shift | URB FDR RUF | D B' R2 B D' B' D R2 D' B |
| corner | column | URB ULF DFL | R U2 R' U' L2 U R U' L2 U' R' |
| corner | per special | URB ULF DRF | U F2 U' F2 U' R2 U F2 U F2 U' R2 |
| edge | half slice-plane | UR DL DR | R2 S' R2 S |
| edge | SP5 | UR UL DB | U M' U2 M U |
| edge | SP7 | UR UB UL | M2 U M' U2 M U M2 |
| edge | SP9 | UR UL DB | U' M U2 M' U2 M U2 M' U' |
| edge | pure | UR UF FD | U F E F' U' F E' F' |
| edge | pure | UR UB FU | M F R' F' M' F R F' |
| edge | A9 | UR UL FU | R' F R' S2 R F' R' S2 R2 |
| edge | B9 | UR UB RB | U F' E2 F U F' E2 F U2 |
| edge | orthogonal | UR UF UB | F' U R' E R U' R' E' R F |
