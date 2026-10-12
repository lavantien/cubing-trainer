#!/bin/sh
for suite in e2e_run_shot_render shots_render_smoke shots_arg_guards; do
	crashes=0
	i=1
	while [ "$i" -le 20 ]; do
		./build/bin/ct_tests.exe "$suite" >/dev/null 2>/dev/null
		rc=$?
		if [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ]; then
			crashes=$((crashes + 1))
		fi
		i=$((i + 1))
	done
	echo "suite=$suite runs=20 crashes=$crashes" >>playground/renderflake/verify20_summary.txt
done
echo done >>playground/renderflake/verify20_summary.txt
