$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"
$env:C_INCLUDE_PATH = ''
$env:CPLUS_INCLUDE_PATH = ''
$env:CT_MUTATE_CC = 'clang'
$env:CT_MUTATE_CFLAGS = '--target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite'
$env:CT_MUTATE_LDFLAGS = '--target=x86_64-w64-mingw32 -fuse-ld=lld -g'
$env:CT_MUTATE_LDLIBS = '-static-libgcc -lopengl32 -lgdi32 -lwinmm'
$env:CT_MUTATE_ONLY = '64'
$env:CT_MUTATE_RESUME = '0'
$v = (Select-String -Path 'playground/gates/verify_4_screen_review_full.log' -Pattern 'build_gate/bin/mutate\.exe src/ui/screen_review\.c' | Select-Object -First 1).Line -split ' '
$suites = $v[3..($v.Count-1)] + @('review_form_memo_second')
$lock = 'build_gate/mutants/src_ui_screen_review.c.lock'
"999999" | Out-File -FilePath $lock -Encoding ascii -NoNewline
"planted stale lock with pid 999999"
& build_gate/bin/mutate.exe src/ui/screen_review.c build_gate @suites 1> playground/gates/mutate_smoke2.log 2> playground/gates/mutate_smoke2.err
"exit=$LASTEXITCODE"
Get-Content playground/gates/mutate_smoke2.err | Select-Object -First 3
Get-Content playground/gates/mutate_smoke2.log | Where-Object { $_ -like 'mutate:*' }
"lock exists after: $(Test-Path $lock)"
