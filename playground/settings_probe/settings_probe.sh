#!/usr/bin/env bash
set -euo pipefail

CC=clang
CROSS="--target=x86_64-w64-mingw32"
CROSS_LINK="--target=x86_64-w64-mingw32 -fuse-ld=lld"
CFLAGS="-std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all"
INC="-Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite"
RAYLIB_CFLAGS="-DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -D_GNU_SOURCE -DUNICODE -fno-strict-aliasing -Wno-missing-braces -Wno-error -Ithird_party/raylib/src -Ithird_party/raylib/src/external/glfw/include"
RAYGUI_CFLAGS="-Ithird_party/raylib/src -Ithird_party/raygui/src -Wno-error"
OUT=playground/settings_probe
OBJ="$OUT/obj"
mkdir -p "$OBJ"

$CC $CROSS -std=c23 -Werror -Wpedantic -Isrc -Ithird_party/sqlite -fsyntax-only tests/unit_settings_form.c

for f in tests/unit_settings_form.c tests/framework.c tests/test_main.c src/config/config.c \
	src/storage/db.c src/storage/migrate.c src/storage/repo_settings.c src/ui/screen_settings.c; do
	o="$OBJ/$(echo "$f" | tr '/' '_').o"
	$CC $CROSS $CFLAGS $INC -c "$f" -o "$o"
done

echo '#define RAYGUI_IMPLEMENTATION' >"$OUT/raygui_impl.c"
echo '#include "raygui.h"' >>"$OUT/raygui_impl.c"
$CC $CROSS $CFLAGS $RAYGUI_CFLAGS -c "$OUT/raygui_impl.c" -o "$OBJ/raygui_impl.o"

for f in third_party/raylib/src/*.c; do
	o="$OBJ/$(basename "$f" .c).o"
	$CC $CROSS $CFLAGS $RAYLIB_CFLAGS -c "$f" -o "$o"
done

$CC $CROSS $CFLAGS -Ithird_party/sqlite -Wno-error -c third_party/sqlite/sqlite3.c -o "$OBJ/sqlite3.o"

$CC $CROSS_LINK -g -o "$OUT/settings_probe.exe" "$OBJ"/tests_*.o "$OBJ"/src_*.o \
	"$OBJ/raygui_impl.o" "$OBJ"/*.o "$OBJ/sqlite3.o" -static-libgcc -lopengl32 -lgdi32 -lwinmm

"$OUT/settings_probe.exe" "${1:-}"
