# parity, buffer flip, and buffer twist sourcing captures

Retrieval date for every capture: 2026-10-10, fetched live this session. Simulation artifact: sim_output.txt, produced by parity_flip_probe.c against src/engine/cube.c, src/engine/notation.c, src/gen/targets.c, build and run logs below.

| capture | url | evidences |
| --- | --- | --- |
| jperm_bld.md | https://jperm.net/bld | the Old Pochmann parity algorithm verbatim: R U' R' U' R U R D R' U' R D' R' U2 R' U' with the usage claim "use between solving corners and solving edges, if odd letters for both", matching shipped row op.08 token for token; also the full example solve 2 used as the end to end oracle (scramble, 13 edge setups, parity, 9 corner setups); also the corner swap claim for op.02 (swaps LBU with DFR) |
| jperm_op_video.md | https://www.youtube.com/watch?v=A64Sy4WKiWY | J Perm's Old Pochmann tutorial description: the same parity algorithm verbatim, the FAQ restating when to apply it, and the statement that OP handles flipped and twisted pieces as new cycles with no dedicated algorithm ("they're just like new cycles", "solve everything else, and the buffer will automatically be solved") |
| jperm_m2_video.md | https://www.youtube.com/watch?v=JTkSQxWk_u8 | J Perm's M2 tutorial description: the M2 parity companions, option 1 M2 U' M U2 M U' M' U2 M' U2 and option 2 D' L2 D M2 D' L2 D, with the usage claim, plus the 11-target example solve 1 (scramble L' D2 B2 D2 B D F D R' F' U D L2 B2 D B2 L2 B2 U' B2 R2, memo MO WE CL XA RJ P) and the full setup table used to replay it; transcript excerpts state the effect claim: after odd edges the M slice is off by one, the parity algorithm leaves all edges solved except the two that Old Pochmann corner solving then swaps an odd number of times |
| pochmann_m2r2.md | http://www.stefan-pochmann.de/spocc/blindsolving/M2R2/ | the M2/R2 inventor page, live (HTTP 200, footer last modified 2007-11-08), cross-checked identical against the archive.org snapshot: the orient edges list containing M' U M' U M' U2 M U M U M U2, the orient corners list whose third entry is (L' U' L U' L' U2 L) (R U R' U R U2 R'), the claim that entries exist to flip misoriented edges and twist misoriented corners after setup moves, and the odd parity algorithm of the combined M2+R2 method |
| cubefreak_m2.md | http://www.cubefreak.net/bld/m2_guide.html | the M2 guide's endgame rules: orient the inactive flipped edges, orienting in addition the DF edge when the inactive count is odd, and example 2's step F2-M'UM'UM'U2MUMUMU2-F2 stated to orient the flipped edges leaving the (DF DR) 2-cycle with correct orientation; example 1 states its pairs leave the edges fully solved; contains the pair 4 erratum recorded in the provenance |
| speedsolving_m2.md | https://www.speedsolving.com/wiki/index.php/M2 | the wiki M2 page, live: the 22-row target table already shipped as m2.01 to m2.22, no parity companion and no buffer flip case anywhere on the page, and the external link that points to J Perm's M2 tutorial as the companion source |
| speedsolving_oldpochmann.md | https://www.speedsolving.com/wiki/index.php/Old_Pochmann | the wiki Old Pochmann page, live: no parity companion and no corner twist case documented, the page describes the swap method and the 2x2 corner variant only |
| bldtrainer.md | https://bldtrainer.com/parity, /algs, /corners | bldtrainer's parity lesson and algorithm library: its parity companion R U R' F' R U R' U' R' F R2 U' R' U' with the explicit effect claim "swaps the leftover two edges and two corners (no centers move)", its UF edge buffer and UBL corner buffer matching this app's pinned buffers, its Y perm F R U' R' U' R U R' F' R U R' U' R' F R F' claimed to swap UBL with UFR, and its corners lesson documenting the twisted corner handling as a cycle break with no dedicated algorithm |

## fetch log

- https://jperm.net/bld: direct fetch from the build machine failed, the web reader fetch succeeded, full page captured, parity text identical to the archived playground/seeds_bld/jperm_bld.md from the m5 pass.
- https://www.youtube.com/watch?v=A64Sy4WKiWY and https://www.youtube.com/watch?v=JTkSQxWk_u8: descriptions and transcript captured through the reader.
- http://www.stefan-pochmann.de/spocc/blindsolving/3x3/m2.php: 404, the M2R2 directory page is the live location, confirmed HTTP 200.
- http://www.cubefreak.net/bld/m2_guide.html: live capture succeeded.
- https://bldtrainer.com: routes discovered from the raw homepage HTML (/parity, /algs, /corners, not /cubing/*), all three captured.
- https://web.archive.org/web/20090202214856/http://www.stefan-pochmann.de/spocc/blindsolving/M2R2/: captured as a cross-check, content identical to the live page.

## simulation

build: clang --target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -Isrc playground/parity_flip_twist/parity_flip_probe.c src/engine/cube.c src/engine/notation.c src/gen/targets.c

run: playground/parity_flip_twist/parity_flip_probe.exe, full output in sim_output.txt, exit 0, every check green.

verified effects, each simulated from a solved cube with identity facelets through the real engine:

- op.08 R U' R' U' R U R D R' U' R D' R' U2 R' U': swaps UB with UL and UBR with UFR, all oriented, centers fixed, odd permutation in both orbits. end to end, J Perm /bld example solve 2 replays to a solved cube.
- op.01 and op.02 swap claims reverified sticker exact, op.02's edge side effect recorded as (UL UB), the pair the corner phase swaps.
- m2 parity option 1 M2 U' M U2 M U' M' U2 M' U2 and option 2 D' L2 D M2 D' L2 D: identical effect, cycles (UB UL)(UF DB) with the M slice centers rotated back. end to end, J Perm M2 example solve 1 with either option leaves the edges solved except exactly the (UL UB) pair equal to op.02's side effect, corners untouched by the edge phase, centers solved, matching the transcript claim.
- M2 flip base M' U M' U M' U2 M U M U M U2: pure flip of UB and UF, no permutation, corners and centers restored. its F2 conjugate flips UB and DF, the DF buffer itself. cubefreak example 1 replays to fully solved edges and centers. cubefreak example 2 with the recorded erratum corrected replays to exactly the claimed (DF DR) 2-cycle with no flipped edges remaining.
- corner twist (L' U' L U' L' U2 L) (R U R' U R U2 R'): pure twist of UBR and UFR in opposite directions, no permutation, edges and centers restored. the other three Pochmann orient corners entries simulated and recorded, entries 1 and 2 twist UBR and DFR, the R2 method's own buffers.
- bldtrainer parity Jb R U R' F' R U R' U' R' F R2 U' R' U': swaps UF with UR sticker exact plus UBR with UFR, centers fixed, exactly its printed claim, and UF to UR is this app's pinned edge buffer to target swap.
- Pochmann M2R2 odd parity (r2' U' r2)(R' U)(L' U2')(R U' R' U2 R)(L U')(r2' U): observation only, permutes (URF DRB) corners, (UR DR)(UF DB)(BR FR) edges with center rotation, it is the combined M2+R2 parity, not the OP companion.
