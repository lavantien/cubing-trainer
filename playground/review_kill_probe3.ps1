$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"
$v = (Select-String -Path "playground/gates/verify_4_screen_review_full.log" -Pattern 'build_gate/bin/mutate\.exe src/ui/screen_review\.c' | Select-Object -First 1).Line -split ' '
$suites = $v[3..($v.Count-1)] + @('review_form_memo_second')
$j = "build_gate/mutants/src_ui_screen_review.c.journal"
Copy-Item $j "$j.kbak" -Force
$env:C_INCLUDE_PATH = ''
$env:CPLUS_INCLUDE_PATH = ''
$env:CT_MUTATE_CC = 'clang'
$env:CT_MUTATE_CFLAGS = '--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite'
$env:CT_MUTATE_LDFLAGS = '--target=x86_64-w64-mingw32 -fuse-ld=lld -g'
$env:CT_MUTATE_LDLIBS = '-static-libgcc -lopengl32 -lgdi32 -lwinmm'
$env:CT_MUTATE_ONLY = '64,66,67,68,69,70,71,75,79,80'
$env:CT_MUTATE_RESUME = '0'
& build_gate/bin/mutate.exe src/ui/screen_review.c build_gate @suites 1> "playground/gates/review_kill3.log" 2> $null
"exit=$LASTEXITCODE"
Get-Content "playground/gates/review_kill3.log" | Where-Object { $_ -like 'KILLED*' -or $_ -like 'SURVIVED*' -or $_ -like 'mutate:*' }
Copy-Item "$j.kbak" $j -Force
Remove-Item "$j.kbak"
"journal restored"
