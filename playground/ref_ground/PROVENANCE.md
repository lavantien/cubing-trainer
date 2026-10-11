# provenance

retrieval date 2026-10-10. files provided by the repo owner under ref/, unmodified, hashes verified with sha256 at dump time (dump_all.py prints the check).

| file | sha-256 | dump |
| --- | --- | --- |
| ref/Tommy Cherry's 3BLD algs.xlsx | DE60EFB66F974C123A84004E214D691BA2762B71718A9BF2F764BCA0B33C466A | cherry/ |
| ref/Charlie Eggins's 3BLD algs.xlsx | 3C1BECF9C4ABA50C1945D8513E78F34314B7FDAE767112847ABEDB9F57AA9B68 | eggins/ |

origin: user-provided exports of the blddb project (github.com/nbwzx/blddb, GPL-3.0, author Zixing Wang), auto-collected per solver from youtube solve reconstructions by scripts/youtube.py on branch v2. solver identities per the readme sheets: Tommy Cherry, multiple 3BLD world records, channel https://www.youtube.com/@TommyCherry. Charlie Eggins, Oceanic record holder, channel https://www.youtube.com/@swiftcubing.

conversion: playground/ref_ground/dump_all.py, python -I, stdlib zipfile + ElementTree, run once 2026-10-10. one tsv per sheet, utf-8, tab-separated, every physical row in sheet order, all 5 columns verbatim including trailing padding spaces in header cells, empty cells empty. no cell in either workbook contains a tab, newline, carriage return, or double quote, so no quoting was needed. hyperlink targets are stored as hyperlink relationships in the xlsx; sidecar files cherry/hyperlinks.tsv and eggins/hyperlinks.tsv (dump_hyperlinks.py) carry sheet, cell, display text, target for all 960 cherry and 121 eggins hyperlinks. in the data sheets the display text equals the url; the only display-text-only cell is the readme youtube channel label.

per-sheet row counts (data rows excluding the header row):

| sheet | cherry | eggins |
| --- | --- | --- |
| Readme | 999 physical, 9 nonempty | 12 physical, 11 nonempty |
| edge | 551 | 71 |
| corner | 328 | 38 |
| flips | 25 | 0 (header only) |
| twists | 20 | 5 |
| parity | 26 | 2 |
| ltct | 8 | 3 |

## cherry readme sheet, full text

lines in sheet order, empty rows omitted, `->` marks a hyperlink cell with its target:

```text
Tommy Cherry's 3BLD algs
Automatically collected from the reconstructions of Tommy Cherry's youtube videos by Zixing Wang.
Algs for some cases are missing since they don't appear in any reconstructions.
Tommy Cherry - YouTube -> https://www.youtube.com/@TommyCherry
Floating order:
edge    UF UB UR UL FR FL DF DB DR DL
corner  UFR UFL UBR UBL DFR DFL
NOTE: This file should not be edited. If you want to edit, make a copy of it and edit the copy.
Github  https://github.com/nbwzx/blddb/blob/v2/scripts/youtube.py
```

## eggins readme sheet, full text

```text
Charlie Eggins's 3BLD algs
Automatically collected from the reconstructions of Charlie Eggins's youtube videos by Zixing Wang.
Algs for some cases are missing since they don't appear in any reconstructions.
Swift Cubing - Charlie Eggins Speedcuber - YouTube -> https://www.youtube.com/@swiftcubing
Floating order:
edge    UF UB UR UL FR FL DF DB DR DL
corner  UFR UFL UBR UBL DFR DFL
NOTE: This file should not be edited. If you want to edit, make a copy of it and edit the copy.
Github  https://github.com/nbwzx/blddb/blob/v2/scripts/youtube.py
cancel into
lockup second R'
```

the last two stray rows (cancel into / lockup second R') are present in the source file, captured verbatim. the eggins readme youtube row is a hyperlink cell, target https://www.youtube.com/@swiftcubing.
