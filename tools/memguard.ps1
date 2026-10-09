param(
    [long]$CapBytes = 0,
    [switch]$Kill
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path.Replace('\', '/').ToLowerInvariant()

function Format-Bytes([long]$value) {
    if ($value -ge 1GB) { '{0:F2} GiB' -f ($value / 1GB) }
    elseif ($value -ge 1MB) { '{0:F2} MiB' -f ($value / 1MB) }
    elseif ($value -ge 1KB) { '{0:F2} KiB' -f ($value / 1KB) }
    else { "$value B" }
}

if ($CapBytes -le 0) {
    $physical = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory
    $CapBytes = [math]::Floor($physical / 4)
}

$rows = @()
foreach ($proc in Get-CimInstance Win32_Process) {
    if ($proc.ProcessId -eq $PID) { continue }
    $cmd = if ($null -ne $proc.CommandLine) { $proc.CommandLine.Replace('\', '/').ToLowerInvariant() } else { $null }
    $exe = if ($null -ne $proc.ExecutablePath) { $proc.ExecutablePath.Replace('\', '/').ToLowerInvariant() } else { $null }
    $hit = ($cmd -and $cmd.Contains($repo)) -or ($exe -and $exe.Contains($repo))
    if (-not $hit) { continue }
    $live = Get-Process -Id $proc.ProcessId -ErrorAction SilentlyContinue
    if ($null -eq $live) { continue }
    $rows += [pscustomobject]@{ Id = $live.Id; Name = $live.ProcessName; WS = [long]$live.WorkingSet64 }
}

$total = [long](($rows | Measure-Object -Property WS -Sum).Sum)

"repo: $repo"
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
