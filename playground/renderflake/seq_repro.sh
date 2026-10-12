#!/bin/sh
i=1
while [ "$i" -le 10 ]; do
	./build/bin/ct_tests.exe e2e_run_shot_render >/dev/null 2>/dev/null
	r1=$?
	./build/bin/ct_tests.exe e2e_run_shot_guards >/dev/null 2>/dev/null
	r2=$?
	echo "iter $i render=$r1 guards=$r2" >>playground/renderflake/seq_repro_summary.txt
	i=$((i + 1))
done
echo done >>playground/renderflake/seq_repro_summary.txt
