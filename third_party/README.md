# vendored sources

Fetched by `make vendor` from pinned upstream releases. Extraction strips the top-level archive directory.

| package | version | source | retrieved |
| --- | --- | --- | --- |
| raylib | 6.0 | https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz | 2026-10-09 |
| raygui | 5.0 | https://github.com/raysan5/raygui/archive/refs/tags/5.0.tar.gz | 2026-10-09 |
| sqlite amalgamation | 3.53.4 | https://sqlite.org/2026/sqlite-amalgamation-3530400.zip | 2026-10-09 |

sqlite archive published SHA3-256: 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e. `make vendor` verifies the archive with python hashlib SHA3-256 when python is reachable, falling back to compiling sqlite3.c and asserting the reported library version is 3.53.4. The method that ran for the cold start is recorded in playground/spike_link/REPORT.md.
