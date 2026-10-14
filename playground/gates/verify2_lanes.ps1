$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"
$stamp = "playground/gates/verify2_progress.txt"
"$(Get-Date -Format o) verify2 orchestrator pid $PID" | Add-Content $stamp
$order = @(
    "src/app/script.c",
    "src/ui/screen_review.c",
    "src/app/paths.c",
    "src/ui/screen_trainer.c"
)
foreach ($scope in $order) {
    Remove-Item "build_gate/mutants/$($scope -replace '/', '_').journal" -Force -ErrorAction SilentlyContinue
}
"$(Get-Date -Format o) journals cleared" | Add-Content $stamp
"$(Get-Date -Format o) baseline build START" | Add-Content $stamp
make BUILDDIR=build_gate test 1> playground/gates/verify2_baseline.log 2> playground/gates/verify2_baseline.err
"$(Get-Date -Format o) baseline build EXIT=$LASTEXITCODE" | Add-Content $stamp
$plan = Get-Content playground/gate_plan.json -Raw | ConvertFrom-Json
$n = 0
foreach ($scope in $order) {
    $n++
    $suites = ($plan.$scope -join ",")
    $name = [System.IO.Path]::GetFileNameWithoutExtension($scope)
    "$(Get-Date -Format o) verify2 lane $n $scope suites=$($plan.$scope.Count) START" | Add-Content $stamp
    make BUILDDIR=build_gate mutate SCOPE=$scope "SUITE=$suites" 1> "playground/gates/verify2_${n}_${name}.log" 2> "playground/gates/verify2_${n}_${name}.err"
    "$(Get-Date -Format o) verify2 lane $n $scope EXIT=$LASTEXITCODE" | Add-Content $stamp
}
"$(Get-Date -Format o) verify2 orchestrator DONE" | Add-Content $stamp
