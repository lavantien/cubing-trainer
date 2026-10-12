Runtime resources for the app, resolved two directories up from the exe.

assets/fonts is produced by `make vendor`: it downloads the pinned nerd-fonts
v3.5.1 Iosevka.zip, verifies it by SHA3-256, and extracts only
IosevkaNerdFont-Regular.ttf. The archive URL and hash live in the Makefile,
and the source records table in README.md carries the provenance row. The
directory is gitignored like third_party, so a fresh clone runs `make vendor`
before `make run`.
