# source: http://www.cubefreak.net/bld/m2_guide.html
# retrieved: 2026-10-10, live fetch this session via web reader
# title: Blindfold Cubing with M2
# stored: full guide text, navigation and the history section's world record prose kept, images none present

## 1 A Brief History

Invented by Stefan Pochmann in early 2006, the M2/R2 blindfold cubing method is a significant improvement of the original Pochmann method. Unlike the 4-step 3-cycle method (hereafter simply 3OP), which separates orientation from permutation in both corners and edges, the Pochmann method and M2/R2 combine the two steps and instead use 2-cycles. Details of both methods can be found on Stefan's website: original Pochmann, M2/R2.

## 2 Preparation

In methods combining orientation and permutation, it is natural to consider cycles of stickers rather than of cubies. Although visual memorization of sticker cycles still works, many cubers find it helpful to use a lettering scheme for the 24 edge stickers. Here's mine:

L stickers: UL=I, LU=J, FL=K, LF=L, DL=M, LD=N, BL=O, LB=P
M stickers: DF=Q, FD=R, UB=S, BU=T, UF=U, FU=V, DB=W, BD=X
R stickers: UR=A, RU=B, BR=C, RB=D, DR=E, RD=F, FR=G, RF=H

UR, for example, refers to the U sticker of the UR edge while RU refers to the R sticker of the same edge.

We assign a sequence, as shown below, to each L/R sticker and to UB/BU:

L stickers: UL=LU'L'U, LU=BL'B', FL=U'L'U, LF=L'BL'B', DL=LU'LU, LD=BLB', BL=U'LU, LB=L'BLB'
R stickers: UR=R'URU', RU=B'RB, BR=UR'U', RB=RB'R'B, DR=R'UR'U', RD=B'R'B, FR=URU', RF=RB'RB
UB=(nothing), BU=B'R'BR'URU'

Note that (s) brings s to position UB without disturbing the other M edges. No move is needed for UB.

## 3 Phase 1: Memorization

First, locate any edge that is flipped in the correct position. We will call these inactive edges and the rest active edges. Memorize the locations of the inactive edges and note whether the number of inactive edges is even or odd.

Starting with the buffer sticker DF, we will now generate sticker cycles. To do this, locate the sticker position where the DF sticker belongs. Memorize this sticker position or the corresponding letter, and repeat the procedure with the sticker in this position. You will eventually reach a sticker that belongs to either DF or FD. Do not memorize this position. This completes your first sticker cycle.

If this cycle involves all active edges, the memorization phase is complete. Otherwise, we need to "break into a new cycle."

Choose any sticker on any active edge that does not already appear in a cycle. If possible, choose a sticker with an easy corresponding sequence, such as UB or FR. Memorize the position of this sticker, and start a new sticker cycle from it, memorizing each sticker location as before. The cycle ends when you return to this sticker you started with or the other sticker on the same edge position. Unlike with the first cycle from DF, you need to memorize this last sticker position. Thus, you should have two stickers of the same edge memorized, corresponding to the beginning and the end of the cycle.

Repeat the process of breaking into a new cycle until all active edges have appeared in some cycle.

Finally, memorize all sticker positions in pairs of two, ever if a cycle ends with the first in a pair. If there is one sticker remaining at the end, memorize it alone. You do not need to memorize where a cycle begins or ends; this is already coded in since you memorize the first edge of each cycle except the first twice.

Note: Unlike in many versions of M2, it is not necessary to switch UF/FU with DB/BD for the second sticker in a pair.

## 4 Phase 2: Resolution

### 4.1 Pairs

The memorization has split the sticker cycles into 3-cycles. For each memorized pair of stickers starting with the first pair, we perform the 3-cycle involving DF and the two stickers in that pair. Use the following rules to determine the appropriate sequence:

Case 1: Neither sticker in M
For pair s t, where s and t are both side stickers, perform (s) M2 (s)' (t) M2 (t)'

Case 2: One sticker in M
Group I: UB s: M2 (s) M2 (s)'. s UB: (s) M2 (s)' - M2. BU s: (BU) M2 (BU)' - (s) M2 (s)'. s BU: (s) M2 (s)' - (BU) M2 (BU)'.
Group II: UF s: (s) U2M'U2M (s)'. s UF: (s) M'U2MU2 (s)'. DB s: (s) MU2MU2M2' (s)'. s DB: (s) M2U2M'U2M' (s)'.
Group III: FU s: M2' (s) M (s)' M. s FU: M' (s) M' (s)' M2. BD s: M2 (s) M' (s)' M'. s BD: M (s) M (s)' M2.

Case 3: Both stickers in M (only some cases)
No flip: UF UB: U2M'U2M. UB UF: M'U2MU2.
Flip on U: FU UB: D[M',UR2U']D'. BU UF: D[M,U'R2U]D'. UB FU: D[UR2U',M']D'. UF BU: D[U'R2U,M]D'.
Flip on F: BU FU: xD'[UR2U',M']Dx'. FU BU: xD'[M',UR2U']Dx'.

[X, Y] is commutator notation and means X Y X' Y'.

### 4.2 Endgame

Case a: Zero sticker left
If there were an even number of inactive edges during memorization, orient them just as in the EO step of 3OP edges. If there were an odd number, orient in addition the DF edge.

The edges should now be solved.

Case b: One sticker left
If there were an even number of inactive edges during memorization, orient them just as in the EO step of 3OP edges. If there were an odd number, there are two cases:
(1) If the last sticker is on U, D, or on F/B in the middle layer (recall the UDRLF2B2 EO definition), then orient in addition the edge containing the last sticker.
(2) Otherwise, orient in addition the DF edge.
In either case, this leaves us with with a 2-cycle of this last sticker and DF with correct orientation according to the UDRLF2B2 EO definition. Finish the parity as in 3OP together with two corners while or after you solve the corners.

## 5 Examples

### Example 1

Scramble: B2 D2 F D' U2 R' B2 U L2 R' D2 L' U2 R' U B R2 D L F R2 U F D2 R2

Memorization: No inactive edge. BL FR. UF BU. BR FL. LD RD. DB UR (DB goes to DF, ending the first cycle. begin the second cycle with UR). UL RU (the repeated UR/RU ends the second cycle). No parity.

Resolution:
BL FR: (BL)M2(BL)' (FR)M2(FR)' = U'LU-M2-U'L'U-URU'-M2-UR'U'
UF BU: D-[U'R2U, M]-D' = D-U'R2U-M-U'R2U-M'-D'
BR FL: (BR)M2(BR)' (FL)M2(FL)' = UR'U'-M2-URU'-U'L'U-M2-U'LU
LD RD: (LD)M2(LD)' (RD)M2(RD)' = BLB'-M2-BL'B'-B'R'B-M2-B'RB
DB UR: (UR)MU2MU2M2'(UR)' = R'URU'-MU2MU2M2'-UR'U'R
UL RU: (UL)M2(UL)' (RU)M2(RU)' = LU'L'U-M2-U'LUL'-B'RB-M2-B'R'B

### Example 2

Scramble: D F' D R2 B2 U F L' F' L2 U2 L2 B R2 B' R U' D B R' F' D2 B' L' R2

Memorization: Inactive edge: UB (odd). FR UR. RB DR (RB goes to DF, ending the first cycle. begin the second cycle with DR). LB FL. LD UF. LU BD. DR (the repeated DR/RD ends the second cycle).

Resolution:
FR UR: (FR)M2(FR)' (UR)M2(UR)' = URU'-M2-UR'U'-R'URU'-M2-UR'U'R
RB DR: (RB)M2(RB)' (DR)M2(DR)' = RB'R'B-M2-B'RBR'-R'UR'U'-M2-URU'R
LB FL: (LB)M2(LB)' (FL)M2(FL)' = L'BLB'-M2-BL'B'L-U'L'U-M2-U'LU
LD UF: (LD) M'U2MU2 (LD)' = BL'B'-M'U2MU2-BLB'
LU BD: M(LU)M(LU)'M2' = M-BL'B'-M-BLB'-M2'

DR is on D, so we orient DB in addition to the inactive edge, UB: F2-M'UM'UM'U2MUMUMU2-F2. This leaves us with the 2-cycle (DF DR) with correct orientation according to the usual definition.
