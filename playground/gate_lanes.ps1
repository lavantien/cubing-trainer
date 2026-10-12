$ErrorActionPreference = "Continue"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$gates = Join-Path $root "playground" "gates"
$progress = Join-Path $gates "progress.txt"
New-Item -ItemType Directory -Force $gates | Out-Null

$plan = Get-Content (Join-Path $PSScriptRoot "gate_plan.json") -Raw | ConvertFrom-Json
$scopes = @(
    "src/ui/screen_trainer.c",
    "src/app/script.c",
    "src/ui/screen_settings.c",
    "src/ui/screen_review.c",
    "src/ui/screen_cards.c",
    "src/ui/screen_stats.c",
    "src/ui/widgets.c",
    "src/ui/screen_menu.c",
    "src/app/shots.c",
    "src/app/paths.c",
    "src/app/app_loop.c",
    "src/app/script_expect.c"
)

function Stamp($line) {
    Add-Content -Path $progress -Value "$(Get-Date -Format o) $line"
}

Stamp "orchestrator pid $PID"
$log = Join-Path $gates "baseline_build.log"
$err = Join-Path $gates "baseline_build.err"
Stamp "baseline build START"
& make BUILDDIR=build_gate test 1> $log 2> $err
$code = $LASTEXITCODE
Stamp "baseline build EXIT=$code"
if ($code -eq 0) {
    $i = 0
    foreach ($scope in $scopes) {
        $i++
        $suites = $plan.$scope -join ","
        $name = [IO.Path]::GetFileNameWithoutExtension($scope)
        $log = Join-Path $gates ("lane_{0:d2}_{1}.log" -f $i, $name)
        $err = Join-Path $gates ("lane_{0:d2}_{1}.err" -f $i, $name)
        Stamp "lane $i $scope suites=$($plan.$scope.Count) START"
        & make BUILDDIR=build_gate mutate SCOPE=$scope "SUITE=$suites" 1> $log 2> $err
        $code = $LASTEXITCODE
        if ($code -ne 0) {
            Stamp "lane $i $scope EXIT=$code retry once"
            $log = Join-Path $gates ("lane_{0:d2}_{1}.retry.log" -f $i, $name)
            $err = Join-Path $gates ("lane_{0:d2}_{1}.retry.err" -f $i, $name)
            & make BUILDDIR=build_gate mutate SCOPE=$scope "SUITE=$suites" 1> $log 2> $err
            $code = $LASTEXITCODE
        }
        Stamp "lane $i $scope EXIT=$code"
    }
}
Stamp "orchestrator DONE"
