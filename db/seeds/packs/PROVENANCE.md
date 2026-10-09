# pack seed provenance

Seed TSVs in this directory hold one row per case: case_key, alg, front_json, source_url, source_at, tab separated, single header line. The loader ct_pack_seed_file computes the per row content hash at load time, hashes are never stored in the TSVs. Retrieval date for every row is 2026-10-10. Where a source page lists several algorithms for one case the first suggested algorithm is stored.

## twoby2_cll.tsv

Source: https://jperm.net/algs/2x2/cll, J Perm's CLL page. The page renders its case table from https://jperm.net/lib/2x2cll.js, fetched and archived at playground/m5t2_jperm_cll.js. Retrieved 2026-10-10.

Page contents: the full 2x2 CLL set, 42 cases named L1 to L6, U1 to U6, T1 to T6, P1 to P6, H1 to H4, AS1 to AS6, S1 to S6, Adjacent, Diagonal, each with probability and one or more algorithm options.

Case key derivation: keys cll.01 to cll.42 follow the page order above, front_json carries the page case name. Generated programmatically from the archived data file by playground/m5t2_gen_seeds.py.

Count: 42 rows. Matches the plan's expected 42.

Discrepancy: the speedsolving wiki page https://www.speedsolving.com/wiki/index.php?title=CLL_algorithms_(2x2x2) is under construction with only the U and H sections filled, 34 of 42 algorithms present, so J Perm's complete list is the source of record.

## skewb_sarah.tsv

Source: https://sarah.cubing.net/skewb/skewb-guide.pdf, Sarah Strong's Speedskewbin guide, with case names cross read from https://sarah.cubing.net/skewb/my-method. Text and page renders archived at playground/m5t2_sarah_guide.txt and playground/m5t2_sarah_p1.png to m5t2_sarah_p11.png. Retrieved 2026-10-10.

Page contents: notation and the three variations. The intermediate variation is step 1 solve a side, step 2 solve the opposite side with 10 cases in 3 categories, step 3 solve the remaining centers with 3 cases. Step 1 lists 2 helper algorithms for the last first-layer corner.

Case key derivation: sarah.01 and sarah.02 are the step 1 corner helpers, the page shows them as two diagram positions without names, derived keys flc-a and flc-b. sarah.03 to sarah.05 are category 1 cases in page order, unnamed diagrams, derived keys cat1.1 to cat1.3. sarah.06 to sarah.09 are category 2 cases, named L2, L4, L3, L5 on the companion page. sarah.10 to sarah.12 are category 3 cases, named Pi5, Pi3, L1. sarah.13 to sarah.15 are the step 3 last 4 centers cases, named l4c-1 to l4c-3 on the companion page. Where a case offers a sledgehammer or hedgeslammer option the first listed option is stored.

Transcription note: the guide writes chains with an arrow glyph, R' F R F' followed by a category reference. Stored as ASCII, for example "R' F R F' -> cat. 1". The alg field is the source text for the case, the chain means do the trigger then finish with the referenced category.

Count: 15 rows, 2 step 1 helpers plus the 10 step 2 cases plus 3 step 3 cases. The intermediate variation itself defines the 10 step 2 cases, helpers and step 3 are included so the pack trains the full method.

Discrepancy: none, the plan named no count for this pack.

## pyraminx_l4e.tsv

Source: http://cubing.mikeg.me.uk/cubing-algs/html/l4e.html, Michael George's L4E page. The page renders from http://cubing.mikeg.me.uk/cubing-algs/data/l4e-min-005.js, fetched and archived at playground/m5t2_mikeg_l4e.js. Retrieved 2026-10-10.

Page contents: the L4E set for V first pyraminx, 36 cases in groups Down-Edge Solved, Down-Edge Flipped, Upper-Edge Solved, Upper-Edge Flipped, Nothing Placed, each with diagram, probability, and a suggested algorithm plus alternatives.

Case key derivation: keys l4e.01 to l4e.35 in page order DS2 to DS6, DF1 to DF6, US1 to US8, UF1 to UF8, NP1 to NP8, front_json carries the page case id. Generated programmatically from the archived data file by playground/m5t2_gen_seeds.py.

Count: 35 rows. The page lists 36 cases, DS1 is the solved state with an empty algorithm list and is excluded. This matches the 35 algs figure on the speedsolving wiki V First page.

Discrepancy: none versus the plan, which named no count.

## megaminx_4lll.tsv

Source: https://www.cubeskills.com/uploads/pdf/tutorials/intermediate-megaminx-techniques.pdf, Megaminx 4 Look Last Layer by Feliks Zemdegs and Andy Klise on CubeSkills. Extracted text archived at playground/m5t2_cubeskills_4lll.txt, page renders at playground/m5t2_4lll_p1.png to m5t2_4lll_p3.png. Retrieved 2026-10-10.

Page contents: a 3 page PDF with four sections, Edge Orientation, Corner Orientation, Edge Permutation, Corner Permutation. Each case is a diagram with one suggested algorithm, round brackets segment triggers.

Case key derivation: keys 4lll.01 to 4lll.39 in page reading order, EO1 to EO3, CO1 to CO16, EP1 to EP5, CP1 to CP15. The PDF prints no case names, the front_json names are section prefix plus position within the section. Algorithms that wrap across printed lines are joined with single spaces between all tokens.

Count: 39 rows, 3 EO plus 16 CO plus 5 EP plus 15 CP.

Discrepancy: the plan expected megaminx algs in R++ D-- style tokens. The sourced page uses R U R' U' style tokens with R2 and U2 doubles and BR wide turns, no ++ or -- tokens appear anywhere in the set. Source notation is stored verbatim. The plan named no count.
