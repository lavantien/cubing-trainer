# toolchain spike report

Date 2026-10-09. Environment: LLVM clang 23.1.3, winlibs UCRT gcc 16.2.0 sysroot at C:/Users/lavantien/scoop/apps/mingw-winlibs-ucrt/current, GNU make 4.4.1 with SHELL resolved to C:/Users/lavantien/scoop/shims/sh.exe so recipes run POSIX sh.

## outcome

The app compiles and links with clang targeting x86_64-w64-mingw32 over the winlibs sysroot and links with lld. The frozen spelling is a split: compile lines carry the target triple only, link lines add -fuse-ld=lld. The plan primary spelling with --gcc-install-dir was rejected by evidence below.

## candidates

Candidate 1, the plan primary: --target=x86_64-w64-mingw32 --gcc-install-dir=C:/Users/lavantien/scoop/apps/mingw-winlibs-ucrt/current/lib/gcc/x86_64-w64-mingw32/16.2.0 -fuse-ld=lld.

Failed. clang 23.1.3 classifies --gcc-install-dir as unused in both compile-only and link modes and emits clang: error: argument unused during compilation: '--gcc-install-dir=...' [-Werror,-Wunused-command-line-argument], which -Werror promotes to fatal. Probing the same command without -Werror links and runs, so the flag is inert decoration in this build.

Candidate 2, target triple only: compiles and links, but header search then relies on auto-detection. clang -v shows the winlibs sysroot found correctly plus an unwanted injection of C:/Users/lavantien/scoop/apps/gcc/current/include caused by the user environment exporting C_INCLUDE_PATH and CPLUS_INCLUDE_PATH at that foreign gcc. The Makefile clobbers both variables to empty for every recipe, which removes the injection under any spelling.

Candidate 3, the sysroot fallback: --sysroot=C:/Users/lavantien/scoop/apps/mingw-winlibs-ucrt/current on compile and link with -fuse-ld=lld. Verified working end to end (compile, link, run, exit 0). Not adopted because it duplicates what auto-detection already resolves deterministically once the environment is clean.

Candidate 4, adopted: compile with --target=x86_64-w64-mingw32, link with --target=x86_64-w64-mingw32 -fuse-ld=lld. clang -### shows the driver selecting C:/Program Files/LLVM/bin/ld.lld -m i386pep and resolving crt2.o, crtbegin.o, and the gcc 16.2.0 library directories from the winlibs install, which is exactly the pin candidate 1 intended.

The remaining ladder rungs (drop lld for the winlibs GNU ld, hybrid gcc-built libraylib.a) were never needed and stay unexercised.

## frozen makefile spelling

CROSS_COMPILE := --target=x86_64-w64-mingw32 on every -c line. CROSS_LINK := --target=x86_64-w64-mingw32 -fuse-ld=lld on every link line. export C_INCLUDE_PATH := and export CPLUS_INCLUDE_PATH := at the top guard the environment hazard. Warning flags stay off link lines, where they only caused the candidate 1 failure class.

## vendor findings

raylib 6.0: compile only the flat set third_party/raylib/src/*.c, which is 7 TUs (rcore.c rglfw.c rshapes.c rtextures.c rtext.c rmodels.c raudio.c). Everything under src/platforms/ is included by rcore.c and everything under src/external/glfw/src/ by rglfw.c, so a recursive glob over-collects and produces broken duplicate symbols. -DPLATFORM_DESKTOP still works, rcore.c maps it to the GLFW backend. Per-TU flags mirror raylib own src/Makefile: -DGRAPHICS_API_OPENGL_33 -D_GNU_SOURCE -DUNICODE -fno-strict-aliasing plus -Wno-missing-braces and -Wno-error, because the vendored audio codecs (stb_vorbis, qoaplay, jar_xm, jar_mod) are not clean under -Wall -Wextra -Werror -Wpedantic and upstream itself ships only -Wall. Include paths -Ithird_party/raylib/src -Ithird_party/raylib/src/external/glfw/include.

raygui 5.0 is header-only, src/ holds raygui.h with no raygui.c. The single TU is generated as build/raygui_impl.c containing RAYGUI_IMPLEMENTATION plus the include, compiled with -Ithird_party/raylib/src -Ithird_party/raygui/src and -Wno-error for unused parameter and unused function warnings in upstream code.

sqlite 3.53.4 amalgamation verified by python hashlib SHA3-256 over the downloaded zip against the published value 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e, which matched. The method line lands in build/vendor/sqlite.verify.log. The compile fallback asserts sqlite3_libversion() equals 3.53.4 through playground/spike_link/sqlite_version.c and stays wired for python-less machines.

Link set for anything touching raylib: -static-libgcc -lopengl32 -lgdi32 -lwinmm, matching raylib own static Windows line.

Windows bsdtar 3.8.8 rejects --one-top-level, so extraction pre-creates the target directory and uses -C dir --strip-components=1.

## runtime evidence

Command make spike after make clean. Exit 0. Stdout first line: sqlite 3.53.4. 573 lines total including raylib 6.0 INFO logs (GLFW backend, default shader, default font) and a clean window close after one rendered frame with DrawText and GuiLabel.

## current build status

make build and make test currently fail inside src/ code owned by other M0 tasks: src/app/app.h redefines the ct_ui_screen enumerators that src/config/config.h already defines, and src/app/app_loop.c references a ct_db_cfg member db_dir_name that does not exist yet. The harness itself is sound: all tests/ TUs compile under the sanitizer test flags and the failure is limited to those cross-file mismatches.
