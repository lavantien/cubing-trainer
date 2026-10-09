# packs sha256 fixtures

Generated 2026-10-10 for tests/unit_packs.c goldens.

Sources:

- RFC 6234, SHA-256 test vectors and constant tables: https://www.rfc-editor.org/rfc/rfc6234.html
  - "abc" -> ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad (test driver entry 1)
  - "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" (448-bit message) -> 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1 (entry 2)
  - "01234567" repeated 80 times, 640 bytes, exact multiple of 512 bits -> 594847328451bdfa85056225462cc1d867d877fb388df0ce35f25ab5562bfbb5 (entry 4)
  - H(0) and K constant tables used to cross-check the runtime prime-derivation in src/packs/provenance.c
- Empty string digest e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855: NIST FIPS 180-4 / RFC 6234 section 8.5
- No canonical NIST or IETF document lists the 64 x 'a' message; its digest was computed with two independent tools and is recorded here:
  - python 3 hashlib: `python -I -c "import hashlib; print(hashlib.sha256(b'a'*64).hexdigest())"`
  - certutil (windows CNG): `certutil -hashfile a64.bin SHA256`
  - both give ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb

Binaries:

- a64.bin: 64 bytes, value 'a' (block boundary case, data fills one block, padding spills into a second)
- rfc6234_test4.bin: 640 bytes, "01234567" x 80
- pat1000.bin: 1000 bytes, pat[i] = (i * 7 + 3) % 251, digest a9425c416f534025a4e2422bd14adba4ec3d4a68d10c3329be8df612964d2b6e from the same two tools

derive_probe.c: prints the prime-derived H and K tables; output matched the RFC 6234 tables word for word on 2026-10-10.
