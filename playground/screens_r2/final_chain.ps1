Set-StrictMode -Version Latest
$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"

function Run-Step([string]$Name, [string]$Cmd) {
    & pwsh.exe -NoProfile -Command $Cmd 1>"playground\screens_r2\final_$Name.log" 2>"playground\screens_r2\final_$Name.err"
    $code = $LASTEXITCODE
    Write-Output ("{0}: exit {1}" -f $Name, $code)
    if ($code -ne 0) { exit $code }
}

Run-Step "lint" "make lint"
Run-Step "analyze" "make analyze BUILDDIR=build_scrn2"
Run-Step "test" "make test BUILDDIR=build_scrn2"
Run-Step "e2e" "make e2e BUILDDIR=build_scrn2"
Run-Step "visual" "make visual BUILDDIR=build_scrn2"
Run-Step "shots1" "make screenshots BUILDDIR=build_scrn2"
Run-Step "shots2" "make screenshots BUILDDIR=build_scrn2"
Write-Output "CHAIN_GREEN"
