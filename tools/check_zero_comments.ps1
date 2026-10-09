param(
    [string]$Root = (Split-Path $PSScriptRoot -Parent)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$rules = @{
    ".c"   = @{ block = $true; lineMarkers = @("//"); stringDelims = @('"', "'"); escape = '\'; doubling = $false }
    ".h"   = @{ block = $true; lineMarkers = @("//"); stringDelims = @('"', "'"); escape = '\'; doubling = $false }
    ".sql" = @{ block = $true; lineMarkers = @("--"); stringDelims = @('"', "'"); escape = $null; doubling = $true }
    ".mjs" = @{ block = $true; lineMarkers = @("//"); stringDelims = @('"', "'"); escape = '\'; doubling = $false }
    ".ps1" = @{ block = $false; lineMarkers = @("#"); stringDelims = @('"', "'"); escape = '`'; doubling = $true }
    ".mmd" = @{ block = $false; lineMarkers = @("%%"); stringDelims = @('"', "'"); escape = '\'; doubling = $false }
}

function Get-CommentViolations {
    param([string]$Path, [hashtable]$Rule)

    $found = @()
    $lines = @(Get-Content -LiteralPath $Path)
    $inBlock = $false
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $n = $line.Length
        $j = 0
        $state = "code"
        $delim = [char]0
        while ($j -lt $n) {
            if ($inBlock) {
                $end = $line.IndexOf("*/", $j, [StringComparison]::Ordinal)
                if ($end -lt 0) { break }
                $inBlock = $false
                $j = $end + 2
                continue
            }
            $ch = $line[$j]
            if ($state -eq "code") {
                $matched = $false
                foreach ($marker in $Rule.lineMarkers) {
                    if ($ch -ne $marker[0]) { continue }
                    if ($marker.Length -gt 1 -and ($j + 1 -ge $n -or $line[$j + 1] -ne $marker[1])) { continue }
                    $found += [pscustomobject]@{ line = $i + 1; kind = $marker }
                    $matched = $true
                    break
                }
                if ($matched) { break }
                if ($Rule.block -and $ch -eq '/' -and $j + 1 -lt $n -and $line[$j + 1] -eq '*') {
                    $found += [pscustomobject]@{ line = $i + 1; kind = "/*" }
                    $inBlock = $true
                    $j += 2
                    continue
                }
                if ($Rule.stringDelims -contains $ch) {
                    $state = "string"
                    $delim = $ch
                    $j++
                    continue
                }
                $j++
            } else {
                if ($null -ne $Rule.escape -and $ch -eq $Rule.escape -and $j + 1 -lt $n) { $j += 2; continue }
                if ($Rule.doubling -and $ch -eq $delim -and $j + 1 -lt $n -and $line[$j + 1] -eq $delim) { $j += 2; continue }
                if ($ch -eq $delim) { $state = "code" }
                $j++
            }
        }
    }
    return $found
}

$scanRoots = @("src", "tests", "tools", "db", "docs/diagrams")
$files = @()
foreach ($scanRoot in $scanRoots) {
    $dir = Join-Path $Root $scanRoot
    if (-not (Test-Path -LiteralPath $dir)) { continue }
    $files += Get-ChildItem -LiteralPath $dir -Recurse -File | Where-Object { $rules.ContainsKey($_.Extension) }
}
$files = @($files | Sort-Object FullName)

$total = 0
$badFiles = 0
foreach ($file in $files) {
    $found = @(Get-CommentViolations -Path $file.FullName -Rule $rules[$file.Extension])
    if ($found.Count -eq 0) { continue }
    $badFiles++
    foreach ($v in $found) {
        Write-Output ("{0}:{1}: comment {2}" -f $file.FullName, $v.line, $v.kind)
        $total++
    }
}

if ($total -gt 0) {
    Write-Output ("zero comments failed: {0} violation lines in {1} of {2} files" -f $total, $badFiles, $files.Count)
    exit 1
}
Write-Output ("zero comments passed: {0} files scanned, 0 violations" -f $files.Count)
exit 0
