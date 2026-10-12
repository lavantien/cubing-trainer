import pathlib
import random
import sys

SEED = 20261012
POPULATION = 64
GENERATIONS = 240
TOURNAMENT = 3
ELITE = 2
LETTERS = [chr(ord("A") + i) for i in range(24)]
MIGRATION_REL = pathlib.Path("db") / "migrations" / "002_memo_pairs.sql"
REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent

SYLLABLES = {
    "A": [("an", "an1"), ("áo", "aw5"), ("ay", "aj1"), ("âu", "a^w1")],
    "B": [("bông", "bo^ng1"), ("bàn", "ban2"), ("bếp", "bep5"), ("buồn", "buo~n2")],
    "C": [("cá", "ka5"), ("câm", "kam1"), ("cốc", "ko^k5"), ("cuốn", "kuo~n5")],
    "D": [("dù", "zu2"), ("dơi", "zo~j1"), ("dâu", "za^w1"), ("dì", "zi2")],
    "E": [("em", "em1"), ("ếch", "e^k5"), ("eo", "ew1"), ("êm", "e^m1")],
    "F": [("phao", "faw1"), ("phím", "fim5"), ("phố", "fo^5"), ("phun", "fun1")],
    "G": [("gà", "ga2"), ("gối", "go^j5"), ("gừng", "gu~ng2"), ("gan", "gan1")],
    "H": [("hổ", "ho3"), ("hương", "huo~ng1"), ("hát", "hat5"), ("hèo", "hew2")],
    "I": [("ít", "it5"), ("inh", "ing1"), ("í", "i5"), ("im", "im1")],
    "J": [("gia", "za1"), ("giò", "zo2"), ("giấy", "zaj3"), ("giun", "zun1")],
    "K": [("kem", "kem1"), ("kèo", "kew2"), ("kính", "king2"), ("kốt", "ko^t5")],
    "L": [("lúa", "lua5"), ("lòng", "long2"), ("lá", "la5"), ("lửa", "luo~3")],
    "M": [("mèo", "mew1"), ("mưa", "muo~1"), ("máy", "maj5"), ("mũi", "mu~j3")],
    "N": [("nón", "non5"), ("nâu", "na^w1"), ("nải", "naj3"), ("nát", "nat5")],
    "O": [("ót", "ot5"), ("ong", "ong1"), ("ổi", "oj3"), ("ôm", "om1")],
    "P": [("pin", "pin1"), ("pao", "paw1"), ("pố", "po^5"), ("pen", "pen1")],
    "Q": [("quả", "kwa3"), ("quyên", "kwien1"), ("quốc", "kwo^k5"), ("quýt", "kwit5")],
    "R": [("rùa", "ruo~1"), ("rồng", "zong2"), ("rét", "ze^t5"), ("rơm", "zo~m1")],
    "S": [("sơ", "so~1"), ("sách", "sajk5"), ("sáo", "saw5"), ("sếu", "se^w3")],
    "T": [("táo", "taw5"), ("túi", "tuj5"), ("tết", "tet5"), ("tịch", "tik5")],
    "U": [("ù", "u2"), ("uống", "uo~ng5"), ("uất", "ua^t5"), ("uân", "ua^n1")],
    "V": [("với", "vo~j3"), ("vàng", "vang2"), ("voi", "voj1"), ("vẽ", "ve3")],
    "W": [("oan", "wan1"), ("oán", "wan5"), ("oanh", "wajng1"), ("oách", "wajk5")],
    "X": [("xinh", "sing1"), ("xôi", "so^j1"), ("xịn", "sin5"), ("xấu", "sa^w5")],
    "Y": [("yến", "ien5"), ("yêu", "ie^w1"), ("yên", "ien1"), ("yết", "iet5")],
}

IMAGES = {
    "A": ["anchor", "apple", "arrow", "atlas"],
    "B": ["balloon", "bicycle", "bridge", "butterfly"],
    "C": ["candle", "castle", "compass", "crown"],
    "D": ["dolphin", "dragon", "drum", "diamond"],
    "E": ["eagle", "engine", "envelope", "elephant"],
    "F": ["feather", "forest", "fountain", "fox"],
    "G": ["guitar", "globe", "grapes", "gate"],
    "H": ["hammer", "harbor", "helmet", "honey"],
    "I": ["igloo", "island", "ink", "iron"],
    "J": ["jacket", "jar", "jellyfish", "jungle"],
    "K": ["kettle", "key", "kite", "kiwi"],
    "L": ["ladder", "lantern", "lemon", "lion"],
    "M": ["mountain", "mirror", "moon", "magnet"],
    "N": ["needle", "nest", "notebook", "nose"],
    "O": ["ocean", "olive", "onion", "owl"],
    "P": ["paddle", "pineapple", "pillow", "pyramid"],
    "Q": ["queen", "quilt", "quiver", "quartz"],
    "R": ["rainbow", "ring", "rocket", "river"],
    "S": ["saddle", "ship", "spider", "sun"],
    "T": ["telescope", "tiger", "torch", "trumpet"],
    "U": ["umbrella", "unicorn", "uniform", "urn"],
    "V": ["valley", "violin", "volcano", "vase"],
    "W": ["wagon", "walnut", "waterfall", "wolf"],
    "X": ["xylophone", "xenon", "xylem", "xray"],
    "Y": ["yacht", "yak", "yoyo", "yogurt"],
    "Z": ["zebra", "zipper", "zeppelin", "zucchini"],
}


def validate_pools():
    writings = []
    phonemes = []
    words = []
    for letter in LETTERS:
        entries = SYLLABLES[letter]
        if len(entries) < 4:
            raise SystemExit(f"pool too small for {letter}")
        for writing, phoneme in entries:
            writings.append(writing)
            phonemes.append(phoneme)
        if len(IMAGES[letter]) < 4:
            raise SystemExit(f"image pool too small for {letter}")
        for word in IMAGES[letter]:
            if not word.startswith(letter.lower()):
                raise SystemExit(f"image word {word} does not start with {letter}")
            words.append(word)
    if len(set(writings)) != len(writings):
        raise SystemExit("duplicate syllable writing in pool")
    if len(set(phonemes)) != len(phonemes):
        raise SystemExit("duplicate phoneme in pool")
    if len(set(words)) != len(words):
        raise SystemExit("duplicate image word in pool")


def levenshtein(a, b):
    if len(a) < len(b):
        a, b = b, a
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i]
        for j, cb in enumerate(b, 1):
            cur.append(min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (ca != cb)))
        prev = cur
    return prev[-1]


def distance(a, b):
    return levenshtein(a, b) / max(len(a), len(b))


def candidate_ids():
    ids = []
    for letter in LETTERS:
        ids.extend((letter, i) for i in range(len(SYLLABLES[letter])))
    return ids


def distance_matrix(ids):
    phones = [SYLLABLES[letter][i][1] for letter, i in ids]
    matrix = [[0.0] * len(phones) for _ in phones]
    for a in range(len(phones)):
        for b in range(a + 1, len(phones)):
            d = distance(phones[a], phones[b])
            matrix[a][b] = d
            matrix[b][a] = d
    return matrix


def offsets():
    marks = []
    total = 0
    for letter in LETTERS:
        marks.append(total)
        total += len(SYLLABLES[letter])
    return marks, total


class search:
    def __init__(self, rng):
        self.rng = rng
        self.ids = candidate_ids()
        self.matrix = distance_matrix(self.ids)
        self.marks, self.total = offsets()
        self.sizes = [len(SYLLABLES[letter]) for letter in LETTERS]
        self.cache = {}

    def local_ids(self, individual):
        return [self.marks[i] + individual[i] for i in range(len(LETTERS))]

    def fitness(self, individual):
        key = tuple(individual)
        cached = self.cache.get(key)
        if cached is not None:
            return cached
        chosen = self.local_ids(individual)
        worst = None
        total = 0.0
        for a in range(len(chosen)):
            row = self.matrix[chosen[a]]
            for b in range(a + 1, len(chosen)):
                d = row[chosen[b]]
                total += d
                if worst is None or d < worst:
                    worst = d
        value = (worst, total)
        self.cache[key] = value
        return value

    def greedy(self):
        individual = [0] * len(LETTERS)
        picked = []
        for i in range(len(LETTERS)):
            best_choice = 0
            best_score = None
            for choice in range(self.sizes[i]):
                local = self.marks[i] + choice
                worst = None
                for other in picked:
                    d = self.matrix[local][other]
                    if worst is None or d < worst:
                        worst = d
                score = 1.0 if worst is None else worst
                if best_score is None or score > best_score:
                    best_score = score
                    best_choice = choice
            individual[i] = best_choice
            picked.append(self.marks[i] + best_choice)
        return individual

    def random_individual(self):
        return [self.rng.randrange(size) for size in self.sizes]

    def tournament(self, population):
        best = None
        for _ in range(TOURNAMENT):
            candidate = population[self.rng.randrange(len(population))]
            if best is None or self.fitness(candidate) > self.fitness(best):
                best = candidate
        return best

    def crossover(self, a, b):
        return [a[i] if self.rng.randrange(2) == 0 else b[i] for i in range(len(LETTERS))]

    def mutate(self, individual):
        child = list(individual)
        for _ in range(1 + (self.rng.randrange(4) == 0)):
            slot = self.rng.randrange(len(LETTERS))
            child[slot] = self.rng.randrange(self.sizes[slot])
        return child

    def run(self):
        population = [self.greedy()]
        while len(population) < POPULATION:
            population.append(self.random_individual())
        ranked = sorted(population, key=self.fitness, reverse=True)
        for _ in range(GENERATIONS):
            children = list(ranked[:ELITE])
            while len(children) < POPULATION:
                left = self.tournament(ranked)
                right = self.tournament(ranked)
                children.append(self.mutate(self.crossover(left, right)))
            ranked = sorted(children, key=self.fitness, reverse=True)
        return ranked[0]


def atom_table(individual):
    rows = {}
    for i, letter in enumerate(LETTERS):
        writing, phoneme = SYLLABLES[letter][individual[i]]
        rows[letter] = (writing, phoneme, IMAGES[letter][individual[i]])
    return rows


def assert_distinct_atoms(atoms):
    phones = [atoms[letter][1] for letter in LETTERS]
    for a in range(len(phones)):
        for b in range(a + 1, len(phones)):
            if levenshtein(phones[a], phones[b]) < 2:
                raise SystemExit(f"atoms {phones[a]} and {phones[b]} are within one edit")


def sql_text(atoms):
    lines = [
        "CREATE TABLE memo_pair(",
        "    orbit INTEGER NOT NULL,",
        "    pair TEXT NOT NULL,",
        "    phoneme TEXT NOT NULL,",
        "    image TEXT NOT NULL,",
        "    updated_at TEXT NOT NULL DEFAULT (datetime('now')),",
        "    PRIMARY KEY(orbit, pair)",
        ");",
        "",
    ]
    for orbit in (0, 1):
        for first in LETTERS:
            for second in LETTERS:
                pair = first + second
                phoneme = atoms[first][1] + " " + atoms[second][1]
                image = atoms[first][2] + " " + atoms[second][2]
                lines.append(
                    f"INSERT INTO memo_pair(orbit, pair, phoneme, image) "
                    f"VALUES({orbit}, '{pair}', '{phoneme}', '{image}');"
                )
        lines.append("")
    return "\n".join(lines[:-1]) + "\n"


def report(atoms):
    phones = [atoms[letter][1] for letter in LETTERS]
    worst = None
    total = 0.0
    for a in range(len(phones)):
        for b in range(a + 1, len(phones)):
            d = distance(phones[a], phones[b])
            total += d
            if worst is None or d < worst:
                worst = d
    print(f"min pairwise phoneme distance: {worst:.6f}")
    print(f"sum pairwise phoneme distance: {total:.6f}")
    header = "    " + "".join(f"{letter:>6}" for letter in LETTERS)
    print(header)
    for i, letter in enumerate(LETTERS):
        cells = []
        for j in range(len(LETTERS)):
            cell = "     -" if i == j else f"{distance(phones[i], phones[j]):6.2f}"
            cells.append(cell)
        print(f"{letter:>3} " + "".join(cells))
    for letter in LETTERS:
        writing, phoneme, image = atoms[letter]
        print(f"atom {letter} = {writing} {phoneme} {image}")


def diff_summary(want, got):
    want_lines = want.splitlines()
    got_lines = got.splitlines()
    differing = [
        (i, want_lines[i], got_lines[i])
        for i in range(min(len(want_lines), len(got_lines)))
        if want_lines[i] != got_lines[i]
    ]
    print(f"memo-seed diff: want {len(want_lines)} lines, committed {len(got_lines)} lines")
    print(f"memo-seed diff: {len(differing)} differing lines")
    for i, expected, committed in differing[:5]:
        print(f"memo-seed diff line {i + 1}: want [{expected}] committed [{committed}]")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    validate_pools()
    rng = random.Random(SEED)
    winner = search(rng).run()
    atoms = atom_table(winner)
    assert_distinct_atoms(atoms)
    text = sql_text(atoms)
    report(atoms)
    if "--check" in sys.argv[1:]:
        target = REPO_ROOT / MIGRATION_REL
        if not target.is_file():
            print(f"memo-seed FAIL: {MIGRATION_REL} is missing")
            return 1
        committed = target.read_text(encoding="ascii", newline="")
        if committed != text:
            diff_summary(text, committed)
            print(f"memo-seed FAIL: {MIGRATION_REL} is stale, rerun without --check")
            return 1
        print(f"memo-seed ok: {MIGRATION_REL} regenerates byte-identical")
        return 0
    target = REPO_ROOT / MIGRATION_REL
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="ascii", newline="\n")
    print(f"wrote {MIGRATION_REL}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
