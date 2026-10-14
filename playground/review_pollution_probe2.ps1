$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"
$t = (Select-String -Path "playground/gates/lane_04_screen_review.log" -Pattern 'build_gate/bin/mutate\.exe src/ui/screen_review\.c' | Select-Object -First 1).Line -split ' '
$tri = $t[3..($t.Count-1)]
$ids = '64,66,67,68,69,70,71,75,79,80'
$j = "build_gate/mutants/src_ui_screen_review.c.journal"
Copy-Item $j "$j.polbak2" -Force
$env:C_INCLUDE_PATH = ''
$env:CPLUS_INCLUDE_PATH = ''
$env:CT_MUTATE_CC = 'clang'
$env:CT_MUTATE_CFLAGS = '--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite'
$env:CT_MUTATE_LDFLAGS = '--target=x86_64-w64-mingw32 -fuse-ld=lld -g'
$env:CT_MUTATE_LDLIBS = '-static-libgcc -lopengl32 -lgdi32 -lwinmm'
$env:CT_MUTATE_ONLY = $ids
$env:CT_MUTATE_RESUME = '0'

function Probe($name, $suites) {
    & build_gate/bin/mutate.exe src/ui/screen_review.c build_gate @suites 1> "playground/gates/review_poll2_$name.log" 2> $null
    $log = Get-Content "playground/gates/review_poll2_$name.log"
    $k = ($log | Where-Object { $_ -like 'KILLED*' }).Count
    $s = ($log | Where-Object { $_ -like 'SURVIVED*' }).Count
    "$name suites=$($suites.Count) exit=$LASTEXITCODE killed=$k survived=$s"
}

Probe "triage24" $tri
Probe "memo_pairs_solo" @('review_form_memo_pairs')
Probe "memo_idle_solo" @('review_form_memo_idle')
Probe "auto_open_solo" @('review_screen_auto_open')

Copy-Item "$j.polbak2" $j -Force
Remove-Item "$j.polbak2"
"journal restored"
