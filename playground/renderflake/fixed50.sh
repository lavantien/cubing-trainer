#!/bin/sh
crashes=0
i=1
while [ "$i" -le 50 ]; do
	./build_scrn2/bin/m0108_fixed.exe e2e_run_shot_guards >/dev/null 2>/dev/null
	rc=$?
	if [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ]; then
		crashes=$((crashes + 1))
		echo "crash run $i rc=$rc" >>playground/renderflake/fixed50_summary.txt
	fi
	i=$((i + 1))
done
echo "exe=m0108_fixed suite=e2e_run_shot_guards runs=50 crashes=$crashes" >>playground/renderflake/fixed50_summary.txt
echo done >>playground/renderflake/fixed50_summary.txt
