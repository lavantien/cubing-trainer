ALLOW = "tests/mutant_allowlist.txt"

COMBINED = {
    "src/ui/widgets.c:61": "atlas texture id is an opaque gpu handle, the default atlas takes id 1 and the custom atlas loads above it so the mutated literal never separates the branch, loadfontex returns a populated atlas or a fully zeroed font so both or operands flip together, and glyphcount sits at the 576 codepoint budget on every reachable load so the widened, lowered, and minus one bounds keep every comparison outcome",
    "src/ui/widgets.c:73": "zero id never reaches the guard with a live window, pre window a guisetfont of the unbound font normalizes back to the default inside guigetfont, and the default atlas id 1 with every custom atlas id above it keeps the raised bound from changing the bind decision",
    "src/ui/widgets.c:99": "count minus one and cap minus one both clamp n to the same minus one and return it through the empty loop",
}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    out = []
    seen = set()
    for ln in lines:
        key = ln.split("\t")[0]
        if key in COMBINED:
            if key not in seen:
                seen.add(key)
                out.append(f"{key}\t{COMBINED[key]}")
        else:
            out.append(ln)
    keys = [ln.split("\t")[0] for ln in out]
    dups = sorted({k for k in keys if keys.count(k) > 1})
    with open(ALLOW, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(out) + "\n")
    print(f"total {len(out)}, dups after fix: {dups if dups else 'none'}")


if __name__ == "__main__":
    main()
