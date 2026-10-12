#!/bin/sh
export CT_MUTATE_CC="clang"
export CT_MUTATE_CFLAGS="--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite"
export CT_MUTATE_LDFLAGS="--target=x86_64-w64-mingw32 -fuse-ld=lld -g"
export CT_MUTATE_LDLIBS="-static-libgcc -lopengl32 -lgdi32 -lwinmm"
export CT_MUTATE_KEEP="1"
export CT_MUTATE_ONLY="7,26,71,82,87,91,93,108,122,131,132,145,158,165,168,173,174"
(
	for tries in $(seq 1 600); do
		exe=$(ls build_scrn2/bin/ct_mutant_p*.exe 2>/dev/null | head -1)
		m108=$(ls build_scrn2/mutants/p*/m0108_script_shots.o 2>/dev/null | grep -v p25680 | head -1)
		if [ -n "$exe" ] && [ -n "$m108" ]; then
			cp "$exe" playground/renderflake/captured_m0108.exe 2>/dev/null && echo "captured from $exe at $(date)" >>playground/renderflake/capture.txt && break
		fi
		sleep 0.2
	done
) &
build_scrn2/bin/mutate.exe src/app/script_shots.c build_scrn2 e2e_parse_shot_words e2e_run_shot_guards e2e_run_shot_render e2e_run_reviewed_seed e2e_run_due_in e2e_run_script_guards e2e_run_trainer_generate shots_arg_guards shots_state_guards shots_render_smoke >playground/renderflake/capture_lane.log 2>playground/renderflake/capture_lane.err
wait
echo lane_done >>playground/renderflake/capture.txt
