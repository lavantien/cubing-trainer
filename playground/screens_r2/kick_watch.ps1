param(
    [string]$BuildDir = "build_scrn2",
    [string]$KicksLog = "playground/screens_r2/kick_watch.txt",
    [int]$KickAge = 330
)
$driverPid = $null
while ($null -eq $driverPid) {
    if (Test-Path "$BuildDir/mutants") {
        $dir = Get-ChildItem "$BuildDir/mutants" -Directory -Filter "p*" | Select-Object -First 1
        if ($dir) { $driverPid = $dir.Name.Substring(1) }
    }
    if ($null -eq $driverPid) { Start-Sleep -Seconds 5 }
}
"driver pid $driverPid detected $(Get-Date -Format o)" | Add-Content $KicksLog
$childName = "ct_mutant_p$driverPid"
while ($true) {
    $driverAlive = Get-Process -Id ([int]$driverPid) -ErrorAction SilentlyContinue
    if (-not $driverAlive) {
        "driver $driverPid gone $(Get-Date -Format o)" | Add-Content $KicksLog
        break
    }
    $kids = Get-Process -Name $childName -ErrorAction SilentlyContinue
    foreach ($k in $kids) {
        $age = ((Get-Date) - $k.StartTime).TotalSeconds
        if ($age -gt $KickAge) {
            $newest = Get-ChildItem "$BuildDir/mutants/p$driverPid" -Filter "m*_*.c" -ErrorAction SilentlyContinue |
                Sort-Object LastWriteTime | Select-Object -Last 1
            $cur = if ($newest) { $newest.Name } else { "unknown" }
            "kick $(Get-Date -Format o) pid $($k.Id) age $([int]$age)s mutant $cur" | Add-Content $KicksLog
            Stop-Process -Id $k.Id -Force
        }
    }
    Start-Sleep -Seconds 15
}
