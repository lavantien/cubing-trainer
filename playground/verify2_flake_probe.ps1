$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"
$hdr = Select-String -Path "playground/gates/lane_02_script.log" -Pattern 'build_gate/bin/mutate\.exe src/app/script\.c' | Select-Object -First 1
$args0 = $hdr.Line -split ' '
$suites = $args0[3..($args0.Count-1)]
$j = "build_gate/mutants/src_app_script.c.journal"
Copy-Item $j "$j.flakebak" -Force
$env:C_INCLUDE_PATH = ''
$env:CPLUS_INCLUDE_PATH = ''
$env:CT_MUTATE_CC = 'clang'
$env:CT_MUTATE_CFLAGS = '--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite'
$env:CT_MUTATE_LDFLAGS = '--target=x86_64-w64-mingw32 -fuse-ld=lld -g'
$env:CT_MUTATE_LDLIBS = '-static-libgcc -lopengl32 -lgdi32 -lwinmm'
$env:CT_MUTATE_ONLY = '115,500,652,656,672,673,674,792,793'
$env:CT_MUTATE_RESUME = '0'
for ($round = 1; $round -le 5; $round++) {
    & build_gate/bin/mutate.exe src/app/script.c build_gate @suites 1> "playground/gates/verify2_flake_r$round.log" 2> $null
    $k = (Select-String -Path "playground/gates/verify2_flake_r$round.log" -Pattern '^KILLED').Count
    $s = (Select-String -Path "playground/gates/verify2_flake_r$round.log" -Pattern '^SURVIVED').Count
    "round $round exit=$LASTEXITCODE killed=$k survived=$s"
}
Copy-Item "$j.flakebak" $j -Force
Remove-Item "$j.flakebak"
"journal restored"
