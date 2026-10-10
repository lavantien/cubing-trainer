import hashlib
import pathlib
import re
import sys

USAGE = "usage: python -I tools/vendor_verify.py <archive> <expected-sha3-256>"
HEX64 = re.compile(r"[0-9a-f]{64}")
MALFORMED = "expected sha3-256 must be 64 hex digits"


def verify_archive(path, expected):
    if not HEX64.fullmatch(expected):
        return 2, f"vendor_verify: {MALFORMED}: {expected}"
    archive = pathlib.Path(path)
    try:
        got = hashlib.sha3_256(archive.read_bytes()).hexdigest()
    except OSError as exc:
        return 2, f"vendor_verify: cannot read {path}: {exc}"
    if got != expected:
        return 1, f"vendor_verify: {path} sha3-256 mismatch: got {got} want {expected}"
    return 0, f"vendor_verify: {path} sha3-256 verified"


def main():
    if len(sys.argv) != 3:
        print(USAGE)
        return 2
    code, line = verify_archive(sys.argv[1], sys.argv[2])
    print(line)
    return code


if __name__ == "__main__":
    sys.exit(main())
