#!/bin/sh
export CT_MUTATE_CC="clang"
export CT_MUTATE_CFLAGS="--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite"
export CT_MUTATE_LDFLAGS="--target=x86_64-w64-mingw32 -fuse-ld=lld -g"
export CT_MUTATE_LDLIBS="-static-libgcc -lopengl32 -lgdi32 -lwinmm"
export CT_MUTATE_ONLY="7,26,71,82,87,91,93,108,122,131,132,145,158,165,168,173,174"
(
	for tries in $(seq 1 1200); do
		m108=$(ls build_scrn2/mutants/p*/m0108_script_shots.o 2>/dev/null | grep -v p25680 | head -1)
		[ -z "$m108" ] && {
			sleep 0.1
			continue
		}
		exe=$(ls -t build_scrn2/bin/ct_mutant_p*.exe 2>/dev/null | head -1)
		[ -z "$exe" ] && {
			sleep 0.05
			continue
		}
		e0=$(stat -c %Y.%y "$exe" 2>/dev/null | cut -d. -f1-2)
		e1=""
		for w in $(seq 1 80); do
			e1=$(stat -c %Y.%y "$exe" 2>/dev/null | cut -d. -f1-2)
			if [ -n "$e1" ] && [ "$e1" != "$e0" ]; then break; fi
			sleep 0.1
		done
		if [ -n "$e1" ] && [ "$e1" != "$e0" ]; then
			sleep 0.5
			cp "$exe" playground/renderflake/cap3_m0108.exe && echo "captured at $e1" >>playground/renderflake/capture3.txt
		else
			echo "window missed" >>playground/renderflake/capture3.txt
		fi
		break
	done
) &
build_scrn2/bin/mutate.exe src/app/script_shots.c build_scrn2 e2e_parse_shot_words e2e_run_shot_guards e2e_run_shot_render e2e_run_reviewed_seed e2e_run_due_in e2e_run_script_guards e2e_run_trainer_generate shots_arg_guards shots_state_guards shots_render_smoke >playground/renderflake/cap3_lane.log 2>playground/renderflake/cap3_lane.err
wait
echo lane_done >>playground/renderflake/capture3.txt
