param(
    [long]$CapBytes = 0,
    [switch]$Kill,
    [switch]$SelfTest,
    [string]$SleeperPath = ""
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$repoNorm = $repo.Replace('\', '/').ToLowerInvariant().TrimEnd('/') + '/'

function Format-Bytes([long]$value) {
    if ($value -ge 1GB) { '{0:F2} GiB' -f ($value / 1GB) }
    elseif ($value -ge 1MB) { '{0:F2} MiB' -f ($value / 1MB) }
    elseif ($value -ge 1KB) { '{0:F2} KiB' -f ($value / 1KB) }
    else { "$value B" }
}

function Resolve-ExePath([string]$path) {
    if ($null -eq $path -or $path.Length -eq 0) { return $null }
    if (-not $path.Contains('~')) { return $path }
    try { return (Get-Item -LiteralPath $path -ErrorAction Stop).FullName } catch { return $path }
}

function Get-RepoProcess {
    $rows = @()
    foreach ($proc in Get-CimInstance Win32_Process) {
        if ($proc.ProcessId -eq $PID) { continue }
        $exe = Resolve-ExePath $proc.ExecutablePath
        if ($null -eq $exe) { continue }
        $norm = $exe.Replace('\', '/').ToLowerInvariant()
        if (-not $norm.StartsWith($repoNorm)) { continue }
        $live = Get-Process -Id $proc.ProcessId -ErrorAction SilentlyContinue
        if ($null -eq $live) { continue }
        $rows += [pscustomobject]@{ Id = $live.Id; Name = $live.ProcessName; WS = [long]$live.WorkingSet64 }
    }
    return $rows
}

function Stop-TestProcess([System.Diagnostics.Process]$p) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    $deadline = (Get-Date).AddSeconds(5)
    while ((Get-Date) -lt $deadline) {
        if ($null -eq (Get-Process -Id $p.Id -ErrorAction SilentlyContinue)) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $null -eq (Get-Process -Id $p.Id -ErrorAction SilentlyContinue)
}

function Invoke-MemguardSelfTest {
    if ($SleeperPath -eq '' -or -not (Test-Path -LiteralPath $SleeperPath)) {
        Write-Host "memguard selftest failed: sleeper binary not found: $SleeperPath"
        return 4
    }
    $sleeperFull = (Resolve-Path -LiteralPath $SleeperPath).Path
    if (-not $sleeperFull.StartsWith($repo, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Host "memguard selftest failed: sleeper binary is outside the repo: $sleeperFull"
        return 4
    }
    $fso = New-Object -ComObject Scripting.FileSystemObject
    $shortDir = $fso.GetFolder($repo).ShortPath
    $shortExe = $shortDir + $sleeperFull.Substring($repo.Length)
    $decoyExe = (Get-Command pwsh).Source
    $decoyArgs = @('-NoProfile', '-Command', "Start-Sleep -Seconds 120 # $repo")
    $sleeper = $null
    $decoy = $null
    $code = 5
    try {
        $sleeper = Start-Process -FilePath $shortExe -PassThru
        $decoy = Start-Process -FilePath $decoyExe -ArgumentList $decoyArgs -PassThru
        $cmdLineHit = (Get-CimInstance Win32_Process -Filter "ProcessId = $($decoy.Id)").CommandLine.Contains($repo)
        if (-not $cmdLineHit) {
            $decoy.Kill()
            Write-Host "memguard selftest failed: decoy command line does not mention the repo path"
            return 5
        }
        Start-Sleep -Seconds 1
        $rows = @(Get-RepoProcess)
        $sleeperCounted = $null -ne ($rows | Where-Object { $_.Id -eq $sleeper.Id })
        $decoyCounted = $null -ne ($rows | Where-Object { $_.Id -eq $decoy.Id })
        Write-Host ("memguard selftest: sleeper launched via {0}" -f $shortExe)
        Write-Host ("memguard selftest: decoy pid {0} carries repo path in command line only" -f $decoy.Id)
        Write-Host ("memguard selftest: sleeper counted = {0}, decoy counted = {1}" -f $sleeperCounted, $decoyCounted)
        if ($sleeperCounted -and -not $decoyCounted) { $code = 0 }
    } finally {
        $sleeperGone = $true
        $decoyGone = $true
        if ($null -ne $sleeper) { $sleeperGone = Stop-TestProcess $sleeper }
        if ($null -ne $decoy) { $decoyGone = Stop-TestProcess $decoy }
        if (-not $sleeperGone -or -not $decoyGone) {
            Write-Host "memguard selftest failed: test process survived cleanup"
            $code = 5
        }
    }
    return $code
}

if ($SelfTest) {
    $selfTestCode = Invoke-MemguardSelfTest
    exit $selfTestCode
}

if ($CapBytes -le 0) {
    $physical = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory
    $CapBytes = [math]::Floor($physical / 4)
}

$rows = @(Get-RepoProcess)
$total = [long](($rows | Measure-Object -Property WS -Sum).Sum)

"repo: $repoNorm"
"processes: $($rows.Count)"
"total: $total bytes ($(Format-Bytes $total))"
"cap: $CapBytes bytes ($(Format-Bytes $CapBytes))"

foreach ($row in ($rows | Sort-Object WS -Descending | Select-Object -First 5)) {
    '{0} {1} {2} bytes ({3})' -f $row.Id, $row.Name, $row.WS, (Format-Bytes $row.WS)
}

if ($total -gt $CapBytes -and $Kill) {
    $victim = $rows | Sort-Object WS -Descending | Select-Object -First 1
    if ($null -ne $victim) {
        "killing pid $($victim.Id) $($victim.Name)"
        taskkill /PID $victim.Id /T /F
        $deadline = (Get-Date).AddSeconds(5)
        while ((Get-Date) -lt $deadline) {
            if ($null -eq (Get-Process -Id $victim.Id -ErrorAction SilentlyContinue)) { break }
            Start-Sleep -Milliseconds 250
        }
        if ($null -eq (Get-Process -Id $victim.Id -ErrorAction SilentlyContinue)) {
            "pid $($victim.Id) verified gone"
        } else {
            "pid $($victim.Id) still alive after 5 seconds"
        }
        exit 3
    }
}
exit 0
