param(
    [string]$Root = (Split-Path $PSScriptRoot -Parent)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (Test-Path -LiteralPath $Root) { $Root = (Get-Item -LiteralPath $Root).FullName }

$configHubRel = "src/config/config.h"
$allowPath = Join-Path $PSScriptRoot "stray_constants_allow.txt"
$recurThreshold = 3
$allowedCores = @("0", "1", "2")
$numberPattern = '(?<![\w.$])(?:0[xX][0-9a-fA-F]+[uUlL]*|\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?[uUlLfF]*(?![\w.])'

$allowEntries = @()
if (Test-Path -LiteralPath $allowPath) {
    foreach ($raw in @(Get-Content -LiteralPath $allowPath)) {
        $entry = $raw.Trim()
        if ($entry.Length -eq 0) { continue }
        $allowEntries += $entry
    }
}

function Test-Allowed {
    param([string]$RelPath, [int]$LineNumber)

    foreach ($entry in $allowEntries) {
        $idx = $entry.LastIndexOf(':')
        if ($idx -gt 0 -and $entry.Substring($idx + 1) -match '^\d+$') {
            $filePart = $entry.Substring(0, $idx)
            if ($RelPath -like $filePart -and [int]$entry.Substring($idx + 1) -eq $LineNumber) { return $true }
        } else {
            if ($RelPath -like $entry) { return $true }
        }
    }
    return $false
}

function Get-MaskedLines {
    param([string]$Path)

    $lines = @(Get-Content -LiteralPath $Path)
    $masked = [string[]]::new($lines.Count)
    $inBlock = $false
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $chars = $lines[$i].ToCharArray()
        $n = $chars.Count
        $j = 0
        $state = "code"
        $delim = [char]0
        while ($j -lt $n) {
            if ($inBlock) {
                $end = -1
                for ($k = $j; $k -lt $n - 1; $k++) {
                    if ($chars[$k] -eq '*' -and $chars[$k + 1] -eq '/') { $end = $k; break }
                }
                if ($end -lt 0) {
                    for ($k = $j; $k -lt $n; $k++) { $chars[$k] = ' ' }
                    break
                }
                for ($k = $j; $k -lt $end + 2; $k++) { $chars[$k] = ' ' }
                $inBlock = $false
                $j = $end + 2
                continue
            }
            $ch = $chars[$j]
            if ($state -eq "code") {
                if ($ch -eq '/' -and $j + 1 -lt $n -and $chars[$j + 1] -eq '/') {
                    for ($k = $j; $k -lt $n; $k++) { $chars[$k] = ' ' }
                    break
                }
                if ($ch -eq '/' -and $j + 1 -lt $n -and $chars[$j + 1] -eq '*') {
                    $chars[$j] = ' '
                    $chars[$j + 1] = ' '
                    $inBlock = $true
                    $j += 2
                    continue
                }
                if ($ch -eq '"' -or $ch -eq "'") {
                    $state = "string"
                    $delim = $ch
                    $chars[$j] = ' '
                    $j++
                    continue
                }
                $j++
            } else {
                if ($ch -eq '\') {
                    $chars[$j] = ' '
                    if ($j + 1 -lt $n) { $chars[$j + 1] = ' '; $j += 2; continue }
                }
                if ($ch -eq $delim) {
                    $state = "code"
                    $chars[$j] = ' '
                    $j++
                    continue
                }
                $chars[$j] = ' '
                $j++
            }
        }
        $masked[$i] = -join $chars
    }
    return , $masked
}

$srcDir = Join-Path $Root "src"
if (-not (Test-Path -LiteralPath $srcDir)) {
    Write-Output "stray constants passed: 0 files scanned, 0 violations"
    exit 0
}
$files = @(Get-ChildItem -LiteralPath $srcDir -Recurse -File -Include *.c, *.h | Sort-Object FullName)

$violations = @()
foreach ($file in $files) {
    $rel = $file.FullName.Substring($Root.Length + 1).Replace('\', '/')
    if ($rel -eq $configHubRel) { continue }
    $masked = Get-MaskedLines -Path $file.FullName
    $direct = @()
    $pending = @{}
    for ($i = 0; $i -lt $masked.Count; $i++) {
        foreach ($m in [regex]::Matches($masked[$i], $numberPattern)) {
            $literal = $m.Value
            $isHex = $literal -match '^0[xX]'
            if ($isHex) {
                $core = $literal -replace '^0[xX]', ''
            } else {
                $core = $literal -replace '[uUlLfF]+$', ''
            }
            if ($allowedCores -contains $core) { continue }
            $isDecimal = $core.Contains('.')
            $digits = ($core -replace '[^0-9]', '').Length
            if (-not $isHex -and -not $isDecimal -and $digits -le 1) {
                if (-not $pending.ContainsKey($core)) { $pending[$core] = @() }
                $pending[$core] += $i + 1
            } else {
                $reason = if ($isHex) { "hex" } elseif ($isDecimal) { "decimal" } else { "multi-digit" }
                $direct += [pscustomobject]@{ line = $i + 1; literal = $literal; reason = $reason }
            }
        }
    }
    foreach ($d in $direct) {
        if (Test-Allowed -RelPath $rel -LineNumber $d.line) { continue }
        $violations += [pscustomobject]@{ path = $rel; line = $d.line; literal = $d.literal; reason = $d.reason }
    }
    foreach ($core in @($pending.Keys)) {
        $hits = $pending[$core]
        if ($hits.Count -lt $recurThreshold) { continue }
        foreach ($line in $hits) {
            if (Test-Allowed -RelPath $rel -LineNumber $line) { continue }
            $violations += [pscustomobject]@{ path = $rel; line = $line; literal = $core; reason = "recurring" }
        }
    }
}

$violations = @($violations | Sort-Object path, line)
if ($violations.Count -gt 0) {
    foreach ($v in $violations) {
        Write-Output ("{0}:{1}: {2} ({3})" -f $v.path, $v.line, $v.literal, $v.reason)
    }
    Write-Output ("stray constants failed: {0} violations in {1} files scanned" -f $violations.Count, $files.Count)
    exit 1
}
Write-Output ("stray constants passed: {0} files scanned, 0 violations" -f $files.Count)
exit 0
