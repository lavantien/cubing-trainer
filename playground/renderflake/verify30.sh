#!/bin/sh
crashes=0
i=1
while [ "$i" -le 30 ]; do
	./build/bin/ct_tests.exe e2e_run_shot_render >/dev/null 2>/dev/null
	rc=$?
	if [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ]; then
		crashes=$((crashes + 1))
		echo "crash run $i rc=$rc" >>playground/renderflake/verify30_summary.txt
	fi
	i=$((i + 1))
done
echo "suite=e2e_run_shot_render runs=30 crashes=$crashes" >>playground/renderflake/verify30_summary.txt
echo done >>playground/renderflake/verify30_summary.txt
