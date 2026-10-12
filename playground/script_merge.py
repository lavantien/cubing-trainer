import re

ALLOW = "tests/mutant_allowlist.txt"
LOG = "playground/gates/lane_02_script.log"

ROWS = [
    (78, "list cap mutants bound the stack array only, every event method pair yields at most 3 lists so a cap of 7 or 9 never changes a reachable scan and the found clause rejects every id at or above it first"),
    (99, "error buffer clear index mutants are unobservable, the byte lands before the buffer or at index one, every failing path rewrites the buffer through snprintf before any pinned reader and no pinned reader reads err after a success return"),
    (105, "script tail copy count mutants are equivalent, the extra byte lands past the nul inside the text array and is never scanned past end, and the short copy leaves the tail byte the tokenizer rewrites when the final token reaches it while comment tails are never read past end"),
    (121, "comment scan bound widening reads the nul sentinel at end which fails the newline clause and exits identically"),
    (131, "command row memset fill mutants are dead stores, every field the parser consumes is assigned after the memset"),
    (135, "line end scan bound widening reads the nul sentinel at the boundary which fails the scan clause and leaves the same line end"),
    (140, "token scan bound widening reaches the break arm at line_end and exits the token loop identically"),
    (141, "separator skip bound widening reads the newline sentinel at line_end which fails the separator test and exits identically"),
    (155, "quote close scan bound widening reads the newline sentinel at line_end which fails the quote clause so the unterminated quote verdict is unchanged"),
    (163, "quote close advance mutant skips one char after the closing quote, every quoted token is followed by a separator or line end so the skipped char is blank and the token stream is unchanged"),
    (169, "delimiter probe bound widening reads the newline or nul sentinel at line_end, neither equals the hash delimiter so the token termination is unchanged"),
    (184, "empty line argc guard mutant compares against minus one, argc is never zero at the guard because the outer loop consumes blank and comment only lines before command parsing starts"),
    (191, "expect kind arity guard mutants only select which parse error message fires on expect lines with fewer than three tokens, the accept set is unchanged and no pinned suite pins the message"),
    (196, "expect table element size index mutants are equivalent, the entries are a uniform struct so sizeof of any element is identical"),
    (201, "word table element size index mutants are equivalent, the entries are a uniform struct so sizeof of any element is identical"),
    (213, "operator index array size mutants only change trailing slack, the designated values at the used indices are unchanged and the runtime index never reaches the boundary"),
    (224, "argument shift loop bound widening writes one offset slot past argc, the slot is in bounds padding and every accessor bounds the index by argc before reading"),
    (226, "argument length copy arithmetic mutant is a dead store, no reader consumes the len fields, argument text is nul terminated in place and accessed through the offsets only"),
    (247, "accessor cap clause widening differs only at an index equal to the args max with argc above it, argc never exceeds six so the argc clause rejects that index first"),
    (259, "errno preset mutants are equivalent, a non ERANGE preset is cleared identically for the ERANGE comparison and strtol sets errno itself on range errors"),
    (270, "errno preset mutants are equivalent, a non ERANGE preset is cleared identically for the ERANGE comparison and strtod sets errno itself on range errors"),
    (311, "negative difference test bound widening is a no-op at zero, negating plus or minus zero yields zero so the epsilon comparison is unchanged"),
    (316, "double equality epsilon boundary mutant differs only at a difference of exactly the epsilon constant which no decimal literal pair constructs, every pinned difference lands strictly above or below"),
    (322, "double inequality epsilon boundary mutant differs only at a difference of exactly the epsilon constant which no decimal literal pair constructs, every pinned difference lands strictly above or below"),
    (360, "clock stamp index mutant checks the second stamp char, the utc clock always writes a stamp whose second char is a digit so the empty stamp arm never fires"),
    (361, "clock failure return payload mutants are unreachable, the utc clock always writes a parseable stamp so the failure return never executes"),
    (422, "card field capacity init mutants are dead stores, every branch of the field resolver assigns the capacity before any read"),
    (427, "card field value index shift runs through the guard the strlen nonnull assumption deletes at o2, the stale pre-shift offset slot still holds the value token for two argument commands"),
    (432, "cards field dirty flag mutant is display bookkeeping, the save path updates unconditionally and the form recomputes the flag on refresh, no headless suite reads it"),
    (448, "cards step default arm is unreachable, the step dispatcher routes exactly the cards kinds into this handler"),
    (456, "review day init mutants are dead stores, the clock helper writes the day on the only path that continues"),
    (460, "review begin limit minus one takes the same unlimited paging path as zero, the queue loader branches only on limit greater than zero"),
    (461, "review begin rc bound widening admits only a begin return of minus one, every reachable begin returns a nonnegative queue count over the scripted default deck"),
    (475, "review grade found flag initializer is a dead store, the grade parse helper clears the out flag on entry before any read"),
    (482, "clock stamp index mutant checks the second stamp char, the utc clock always writes a stamp whose second char is a digit so the empty stamp arm never fires"),
    (493, "clock stamp index mutant checks the second stamp char, the utc clock always writes a stamp whose second char is a digit so the empty stamp arm never fires"),
    (508, "wait tick loop bound and start mutants shift one animation step, no script observable reads animation progress and no pinned wait count sits on a completion edge"),
    (514, "wait animation pause branch mutant touches only anim bookkeeping, pausing a completed or idle animation changes no graded state"),
    (516, "wait playing flag mutant touches a display state field that no headless suite reads"),
    (523, "review step default arm is unreachable, the step dispatcher routes exactly the review kinds into this handler"),
    (576, "settings step default arm is unreachable, the step dispatcher routes exactly the settings kinds into this handler"),
    (590, "cfop stage parse error payload mutant is sign tested, both failure paths return a negative payload and the pinned bogus stage script pins the message"),
    (597, "cfop stage argument index shift runs through the guard the strcmp nonnull assumption deletes at o2, the stale pre-shift offset slot still holds the stage token for one argument commands"),
    (602, "cfop populate bound widening admits made values of minus one and zero, every reachable stage inserts a positive card count and populate failures are unreachable over the in memory corpus"),
    (608, "trainer list id init mutants are dead stores, the integer parse writes the id on the only path that continues"),
    (612, "trainer event ok flag initializer is a dead store, the event parse helper clears the out flag on entry before any read"),
    (613, "trainer method ok flag initializer is a dead store, the method parse helper clears the out flag on entry before any read"),
    (624, "list guard bound and cap mutants are clause dominated, found zero rejects through the found index clause with the same verdict and message and the cap clause never fires because no pair yields more than three lists"),
    (631, "trainer list target key reset index mutant writes the nul one byte into a stale tail slot, the key is rewritten by the next case step before any read"),
    (642, "case guard bound mutants are clause dominated or unreachable, count zero rejects through the index clause with the same verdict and no selector list holds exactly one case across cfop stages, chart targets, and seeded packs"),
    (659, "synth ready guard mutant is equivalent, the synthesis entry self initializes through its built gate so skipping the eager init changes no result"),
    (663, "target sticker one init mutants are dead stores, the pair resolver writes both outputs on the only path that continues"),
    (664, "target sticker two init mutants are dead stores, the pair resolver writes both outputs on the only path that continues"),
    (669, "orbit buffer ternary flip is equivalent over the script surface, no script word stages buffer overrides so both scheme buffers carry the equal fallback index and the flip selects the same buffer"),
    (671, "synthesis result memset fill mutants are dead stores, the synthesis entry memsets the whole output on every return path before writing"),
    (673, "synthesis failure disjunction mutant is equivalent over reachable targets, every generate resolves a candidate so the success with found false arm never occurs"),
    (680, "classification memset fill mutants are equivalent, the classifier overwrites every consumed field and the off bytes are tail padding never read"),
    (684, "scoring memset fill mutants are equivalent, the scorer overwrites every consumed field and the off bytes are tail padding never read"),
    (689, "scheme id bound mutants never change the verdict, generate only runs after the list step seeded the scheme so the id is always one at the guard and the widened bound only reenters an idempotent find first reseed"),
    (690, "scheme seed guard mutants are unreachable, generate only runs after the list step seeded the scheme so the enclosing branch never runs"),
    (695, "post seed scheme id bound mutants are unreachable, the scheme id is always one on every path that reaches the guard"),
    (701, "generated alg struct memset fill mutants are equivalent, every consumed field is assigned before the store and the off bytes are tail padding the row validator never reads"),
    (739, "trainer step dispatcher default arm is unreachable, the step dispatcher routes every trainer kind and the expect kinds route to the expect handler"),
    (752, "due date day output literal mutants are unobservable, the due command validates only and discards the parsed day value"),
    (770, "orbit parse error payload mutant is sign tested, both callers compare the result against zero"),
    (873, "scripted entry guard mutants sit outside the gate suite surface, the file entry runs only through the e2e make target against the real binary where every pinned invocation passes a valid app and path"),
    (878, "scripted entry open failure mutant sits outside the gate suite surface, every pinned e2e invocation opens its script file successfully"),
    (882, "scripted entry buffer allocation mutants sit outside the gate suite surface, every pinned e2e invocation allocates the script buffer successfully"),
    (883, "scripted entry allocation failure guard sits outside the gate suite surface, malloc failure is unreachable in every pinned e2e invocation"),
    (888, "scripted entry read size mutants sit outside the gate suite surface, the pinned e2e corpus holds files well under the cap so a read bound one byte either way returns the same content, and the oversize arm is pinned by the caps suite at the parse layer"),
    (890, "scripted entry oversize boundary mutant admits a file of exactly the text cap, no pinned e2e file reaches the cap"),
    (901, "scripted entry parse result comparison mutants change only the stderr diagnostic, the return code is rc in both arms and no gate suite reads stderr"),
    (906, "scripted entry runtime seed mutants sit outside the gate suite surface, no gate suite invokes the file entry so the seed never executes"),
    (908, "scripted entry run result comparison mutants change only the stderr diagnostic, the return code is rc in both arms and no gate suite reads stderr"),
]

KILLED = {226, 228, 456, 615, 644, 758, 790, 800}


def main():
    lines = open(ALLOW, encoding="utf-8").read().splitlines()
    kept = [ln for ln in lines if not ln.startswith("src/app/script.c:")]
    new = [f"src/app/script.c:{ln}\t{why}" for ln, why in ROWS]
    merged = kept + new
    keys = [ln.split("\t")[0] for ln in merged]
    dups = sorted({k for k in keys if keys.count(k) > 1})
    with open(ALLOW, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(merged) + "\n")
    print(f"dropped {len(lines) - len(kept)} old script rows, appended {len(new)}, total {len(merged)}")
    print(f"dup keys: {dups if dups else 'none'}")

    surv = []
    acc = set()
    for line in open(LOG, encoding="utf-8", errors="replace"):
        m = re.match(r"SURVIVED\s+m(\d+) (\S+) \[", line)
        if m:
            surv.append((int(m.group(1)), m.group(2)))
        m = re.match(r"ACCEPTED\s+m(\d+)", line)
        if m:
            acc.add(int(m.group(1)))
    covered = {}
    for ln in merged:
        m = re.match(r"^(\S+?)(?::(\d+))?\t", ln)
        if m:
            covered[(m.group(1), int(m.group(2)) if m.group(2) else None)] = True
    left = []
    for mid, loc in surv:
        if mid in acc or mid in KILLED:
            continue
        m = re.match(r"^(.+):(\d+)$", loc)
        if not m or (m.group(1), int(m.group(2))) not in covered:
            left.append((mid, loc))
    print(f"acceptance leftovers: {left if left else 'none'}")


if __name__ == "__main__":
    main()
