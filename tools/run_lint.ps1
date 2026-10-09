param(
    [string]$Root = (Split-Path $PSScriptRoot -Parent),
    [string]$GccInstallDir = "C:/Users/lavantien/scoop/apps/mingw-winlibs-ucrt/current/lib/gcc/x86_64-w64-mingw32/16.2.0"
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

$failures = @()

Write-Output "component: zero comments"
$zero = Invoke-Captured "pwsh" @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $PSScriptRoot "check_zero_comments.ps1"), "-Root", $Root)
Write-Output $zero.text.TrimEnd()
if ($zero.code -ne 0) { $failures += "zero comments" }

Write-Output "component: stray constants"
$stray = Invoke-Captured "pwsh" @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $PSScriptRoot "check_stray_constants.ps1"), "-Root", $Root)
Write-Output $stray.text.TrimEnd()
if ($stray.code -ne 0) { $failures += "stray constants" }

Write-Output "component: whitespace"
$wsFiles = @()
foreach ($target in @("src", "tests")) {
    $dir = Join-Path $Root $target
    if (-not (Test-Path -LiteralPath $dir)) { continue }
    $wsFiles += Get-ChildItem -LiteralPath $dir -Recurse -File -Include *.c, *.h
}
$wsFiles = @($wsFiles | Sort-Object FullName)
$wsCount = 0
foreach ($f in $wsFiles) {
    $lines = @(Get-Content -LiteralPath $f.FullName)
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line.Contains("`t")) {
            Write-Output ("{0}:{1}: tab" -f $f.FullName, ($i + 1))
            $wsCount++
        } elseif ($line -match '[ \t]+$') {
            Write-Output ("{0}:{1}: trailing whitespace" -f $f.FullName, ($i + 1))
            $wsCount++
        }
    }
}
if ($wsCount -gt 0) {
    $failures += "whitespace"
} else {
    Write-Output ("whitespace passed: {0} files, 0 violations" -f $wsFiles.Count)
}

Write-Output "component: clang-tidy"
$tidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if ($null -eq $tidy) {
    Write-Output "clang-tidy skipped: not found on PATH"
} else {
    $tidyFiles = @(Get-ChildItem -LiteralPath (Join-Path $Root "src") -Recurse -File -Include *.c | Sort-Object FullName)
    $compileArgs = @("-std=c23", "-I" + (Join-Path $Root "src").Replace('\', '/'))
    foreach ($inc in @("third_party/sqlite", "third_party/raylib/src", "third_party/raygui/src")) {
        $dir = Join-Path $Root $inc
        if (Test-Path -LiteralPath $dir) { $compileArgs += "-I" + $dir.Replace('\', '/') }
    }
    if (Test-Path -LiteralPath $GccInstallDir) {
        $compileArgs = @("--target=x86_64-w64-mingw32", "--gcc-install-dir=$GccInstallDir") + $compileArgs
    } else {
        Write-Output "note: gcc-install-dir $GccInstallDir missing, header resolution may fail"
    }
    $tidyChecks = "-*,clang-analyzer-*,bugprone-*,-bugprone-signed-bitwise,-bugprone-easily-swappable-parameters,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling"
    $srcFilter = [regex]::Escape((Join-Path $Root "src").Replace('\', '/')).Replace('/', '[\\/]')
    $tidyArgs = @("--quiet", "-checks=$tidyChecks", "--header-filter=^$srcFilter") + @($tidyFiles | ForEach-Object { $_.FullName }) + @("--") + $compileArgs
    $result = Invoke-Captured $tidy.Source $tidyArgs
    if ($result.text.Trim().Length -gt 0) { Write-Output $result.text.TrimEnd() }
    $diag = [regex]::Matches($result.text, ':(?:\d+:\d+)? (?:warning|error): ').Count
    if ($result.code -ne 0 -or $diag -gt 0) {
        Write-Output ("clang-tidy failed: exit {0}, {1} diagnostics" -f $result.code, $diag)
        $failures += "clang-tidy"
    } else {
        Write-Output ("clang-tidy passed: {0} files, 0 diagnostics" -f $tidyFiles.Count)
    }
}

if ($failures.Count -gt 0) {
    Write-Output ("lint failed: " + ($failures -join ", "))
    exit 1
}
Write-Output "lint passed"
exit 0
