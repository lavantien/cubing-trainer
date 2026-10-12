#!/bin/sh
n=1
found=0
while [ "$n" -le 40 ]; do
	pad=$(printf 'X%.0s' $(seq 1 $n))
	CT_MUTATE_ONLY="7,26,71,82,87,91,93,108,122,131,132,145,158,165,168,173,174$pad" ./build/bin/ct_tests.exe e2e_run_shot_guards >/dev/null 2>/dev/null
	rc=$?
	echo "pad $n rc=$rc" >>playground/renderflake/envpad_summary.txt
	if [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ]; then
		found=1
		echo "CRASH at pad $n" >>playground/renderflake/envpad_summary.txt
		break
	fi
	n=$((n + 1))
done
echo "found=$found" >>playground/renderflake/envpad_summary.txt
echo done >>playground/renderflake/envpad_summary.txt
