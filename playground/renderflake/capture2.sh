#!/bin/sh
export CT_MUTATE_CC="clang"
export CT_MUTATE_CFLAGS="--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite"
export CT_MUTATE_LDFLAGS="--target=x86_64-w64-mingw32 -fuse-ld=lld -g"
export CT_MUTATE_LDLIBS="-static-libgcc -lopengl32 -lgdi32 -lwinmm"
export CT_MUTATE_ONLY="7,26,71,82,87,91,93,108,122,131,132,145,158,165,168,173,174"
(
	captured=0
	for tries in $(seq 1 900); do
		m108=$(ls build_scrn2/mutants/p*/m0108_script_shots.o 2>/dev/null | grep -v p25680 | head -1)
		if [ -n "$m108" ]; then
			exe=$(ls -t build_scrn2/bin/ct_mutant_p*.exe 2>/dev/null | head -1)
			if [ -n "$exe" ]; then
				om=$(stat -c %Y "$m108" 2>/dev/null)
				em=$(stat -c %Y "$exe" 2>/dev/null)
				if [ -n "$om" ] && [ -n "$em" ] && [ "$em" -ge "$om" ]; then
					cp "$exe" playground/renderflake/cap2_m0108.exe
					echo "captured $exe o=$om e=$em" >>playground/renderflake/capture2.txt
					captured=1
					break
				fi
			fi
		fi
		sleep 0.1
	done
	echo "captured=$captured" >>playground/renderflake/capture2.txt
) &
build_scrn2/bin/mutate.exe src/app/script_shots.c build_scrn2 e2e_parse_shot_words e2e_run_shot_guards e2e_run_shot_render e2e_run_reviewed_seed e2e_run_due_in e2e_run_script_guards e2e_run_trainer_generate shots_arg_guards shots_state_guards shots_render_smoke >playground/renderflake/cap2_lane.log 2>playground/renderflake/cap2_lane.err
wait
echo lane_done >>playground/renderflake/capture2.txt
