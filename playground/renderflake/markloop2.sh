#!/bin/sh
i=1
while [ "$i" -le 60 ]; do
	./build/bin/ct_tests.exe e2e_run_shot_render >/dev/null 2>"playground/renderflake/mk2_run.err"
	rc=$?
	if [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ]; then
		echo "crash $i rc=$rc:" >>playground/renderflake/markloop2_summary.txt
		tail -2 "playground/renderflake/mk2_run.err" >>playground/renderflake/markloop2_summary.txt
		cp "playground/renderflake/mk2_run.err" "playground/renderflake/mk2_crash_$i.err"
	fi
	i=$((i + 1))
done
echo done >>playground/renderflake/markloop2_summary.txt
