#!/bin/sh
export CT_MUTATE_CC="clang"
export CT_MUTATE_CFLAGS="--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite"
export CT_MUTATE_LDFLAGS="--target=x86_64-w64-mingw32 -fuse-ld=lld -g"
export CT_MUTATE_LDLIBS="-static-libgcc -lopengl32 -lgdi32 -lwinmm"
probe() {
	mid="$1"
	suite="$2"
	i=1
	while [ "$i" -le 5 ]; do
		CT_MUTATE_ONLY="$mid" build_scrn2/bin/mutate.exe src/ui/screen_trainer.c build_scrn2 "$suite" >"playground/renderflake/p2_${mid}_$i.log" 2>"playground/renderflake/p2_${mid}_$i.err"
		grep -h "suite .* exited" "playground/renderflake/p2_${mid}_$i.err" | sed "s/^/m$mid run$i: /" >>playground/renderflake/probe5_lane2_summary.txt
		echo "m$mid run$i killed=$(grep -c '^KILLED' playground/renderflake/p2_${mid}_$i.log)" >>playground/renderflake/probe5_lane2_summary.txt
		i=$((i + 1))
	done
}
probe 9 shots_render_smoke
probe 11 shots_render_smoke
probe 95 trainer_form_open
echo done >>playground/renderflake/probe5_lane2_summary.txt
