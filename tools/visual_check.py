import os
import struct
import sys
import zlib

AHASH_GRID = 8
MAX_HAMMING = 4
MAX_MEAN_DELTA = 0.25
SELF_CHECK_SIZE = 64


def decode_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a png")
    pos = 8
    width = height = None
    bit_depth = color_type = None
    idat = bytearray()
    palette = []
    while pos < len(data):
        length = struct.unpack(">I", data[pos : pos + 4])[0]
        ctype = data[pos + 4 : pos + 8]
        chunk = data[pos + 8 : pos + 8 + length]
        if ctype == b"IHDR":
            width, height, bit_depth, color_type = struct.unpack(">IIBB", chunk[:10])
        elif ctype == b"PLTE":
            palette = [tuple(chunk[i : i + 3]) for i in range(0, len(chunk), 3)]
        elif ctype == b"IDAT":
            idat.extend(chunk)
        elif ctype == b"IEND":
            break
        pos += 12 + length
    if width is None or bit_depth != 8:
        raise ValueError(f"{path}: unsupported png (bit depth {bit_depth})")
    if color_type not in (0, 2, 3, 4, 6):
        raise ValueError(f"{path}: unsupported color type {color_type})")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color_type]
    raw = zlib.decompress(bytes(idat))
    stride = width * channels
    prev = bytearray(stride)
    rows = []
    p = 0
    for _ in range(height):
        ftype = raw[p]
        p += 1
        row = bytearray(raw[p : p + stride])
        p += stride
        if ftype == 1:
            for i in range(channels, stride):
                row[i] = (row[i] + row[i - channels]) & 0xFF
        elif ftype == 2:
            for i in range(stride):
                row[i] = (row[i] + prev[i]) & 0xFF
        elif ftype == 3:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                row[i] = (row[i] + (left + prev[i]) // 2) & 0xFF
        elif ftype == 4:
            for i in range(stride):
                a = row[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                row[i] = (row[i] + pr) & 0xFF
        prev = row
        rows.append(row)
    pixels = []
    for row in rows:
        out_row = []
        for x in range(width):
            base = x * channels
            if color_type == 0:
                g = row[base]
                out_row.append((g, g, g, 255))
            elif color_type == 2:
                out_row.append((row[base], row[base + 1], row[base + 2], 255))
            elif color_type == 3:
                r, g, b = palette[row[base]]
                out_row.append((r, g, b, 255))
            elif color_type == 4:
                g = row[base]
                out_row.append((g, g, g, row[base + 1]))
            else:
                out_row.append((row[base], row[base + 1], row[base + 2], row[base + 3]))
        pixels.append(out_row)
    return width, height, pixels


def encode_png(width, height, pixels):
    def chunk(ctype, body):
        return (
            struct.pack(">I", len(body))
            + ctype
            + body
            + struct.pack(">I", zlib.crc32(ctype + body) & 0xFFFFFFFF)
        )

    raw = bytearray()
    for row in pixels:
        raw.append(0)
        for px in row:
            raw.extend(px[:4])
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def luma(px):
    return 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2]


def ahash(width, height, pixels):
    cells = []
    for by in range(AHASH_GRID):
        y0 = by * height // AHASH_GRID
        y1 = (by + 1) * height // AHASH_GRID
        for bx in range(AHASH_GRID):
            x0 = bx * width // AHASH_GRID
            x1 = (bx + 1) * width // AHASH_GRID
            total = 0.0
            for y in range(y0, y1):
                for x in range(x0, x1):
                    total += luma(pixels[y][x])
            cells.append(total / ((y1 - y0) * (x1 - x0)))
    mean = sum(cells) / len(cells)
    bits = 0
    for i, value in enumerate(cells):
        if value > mean:
            bits |= 1 << i
    return bits


def hamming(a, b):
    return bin(a ^ b).count("1")


def mean_deltas(pa, pb):
    w = min(len(pa[0]), len(pb[0]))
    h = min(len(pa), len(pb))
    sums_a = [0, 0, 0]
    sums_b = [0, 0, 0]
    for y in range(h):
        for x in range(w):
            for c in range(3):
                sums_a[c] += pa[y][x][c]
                sums_b[c] += pb[y][x][c]
    n = w * h
    return [abs(sums_a[c] - sums_b[c]) / n for c in range(3)]


def compare(golden_path, candidate_path):
    gw, gh, gp = decode_png(golden_path)
    cw, ch, cp = decode_png(candidate_path)
    if gw != cw or gh != ch:
        return False, -1, [255.0, 255.0, 255.0]
    ha = ahash(gw, gh, gp)
    hb = ahash(cw, ch, cp)
    dist = hamming(ha, hb)
    deltas = mean_deltas(gp, cp)
    delta = max(deltas)
    return dist <= MAX_HAMMING and delta <= MAX_MEAN_DELTA, dist, deltas


def blank(width, height, value):
    return [[(value, value, value, 255) for _ in range(width)] for _ in range(height)]


def self_check():
    size = SELF_CHECK_SIZE
    failures = []
    os.makedirs("build", exist_ok=True)

    base = blank(size, size, 20)
    for y in range(size):
        base[y][size - 1] = (240, 240, 240, 255)
    base_png = encode_png(size, size, base)
    open("build/visual_selfcheck_base.png", "wb").write(base_png)

    identical_png = base_png
    open("build/visual_selfcheck_identical.png", "wb").write(identical_png)

    shifted = blank(size, size, 20)
    shifted_png = encode_png(size, size, shifted)
    open("build/visual_selfcheck_shifted.png", "wb").write(shifted_png)

    recolored = blank(size, size, 20)
    for y in range(size // 2):
        for x in range(size // 5):
            recolored[y][x] = (240, 240, 240, 255)
    recolored_png = encode_png(size, size, recolored)
    open("build/visual_selfcheck_recolored.png", "wb").write(recolored_png)

    near = [row[:] for row in base]
    near[0][0] = (21, 21, 21, 255)
    near_png = encode_png(size, size, near)
    open("build/visual_selfcheck_near.png", "wb").write(near_png)

    ok, dist, deltas = compare_bytes(base_png, identical_png)
    if not ok or dist != 0 or max(deltas) > 0.0:
        failures.append(f"identical pair must pass exactly, got dist={dist} deltas={deltas}")

    ok, dist, deltas = compare_bytes(base_png, shifted_png)
    if ok:
        failures.append(f"1px shift must fail, got pass dist={dist} deltas={deltas}")

    ok, dist, deltas = compare_bytes(base_png, recolored_png)
    if ok:
        failures.append(f"10 percent recolor must fail, got pass dist={dist} deltas={deltas}")

    ok, dist, deltas = compare_bytes(base_png, near_png)
    if not ok:
        failures.append(f"single near pixel must pass, got fail dist={dist} deltas={deltas}")

    return failures


def compare_bytes(a, b):
    open("build/visual_selfcheck_a.png", "wb").write(a)
    open("build/visual_selfcheck_b.png", "wb").write(b)
    return compare("build/visual_selfcheck_a.png", "build/visual_selfcheck_b.png")


def main():
    if len(sys.argv) == 2 and sys.argv[1] == "--selfcheck":
        failures = self_check()
        for f in failures:
            print(f"SELF CHECK FAIL: {f}")
        print(f"self check {'failed' if failures else 'passed'}: {len(failures)} failures")
        return 1 if failures else 0
    if len(sys.argv) != 3:
        print("usage: python -I tools/visual_check.py <golden.png> <candidate.png>")
        print("       python -I tools/visual_check.py --selfcheck")
        return 2
    ok, dist, deltas = compare(sys.argv[1], sys.argv[2])
    delta = max(deltas)
    verdict = "pass" if ok else "fail"
    print(
        f"{verdict}: hamming={dist}/{AHASH_GRID * AHASH_GRID} "
        f"mean_delta_r={deltas[0]:.3f} g={deltas[1]:.3f} b={deltas[2]:.3f} "
        f"thresholds={MAX_HAMMING}/{MAX_MEAN_DELTA}"
    )
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
