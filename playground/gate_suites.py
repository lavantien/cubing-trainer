import json
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[1]

SCOPES = [
    "src/ui/screen_trainer.c",
    "src/ui/screen_settings.c",
    "src/app/script.c",
    "src/ui/screen_review.c",
    "src/ui/screen_stats.c",
    "src/ui/screen_cards.c",
    "src/ui/screen_menu.c",
    "src/ui/widgets.c",
    "src/app/shots.c",
    "src/app/paths.c",
    "src/app/app_loop.c",
    "src/app/script_expect.c",
]

KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "else", "do"}

DEF_WHOLE = re.compile(
    r"^(?:static\s+)?[A-Za-z_][A-Za-z0-9_ \t\*]*?\b([a-z_][a-z0-9_]*)\s*\("
    r"([^;{}()]*(?:\([^;{}()]*\)[^;{}()]*)*)\)\s*\{",
    re.M,
)
SUITE_RE = re.compile(r"CT_SUITE\(([a-z0-9_]+)\)")

GL_CHURN = {"e2e_run_shot_render", "e2e_run_shot_guards", "shots_render_smoke"}

E2E_SCRIPT_FILES = {"unit_e2e_script.c", "unit_e2e_script_playback.c"}

E2E_KEYWORDS = {
    "src/ui/screen_trainer.c": r"trainer|memo",
    "src/ui/screen_review.c": r"review",
    "src/ui/screen_cards.c": r"card|cfop",
    "src/ui/screen_settings.c": r"settings",
    "src/app/shots.c": r"shot",
    "src/app/paths.c": r"restart|data_dir",
    "src/app/app_loop.c": r"restart|data_dir",
}

OVERLAYS = {
    "src/app/script.c": "all_e2e",
    "src/app/script_expect.c": "e2e_run",
    "src/ui/screen_menu.c": ["stats_screen_view_load"],
}


def defined_functions(path):
    names = set()
    for m in DEF_WHOLE.finditer(path.read_text(encoding="utf-8")):
        if m.group(1) not in KEYWORDS:
            names.add(m.group(1))
    return names


def suite_blocks(path):
    text = path.read_text(encoding="utf-8")
    marks = [(m.start(), m.group(1)) for m in SUITE_RE.finditer(text)]
    blocks = []
    for i, (pos, name) in enumerate(marks):
        end = marks[i + 1][0] if i + 1 < len(marks) else len(text)
        blocks.append((name, text[pos:end]))
    return blocks


def main():
    suites = {}
    seen_in = {}
    for tf in sorted((root / "tests").glob("unit_*.c")):
        for name, block in suite_blocks(tf):
            if name in suites and seen_in[name] != tf.name:
                raise SystemExit(f"duplicate suite name {name} in {tf.name} and {seen_in[name]}")
            seen_in[name] = tf.name
            suites[name] = (tf.name, block)
    funcs = {scope: defined_functions(root / scope) for scope in SCOPES}
    plan = {}
    for scope in SCOPES:
        refs = set()
        for name, (tf, block) in suites.items():
            if any(re.search(rf"\b{re.escape(f)}\b", block) for f in funcs[scope]):
                refs.add(name)
            elif tf in E2E_SCRIPT_FILES and scope in E2E_KEYWORDS:
                if re.search(E2E_KEYWORDS[scope], block):
                    refs.add(name)
        overlay = OVERLAYS.get(scope)
        if overlay == "all_e2e":
            refs |= {n for n, (tf, _) in suites.items() if tf in E2E_SCRIPT_FILES}
        elif overlay == "e2e_run":
            refs |= {
                n for n, (tf, _) in suites.items() if tf in E2E_SCRIPT_FILES and n.startswith("e2e_run")
            }
        elif isinstance(overlay, list):
            refs |= set(overlay)
        if scope != "src/app/shots.c":
            refs -= GL_CHURN
        plan[scope] = sorted(refs)
    (root / "playground" / "gate_plan.json").write_text(json.dumps(plan, indent=1), encoding="utf-8")
    for scope in SCOPES:
        print(f"{scope} [{len(plan[scope])}]")


if __name__ == "__main__":
    main()
