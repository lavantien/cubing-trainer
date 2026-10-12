#!/bin/sh
i=1
while [ "$i" -le 30 ]; do
	./build/bin/ct_tests.exe e2e_run_shot_render >/dev/null 2>"playground/renderflake/mark_run$i.err"
	rc=$?
	echo "run $i rc=$rc" >>playground/renderflake/markloop_summary.txt
	if [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ]; then
		cp "playground/renderflake/mark_run$i.err" "playground/renderflake/mark_crash_$i.err"
		echo "CRASH at run $i, last marker:" >>playground/renderflake/markloop_summary.txt
		tail -3 "playground/renderflake/mark_run$i.err" >>playground/renderflake/markloop_summary.txt
		break
	fi
	i=$((i + 1))
done
echo "loop end" >>playground/renderflake/markloop_summary.txt
