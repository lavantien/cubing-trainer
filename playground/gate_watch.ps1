$limitSeconds = 330
while ($true) {
    Start-Sleep -Seconds 30
    $now = Get-Date
    Get-CimInstance Win32_Process -Filter "Name like 'ct_mutant%'" -ErrorAction SilentlyContinue | ForEach-Object {
        $started = $_.CreationDate
        if ($started -is [datetime]) {
            $age = ($now - $started).TotalSeconds
            if ($age -gt $limitSeconds) {
                $pid = $_.ProcessId
                Write-Output "kick $pid age $([int]$age)s cmd $($_.CommandLine)"
                Stop-Process -Id $pid -Force -ErrorAction SilentlyContinue
            }
        }
    }
}
