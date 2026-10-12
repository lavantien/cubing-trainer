#!/bin/sh
suite="$1"
runs="${2:-20}"
exe=./build/bin/ct_tests.exe
outdir="playground/renderflake/$suite"
mkdir -p "$outdir"
crashes=0
i=1
while [ "$i" -le "$runs" ]; do
	"$exe" "$suite" >"$outdir/run$i.out" 2>"$outdir/run$i.err"
	rc=$?
	echo "run $i rc=$rc" >>"$outdir/summary.txt"
	if [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ]; then
		crashes=$((crashes + 1))
		cp "$outdir/run$i.err" "$outdir/crash_$i.err"
	fi
	i=$((i + 1))
done
echo "suite=$suite runs=$runs crashes=$crashes" >>"$outdir/summary.txt"
