import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ALLOWLIST_PATH = ROOT / "tests" / "mutant_allowlist.txt"
REPORT_PATH = ROOT / "playground" / "allowlist_lint_report.txt"

PUNCT3 = ("<<=", ">>=", "...")
PUNCT2 = ("->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
          "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "##")
MUT1 = set("+-*/<>!")
MUT2 = {"<=", ">=", "==", "!=", "&&", "||"}
SUFFIX = "uUlL"
STOPWORDS = {"true", "false"}
LIBC = {"errno", "memset", "memcpy", "memmove", "snprintf", "strlen", "strncmp", "strcmp",
        "strchr", "strrchr", "strtol", "strtod", "opendir", "readdir", "closedir", "remove",
        "malloc", "calloc", "realloc", "free", "fopen", "fclose", "fread", "fwrite", "fgets",
        "fseek", "ftell", "rewind", "gmtime", "localtime", "isdigit", "isalpha", "isalnum",
        "isfinite", "isnan", "vsnprintf", "qsort", "exit", "stat", "getenv", "putenv"}
IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
ROW_RE = re.compile(r"^(\S+):(\d+)\t(.*)$")
WINDOW = 3
TRIM = 120


def process_source(text):
    n = len(text)
    i = 0
    out = []
    cur = []
    in_block = False
    in_preproc = False
    seen_token = False
    while i < n:
        c = text[i]
        if c == "\n":
            cont = in_preproc and i > 0 and text[i - 1] == "\\"
            out.append(("".join(cur), in_preproc))
            cur = []
            in_preproc = cont
            seen_token = False
            i += 1
            continue
        if in_preproc:
            i += 1
            continue
        if in_block:
            if c == "*" and i + 1 < n and text[i + 1] == "/":
                in_block = False
                seen_token = True
                cur.append(" ")
                i += 2
            else:
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            in_block = True
            i += 2
            continue
        if c == "#" and not seen_token:
            in_preproc = True
            i += 1
            continue
        if c == '"' or c == "'":
            q = c
            j = i + 1
            while j < n:
                if text[j] == "\\" and j + 1 < n:
                    j += 2
                    continue
                if text[j] == q or text[j] == "\n":
                    break
                j += 1
            end = min(j + 1, n)
            cur.append(text[i:end])
            seen_token = True
            i = end
            continue
        cur.append(c)
        if not c.isspace():
            seen_token = True
        i += 1
    out.append(("".join(cur), in_preproc))
    return out


def is_int_literal(tok):
    core = tok.rstrip(SUFFIX)
    if not core or not tok[0].isdigit():
        return False
    if core[:2].lower() == "0x":
        hexpart = core[2:]
        return len(hexpart) > 0 and all(ch in "0123456789abcdefABCDEF" for ch in hexpart)
    return all(ch.isdigit() for ch in core)


def scan_code(code):
    idents = set()
    mutable = False
    i = 0
    n = len(code)
    while i < n:
        c = code[i]
        if c.isspace():
            i += 1
            continue
        if c == '"' or c == "'":
            q = c
            i += 1
            while i < n:
                if code[i] == "\\" and i + 1 < n:
                    i += 2
                    continue
                if code[i] == q or code[i] == "\n":
                    break
                i += 1
            i += 1
            continue
        if c.isalpha() or c == "_":
            j = i + 1
            while j < n and (code[j].isalnum() or code[j] == "_"):
                j += 1
            word = code[i:j]
            idents.add(word)
            if word in ("true", "false"):
                mutable = True
            i = j
            continue
        if c.isdigit() or (c == "." and i + 1 < n and code[i + 1].isdigit()):
            j = i + 1
            while True:
                while j < n and (code[j].isalnum() or code[j] == "."):
                    j += 1
                if j < n and code[j] in "+-" and j > i and code[j - 1] in "eEpP":
                    j += 1
                    continue
                break
            if is_int_literal(code[i:j]):
                mutable = True
            i = j
            continue
        if code[i:i + 3] in PUNCT3:
            i += 3
            continue
        if code[i:i + 2] in PUNCT2:
            if code[i:i + 2] in MUT2:
                mutable = True
            i += 2
            continue
        if c in MUT1:
            mutable = True
        i += 1
    return idents, mutable


def load_lines(path):
    text = path.read_text(encoding="utf-8", errors="replace")
    out = []
    for code, pre in process_source(text):
        idents, mutable = scan_code(code)
        out.append({"code": code.strip(), "pre": pre, "idents": idents, "mut": mutable})
    return out


def build_vocab(paths):
    vocab = set()
    for path in paths:
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for code, _ in process_source(text):
            idents, _ = scan_code(code)
            vocab.update(idents)
    return vocab


def code_shaped(word):
    if "_" in word or any(ch.isdigit() for ch in word):
        return True
    if any(ch.isupper() for ch in word[1:]):
        return True
    if len(word) >= 3 and word.isupper() and any(ch.isalpha() for ch in word):
        return True
    return word in LIBC


def window_has(window, word):
    if word in window:
        return True
    prefix = word + "_"
    return any(tok.startswith(prefix) for tok in window)


def classify(row, lines, vocab):
    idx = row["line"] - 1
    if lines is None:
        return "DEAD", "source file missing", ""
    if idx < 0 or idx >= len(lines):
        return "DEAD", "line past end of file", ""
    entry = lines[idx]
    if entry["pre"]:
        return "DEAD", "preprocessor line", entry["code"]
    if not entry["code"]:
        return "DEAD", "blank or comment only", ""
    if not entry["mut"]:
        return "DEAD", "no mutable token on line", entry["code"]
    window = set()
    for k in range(max(0, idx - WINDOW), min(len(lines), idx + WINDOW + 1)):
        window.update(lines[k]["idents"])
    for word in IDENT_RE.findall(row["just"]):
        if len(word) < 2 or word in STOPWORDS or word not in vocab or not code_shaped(word):
            continue
        if not window_has(window, word):
            return "SUSPECT", word, entry["code"]
    return "OK", "", entry["code"]


def main():
    rows = []
    malformed = 0
    for raw in ALLOWLIST_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        if not raw.strip():
            continue
        m = ROW_RE.match(raw)
        if m is None:
            malformed += 1
            continue
        rows.append({"path": m.group(1), "line": int(m.group(2)), "just": m.group(3)})
    file_order = []
    for row in rows:
        if row["path"] not in file_order:
            file_order.append(row["path"])
    lines_by_file = {}
    for rel in file_order:
        path = ROOT / rel
        lines_by_file[rel] = load_lines(path) if path.is_file() else None
    src_paths = sorted((ROOT / "src").rglob("*.c")) + sorted((ROOT / "src").rglob("*.h"))
    vocab = build_vocab(src_paths)
    for row in rows:
        verdict, detail, code = classify(row, lines_by_file[row["path"]], vocab)
        row["verdict"] = verdict
        row["detail"] = detail
        row["code"] = code
    per_file = {rel: {"rows": 0, "OK": 0, "DEAD": 0, "SUSPECT": 0} for rel in file_order}
    for row in rows:
        per_file[row["path"]]["rows"] += 1
        per_file[row["path"]][row["verdict"]] += 1
    with REPORT_PATH.open("w", encoding="utf-8") as out:
        for rel in file_order:
            counts = per_file[rel]
            if counts["DEAD"] == 0 and counts["SUSPECT"] == 0:
                continue
            out.write(f"{rel}  rows={counts['rows']} ok={counts['OK']} dead={counts['DEAD']} "
                      f"suspect={counts['SUSPECT']}\n")
            for row in rows:
                if row["path"] != rel or row["verdict"] == "OK":
                    continue
                code = row["code"][:TRIM]
                if row["verdict"] == "DEAD":
                    out.write(f"  DEAD   :{row['line']} [{row['detail']}] {code}\n")
                else:
                    out.write(f"  SUSPECT:{row['line']} missing={row['detail']} {code}\n")
        total_dead = sum(c["DEAD"] for c in per_file.values())
        total_suspect = sum(c["SUSPECT"] for c in per_file.values())
        out.write(f"TOTAL rows={len(rows)} ok={len(rows) - total_dead - total_suspect} "
                  f"dead={total_dead} suspect={total_suspect} malformed={malformed}\n")
    for rel in file_order:
        c = per_file[rel]
        print(f"{rel}  rows={c['rows']} ok={c['OK']} dead={c['DEAD']} suspect={c['SUSPECT']}")
    total_dead = sum(c["DEAD"] for c in per_file.values())
    total_suspect = sum(c["SUSPECT"] for c in per_file.values())
    worst = sorted(file_order, key=lambda rel: -(per_file[rel]["DEAD"] + per_file[rel]["SUSPECT"]))[:5]
    print(f"TOTAL rows={len(rows)} ok={len(rows) - total_dead - total_suspect} "
          f"dead={total_dead} suspect={total_suspect} malformed={malformed}")
    print("worst5:")
    for rel in worst:
        c = per_file[rel]
        print(f"  {rel} dead={c['DEAD']} suspect={c['SUSPECT']}")
    print(f"report: {REPORT_PATH}")


if __name__ == "__main__":
    main()
