import hashlib
import importlib.util
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTDIR = os.path.join(ROOT, "build", "vendor_selftest")

RAYLIB_LIKE = b"raylib archive bytes"
RAYGUI_LIKE = b"raygui archive bytes"
SQLITE_LIKE = b"sqlite archive bytes"


def load_module():
    spec = importlib.util.spec_from_file_location(
        "vendor_verify", os.path.join(ROOT, "tools", "vendor_verify.py")
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def digest(data):
    return hashlib.sha3_256(data).hexdigest()


def write(name, data):
    path = os.path.join(OUTDIR, name)
    with open(path, "wb") as handle:
        handle.write(data)
    return path


def expect(name, got, want):
    status = "PASS" if got == want else "FAIL"
    print(f"{status} {name}: got {got!r} want {want!r}")
    return got == want


def main():
    vv = load_module()
    os.makedirs(OUTDIR, exist_ok=True)
    results = []

    raylib = write("raylib.tar.gz", RAYLIB_LIKE)
    raygui = write("raygui.tar.gz", RAYGUI_LIKE)
    sqlite = write("sqlite.zip", SQLITE_LIKE)
    corrupt = write("corrupt.tar.gz", RAYGUI_LIKE[:-1] + bytes([RAYGUI_LIKE[-1] ^ 1]))

    results.append(
        expect(
            "raylib_match",
            vv.verify_archive(raylib, digest(RAYLIB_LIKE)),
            (0, f"vendor_verify: {raylib} sha3-256 verified"),
        )
    )
    results.append(
        expect(
            "raygui_match",
            vv.verify_archive(raygui, digest(RAYGUI_LIKE)),
            (0, f"vendor_verify: {raygui} sha3-256 verified"),
        )
    )
    results.append(
        expect(
            "sqlite_match",
            vv.verify_archive(sqlite, digest(SQLITE_LIKE)),
            (0, f"vendor_verify: {sqlite} sha3-256 verified"),
        )
    )

    code, line = vv.verify_archive(corrupt, digest(RAYGUI_LIKE))
    results.append(expect("corrupt_mismatch_code", code, 1))
    results.append(expect("corrupt_mismatch_names_got", "got " + digest(RAYGUI_LIKE[:-1] + bytes([RAYGUI_LIKE[-1] ^ 1])) in line, True))
    results.append(expect("corrupt_mismatch_names_want", "want " + digest(RAYGUI_LIKE) in line, True))

    code, _ = vv.verify_archive(raylib, digest(SQLITE_LIKE))
    results.append(expect("wrong_expected_code", code, 1))

    code, line = vv.verify_archive(raylib, "not-a-hash")
    results.append(expect("malformed_expected_code", code, 2))
    results.append(expect("malformed_expected_message", "expected sha3-256 must be 64 hex digits" in line, True))

    code, line = vv.verify_archive("0" * 64, digest(RAYLIB_LIKE))
    results.append(expect("malformed_expected_digits_only_code", code, 2))

    code, line = vv.verify_archive(os.path.join(OUTDIR, "missing.tar.gz"), digest(RAYLIB_LIKE))
    results.append(expect("missing_archive_code", code, 2))
    results.append(expect("missing_archive_names_file", "missing.tar.gz" in line, True))

    passed = sum(results)
    total = len(results)
    print(f"vendor_verify selftest: {passed}/{total} cases met expectations")
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
