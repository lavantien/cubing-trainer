import subprocess
import sys

SCOPE = "src/app/script.c"


def load_diff():
    out = subprocess.run(
        ["git", "show", "ad2b72f", "--", SCOPE], capture_output=True, text=True, check=True
    ).stdout
    hunks = []
    for line in out.splitlines():
        if line.startswith("@@"):
            head = line.split("@@")[1]
            old, new = head.split()[0], head.split()[1]
            o_start = int(old.split(",")[0].lstrip("-"))
            n_start = int(new.split(",")[0].lstrip("+"))
            hunks.append({"o": o_start, "n": n_start, "body": []})
        elif hunks and line[:1] in (" ", "-", "+"):
            hunks[-1]["body"].append(line)
    return hunks


def build_map(hunks, old_len):
    old_to_new = {}
    dead = set()
    o = 1
    n = 1
    for h in hunks:
        while o < h["o"]:
            old_to_new[o] = n
            o += 1
            n += 1
        for line in h["body"]:
            if line.startswith("\\"):
                continue
            if line.startswith(" "):
                old_to_new[o] = n
                o += 1
                n += 1
            elif line.startswith("-"):
                dead.add(o)
                o += 1
            elif line.startswith("+"):
                n += 1
    while o <= old_len:
        old_to_new[o] = n
        o += 1
        n += 1
    return old_to_new, dead


def main():
    hunks = load_diff()
    old_len = int(
        subprocess.run(
            ["git", "show", "ad2b72f~1:src/app/script.c"], capture_output=True, text=True, check=True
        ).stdout.count("\n")
    )
    old_to_new, dead_lines = build_map(hunks, old_len)
    anchor = {44: 46, 498: 469, 643: 606, 828: 777, 853: 804, 862: 823, 33: 34}
    for o, want in anchor.items():
        got = old_to_new.get(o)
        print(f"anchor old {o} -> new {got} (expect {want})")
    new_len = int(
        subprocess.run(
            ["git", "show", "ad2b72f:src/app/script.c"], capture_output=True, text=True, check=True
        ).stdout.count("\n")
    )
    print(f"old_len {old_len} new_len {new_len} tail_map {old_to_new.get(old_len)}")
    rows = []
    with open("tests/mutant_allowlist.txt", encoding="utf-8") as f:
        for raw in f:
            if raw.startswith(SCOPE + ":"):
                key, text = raw.split("\t", 1)
                rows.append((int(key.split(":")[1]), text.rstrip("\n")))
    live, dead = [], []
    for old_line, text in rows:
        if old_line in dead_lines:
            dead.append((old_line, text))
        else:
            live.append((old_line, old_to_new[old_line], text))
    print(f"rows total {len(rows)} live {len(live)} dead {len(dead)}")
    for old_line, new_line, text in live:
        print(f"REKEY {old_line} -> {new_line}\t{text}")
    for old_line, text in dead:
        print(f"DEAD {old_line}\t{text}")


def reconcile(old_to_new, dead_lines):
    new_to_old = {}
    for o, n in old_to_new.items():
        new_to_old.setdefault(n, o)
    verdicts = {}
    accepted = set()
    with open("playground/eslice/mutate_script.log", encoding="utf-8") as f:
        for line in f:
            parts = line.split()
            if parts[0] in ("SURVIVED", "ACCEPTED", "KILLED") and len(parts) >= 4:
                mid, site = parts[1], parts[2]
                verdicts[mid] = (parts[0], site, " ".join(parts[3:]))
                if parts[0] == "ACCEPTED":
                    accepted.add(mid)
    rows = {}
    with open("tests/mutant_allowlist.txt", encoding="utf-8") as f:
        for raw in f:
            if raw.startswith(SCOPE + ":"):
                key, text = raw.split("\t", 1)
                rows[int(key.split(":")[1])] = text.rstrip("\n")
    unexplained = []
    for mid, (kind, site, desc) in verdicts.items():
        if kind == "SURVIVED" and mid not in accepted:
            unexplained.append((int(mid[1:]), int(site.split(":")[1]), desc))
    unexplained.sort()
    rekeyed, unmatched = [], []
    for num, new_line, desc in unexplained:
        old_line = new_to_old.get(new_line)
        if old_line is not None and old_line in rows:
            rekeyed.append((num, new_line, old_line, desc, rows[old_line]))
        else:
            unmatched.append((num, new_line, desc))
    print(f"unexplained {len(unexplained)} rekeyed {len(rekeyed)} unmatched {len(unmatched)}")
    for num, new_line, old_line, desc, text in rekeyed:
        print(f"MAPPED m{num:04d} new {new_line} (old {old_line}) [{desc}]")
    for num, new_line, desc in unmatched:
        print(f"NEWRULE m{num:04d} new {new_line} [{desc}]")
    for mid in ("m0095", "m0104"):
        if mid in verdicts:
            print(f"KICKED {mid} {verdicts[mid][1]} {verdicts[mid][2]}")


NEW_ROWS = {
    472: "review grade found flag initializer is a dead store, ct_script_grade_parse clears the out flag on entry before any read",
    609: "trainer event ok flag initializer is a dead store, ct_script_event_parse clears the out flag on entry before any read",
    610: "trainer method ok flag initializer is a dead store, ct_script_method_parse clears the out flag on entry before any read",
    826: "script run rc initializer is a dead store, the parse layer rejects zero command scripts with the empty script error so the loop always assigns rc before the return",
}


def apply_rekey(old_to_new, dead_lines):
    with open("tests/mutant_allowlist.txt", encoding="utf-8") as f:
        lines = f.readlines()
    out = []
    last_script_at = None
    kept = dropped = inserted = 0
    for raw in lines:
        if raw.startswith(SCOPE + ":"):
            key, text = raw.split("\t", 1)
            old_line = int(key.split(":")[1])
            if old_line in dead_lines:
                dropped += 1
                continue
            out.append(f"{SCOPE}:{old_to_new[old_line]}\t{text}")
            kept += 1
            last_script_at = len(out) - 1
        else:
            out.append(raw)
    for extra in sorted(NEW_ROWS):
        out.insert(last_script_at + 1, f"{SCOPE}:{extra}\t{NEW_ROWS[extra]}\n")
        last_script_at += 1
        inserted += 1
    with open("tests/mutant_allowlist.txt", "w", encoding="utf-8", newline="") as f:
        f.writelines(out)
    print(f"apply kept {kept} dropped {dropped} inserted {inserted}")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] in ("reconcile", "apply"):
        hunks = load_diff()
        old_len = int(
            subprocess.run(
                ["git", "show", "ad2b72f~1:src/app/script.c"],
                capture_output=True,
                text=True,
                check=True,
            ).stdout.count("\n")
        )
        old_to_new, dead_lines = build_map(hunks, old_len)
        if sys.argv[1] == "reconcile":
            reconcile(old_to_new, dead_lines)
        else:
            apply_rekey(old_to_new, dead_lines)
    else:
        sys.exit(main())
