import pathlib
import subprocess
import sys

root = pathlib.Path(r"C:\Users\lavantien\dev\github\cubing-trainer")


def run(args, **kw):
    return subprocess.run(args, cwd=root, capture_output=True, text=True, check=True, **kw).stdout


def head(path):
    return run(["git", "show", "HEAD:" + path])


def work(path):
    return (root / path).read_text(encoding="utf-8")


def stage(path, content):
    tmp = root / ".git" / ("ct_stage_" + pathlib.Path(path).name)
    tmp.write_text(content, encoding="utf-8", newline="\n")
    h = run(["git", "hash-object", "-w", str(tmp)]).strip()
    run(["git", "update-index", "--add", "--cacheinfo", f"100644,{h},{path}"])
    tmp.unlink()


ui_head = head("src/ui/ui.h")
ui_work = work("src/ui/ui.h")
marker = '#include "engine/cube.h"'
start = ui_work.index(marker)
end = ui_work.rindex("#endif")
staged_ui = ui_head[: ui_head.rindex("#endif")] + ui_work[start:end] + ui_head[ui_head.rindex("#endif") :]

menu_head = head("src/ui/screen_menu.c")
update_anchor = """    if (app->screen == CT_SCREEN_CARDS) {
        ct_screen_cards_update(app);
        return;
    }
"""
update_review = update_anchor + """    if (app->screen == CT_SCREEN_REVIEW) {
        ct_screen_review_update(app);
        return;
    }
"""
draw_anchor = """    if (app->screen == CT_SCREEN_CARDS) {
        ct_screen_cards_draw(app);
        return;
    }
"""
draw_review = draw_anchor + """    if (app->screen == CT_SCREEN_REVIEW) {
        ct_screen_review_draw(app);
        return;
    }
"""
if menu_head.count(update_anchor) != 1 or menu_head.count(draw_anchor) != 1:
    sys.exit("menu anchors not unique in HEAD")
staged_menu = menu_head.replace(update_anchor, update_review).replace(draw_anchor, draw_review)

stage("src/ui/ui.h", staged_ui)
stage("src/ui/screen_menu.c", staged_menu)
run(["git", "add", "src/ui/screen_review.c", "tests/unit_review_form.c"])
print(run(["git", "diff", "--cached", "--stat"]))
