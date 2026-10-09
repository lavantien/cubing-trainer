import struct
import sys
import zlib


def decode_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a png")
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
    return width, height, channels, rows, palette


def pixel(rows, channels, x, y):
    row = rows[y]
    if channels == 4:
        return tuple(row[x * 4 : x * 4 + 4])
    if channels == 3:
        return tuple(row[x * 3 : x * 3 + 3]) + (255,)
    return (row[x], row[x], row[x], 255)


def main():
    path = sys.argv[1]
    width, height, channels, rows, palette = decode_png(path)
    counts = {}
    for y in range(height):
        for x in range(width):
            px = pixel(rows, channels, x, y)
            counts[px] = counts.get(px, 0) + 1
    top = sorted(counts.items(), key=lambda kv: -kv[1])[:12]
    print(f"{path} {width}x{height} channels={channels} distinct={len(counts)}")
    for px, n in top:
        share = 100.0 * n / (width * height)
        print(f"  rgba{px} {n} ({share:.2f}%)")


if __name__ == "__main__":
    main()
