#!/bin/sh
i=1
while [ "$i" -le 20 ]; do
	gdb -batch -quiet \
		-ex "handle SIGTRAP nostop noprint pass" \
		-ex run -ex "echo \n===CRASH===\n" -ex "bt 25" -ex "info registers rip" \
		--args ./build/bin/ct_tests.exe e2e_run_shot_render \
		>"playground/renderflake/gdb_run$i.txt" 2>&1
	if grep -q "===CRASH===" "playground/renderflake/gdb_run$i.txt"; then
		if grep -q "Inferior .* exited normally\|exited with code 0" "playground/renderflake/gdb_run$i.txt"; then
			echo "gdb run $i clean"
		else
			echo "gdb run $i CRASH captured"
			break
		fi
	else
		echo "gdb run $i clean"
	fi
	i=$((i + 1))
done
