#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/obj/tools build/bin
clang --target=x86_64-w64-mingw32 -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite -c tools/render_scene.c -o build/obj/tools/render_scene.o
objs="$(
	find build/obj/src build/obj/third_party/raylib/src -name '*.o' ! -path 'build/obj/src/app/main.o'
	echo build/obj/tools/render_scene.o build/obj/raygui_impl.o build/obj/third_party/sqlite/sqlite3.o
)"
clang --target=x86_64-w64-mingw32 -fuse-ld=lld -o build/bin/render_scene.exe $objs -static-libgcc -lopengl32 -lgdi32 -lwinmm
echo BUILD_OK
