$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"
$stamp = "playground/gates/verify_progress.txt"
"$(Get-Date -Format o) verify orchestrator pid $PID" | Add-Content $stamp
Get-ChildItem build_gate/mutants -Filter *.journal -ErrorAction SilentlyContinue | Remove-Item -Force
"$(Get-Date -Format o) journals cleared" | Add-Content $stamp
"$(Get-Date -Format o) baseline build START" | Add-Content $stamp
make BUILDDIR=build_gate test 1> playground/gates/verify_baseline.log 2> playground/gates/verify_baseline.err
"$(Get-Date -Format o) baseline build EXIT=$LASTEXITCODE" | Add-Content $stamp
$order = @(
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
$plan = Get-Content playground/gate_plan.json -Raw | ConvertFrom-Json
$n = 0
foreach ($scope in $order) {
    $n++
    $suites = ($plan.$scope -join ",")
    $name = [System.IO.Path]::GetFileNameWithoutExtension($scope)
    "$(Get-Date -Format o) verify lane $n $scope suites=$($plan.$scope.Count) START" | Add-Content $stamp
    make BUILDDIR=build_gate mutate SCOPE=$scope "SUITE=$suites" 1> "playground/gates/verify_${n}_${name}.log" 2> "playground/gates/verify_${n}_${name}.err"
    "$(Get-Date -Format o) verify lane $n $scope EXIT=$LASTEXITCODE" | Add-Content $stamp
}
"$(Get-Date -Format o) verify orchestrator DONE" | Add-Content $stamp
