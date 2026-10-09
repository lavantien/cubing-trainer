param(
    [string]$Root = (Split-Path $PSScriptRoot -Parent)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Invoke-Captured {
    param([string]$Exe, [string[]]$ArgumentList)

    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $text = (& $Exe @ArgumentList 2>&1 | Out-String)
        return @{ text = $text; code = $LASTEXITCODE }
    } finally {
        $ErrorActionPreference = $prev
    }
}

$clang = Get-Command clang -ErrorAction SilentlyContinue
if ($null -eq $clang) {
    Write-Output "analyze failed: clang not found on PATH"
    exit 1
}

$srcDir = Join-Path $Root "src"
if (-not (Test-Path -LiteralPath $srcDir)) {
    Write-Output "analyze passed: 0 files scanned"
    exit 0
}
$files = @(Get-ChildItem -LiteralPath $srcDir -Recurse -File -Include *.c | Sort-Object FullName)
if ($files.Count -eq 0) {
    Write-Output "analyze passed: 0 files scanned"
    exit 0
}

$clangArgs = @("--analyze", "-std=c23", "--target=x86_64-w64-mingw32", "-Xclang", "-analyzer-output=text")
$clangArgs += "-I" + (Join-Path $Root "third_party/sqlite").Replace('\', '/')
foreach ($inc in @("src", "third_party/raylib/src", "third_party/raygui/src")) {
    $dir = Join-Path $Root $inc
    if (Test-Path -LiteralPath $dir) { $clangArgs += "-I" + $dir.Replace('\', '/') }
}

$bad = 0
foreach ($f in $files) {
    $result = Invoke-Captured $clang.Source ($clangArgs + @($f.FullName))
    $diag = [regex]::Matches($result.text, ':(?:\d+:\d+)? (?:warning|error): ').Count
    if ($result.code -ne 0 -or $diag -gt 0) {
        $bad++
        Write-Output ("{0}: FAIL exit {1}, {2} diagnostics" -f $f.FullName, $result.code, $diag)
        if ($result.text.Trim().Length -gt 0) { Write-Output $result.text.TrimEnd() }
    } else {
        Write-Output ("{0}: ok" -f $f.FullName)
    }
}

if ($bad -gt 0) {
    Write-Output ("analyze failed: {0} of {1} files with diagnostics" -f $bad, $files.Count)
    exit 1
}
Write-Output ("analyze passed: {0} files scanned" -f $files.Count)
exit 0
