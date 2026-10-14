param(
    [string]$BuildDir = "build_gate",
    [string]$KicksLog = "playground/gates/kick_watch2.txt",
    [string]$DoneFile = "playground/gates/verify_progress.txt",
    [int]$KickAge = 330
)
$started = Get-Date
while ($true) {
    $done = Get-Content $DoneFile -ErrorAction SilentlyContinue | Where-Object { $_ -like '*orchestrator DONE*' } |
        ForEach-Object { [datetime]::Parse($_.Substring(0, $_.IndexOf(' '))) } | Sort-Object | Select-Object -Last 1
    if ($done -and $done -gt $started) {
        "all lanes done $(Get-Date -Format o)" | Add-Content $KicksLog
        break
    }
    $dir = Get-ChildItem "$BuildDir/mutants" -Directory -Filter "p*" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime | Select-Object -Last 1
    if (-not $dir) {
        Start-Sleep -Seconds 10
        continue
    }
    $driverPid = $dir.Name.Substring(1)
    $alive = Get-Process -Id ([int]$driverPid) -ErrorAction SilentlyContinue
    if (-not $alive) {
        Start-Sleep -Seconds 2
        continue
    }
    $childName = "ct_mutant_p$driverPid"
    "driver pid $driverPid detected $(Get-Date -Format o)" | Add-Content $KicksLog
    while (Get-Process -Id ([int]$driverPid) -ErrorAction SilentlyContinue) {
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
    "driver $driverPid gone $(Get-Date -Format o)" | Add-Content $KicksLog
}
