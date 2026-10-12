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
		CT_MUTATE_ONLY="$mid" build_scrn2/bin/mutate.exe src/app/script_shots.c build_scrn2 "$suite" >"playground/renderflake/p3_${mid}_$i.log" 2>"playground/renderflake/p3_${mid}_$i.err"
		ev=$(grep -h "suite .* exited" "playground/renderflake/p3_${mid}_$i.err" | head -1)
		echo "m$mid run$i killed=$(grep -c '^KILLED' playground/renderflake/p3_${mid}_$i.log) $ev" >>playground/renderflake/probe5_lane3_summary.txt
		i=$((i + 1))
	done
}
probe 108 e2e_run_shot_guards
probe 132 e2e_run_shot_guards
probe 145 e2e_run_shot_guards
probe 173 e2e_run_shot_guards
echo done >>playground/renderflake/probe5_lane3_summary.txt
