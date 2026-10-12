$limitSeconds = 330
function ToDate($value) {
    if ($value -is [datetime]) { return $value }
    if ($value -is [string]) { return [System.Management.ManagementDateTimeConverter]::ToDateTime($value) }
    return $null
}
while ($true) {
    Start-Sleep -Seconds 30
    $now = Get-Date
    $drivers = Get-CimInstance Win32_Process -Filter "Name = 'mutate.exe'" -ErrorAction SilentlyContinue
    $driverIds = @($drivers | ForEach-Object { $_.ProcessId })
    Get-CimInstance Win32_Process -Filter "Name like 'ct_mutant%'" -ErrorAction SilentlyContinue | ForEach-Object {
        $started = ToDate $_.CreationDate
        if ($null -ne $started) {
            $age = ($now - $started).TotalSeconds
            if ($age -gt $limitSeconds) {
                $victim = $_.ProcessId
                Write-Output "$(Get-Date -Format o) kick ct_mutant $victim age $([int]$age)s cmd $($_.CommandLine)"
                Stop-Process -Id $victim -Force -ErrorAction SilentlyContinue
            }
        }
    }
    if ($driverIds.Count -gt 0) {
        Get-CimInstance Win32_Process -Filter "Name = 'cmd.exe'" -ErrorAction SilentlyContinue | Where-Object {
            $driverIds -contains $_.ParentProcessId -and $_.CommandLine -like '*ct_mutant*'
        } | ForEach-Object {
            $started = ToDate $_.CreationDate
            if ($null -ne $started) {
                $age = ($now - $started).TotalSeconds
                if ($age -gt $limitSeconds) {
                    $victim = $_.ProcessId
                    Write-Output "$(Get-Date -Format o) kick cmd $victim age $([int]$age)s cmd $($_.CommandLine)"
                    Stop-Process -Id $victim -Force -ErrorAction SilentlyContinue
                }
            }
        }
    }
}
