CC := clang
TARGET_TRIPLE := x86_64-w64-mingw32
CROSS_COMPILE := --target=$(TARGET_TRIPLE)
CROSS_LINK := --target=$(TARGET_TRIPLE) -fuse-ld=lld
export C_INCLUDE_PATH :=
export CPLUS_INCLUDE_PATH :=
CFLAGS := -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic
TEST_CFLAGS := $(CFLAGS) -g -fsanitize=undefined -fsanitize-trap=all
WINLIBS_LDLIBS := -static-libgcc -lopengl32 -lgdi32 -lwinmm

RAYLIB_VERSION := 6.0
RAYGUI_VERSION := 5.0
SQLITE_VERSION := 3.53.4
SQLITE_CODE := 3530400
SQLITE_YEAR := 2026
SQLITE_SHA3 := 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e
RAYLIB_URL := https://github.com/raysan5/raylib/archive/refs/tags/$(RAYLIB_VERSION).tar.gz
RAYGUI_URL := https://github.com/raysan5/raygui/archive/refs/tags/$(RAYGUI_VERSION).tar.gz
SQLITE_URL := https://sqlite.org/$(SQLITE_YEAR)/sqlite-amalgamation-$(SQLITE_CODE).zip

BUILDDIR := build
BINDIR := $(BUILDDIR)/bin
OBJDIR := $(BUILDDIR)/obj
TOBJDIR := $(BUILDDIR)/tobj
DLDIR := $(BUILDDIR)/vendor

RAYLIB_CFLAGS := -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -D_GNU_SOURCE -DUNICODE -fno-strict-aliasing -Wno-missing-braces -Wno-error -Ithird_party/raylib/src -Ithird_party/raylib/src/external/glfw/include
RAYGUI_CFLAGS := -Ithird_party/raylib/src -Ithird_party/raygui/src -Wno-error
SQLITE_CFLAGS := -Ithird_party/sqlite -Wno-error
SRC_INCLUDES := -Isrc -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite
SPIKE_CFLAGS := -Ithird_party/raylib/src -Ithird_party/raygui/src -Ithird_party/sqlite

rwildcard = $(wildcard $1$2)$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

APP_MAIN := src/app/main.c
SRC := $(call rwildcard,src/,*.c)
LIB_SRC := $(filter-out $(APP_MAIN),$(SRC))
TEST_SRC := tests/test_main.c tests/framework.c $(wildcard tests/unit_*.c)
RAYLIB_SRC := $(wildcard third_party/raylib/src/*.c)
RAYGUI_IMPL_SRC := $(BUILDDIR)/raygui_impl.c
SQLITE_SRC := third_party/sqlite/sqlite3.c

LIB_OBJ := $(addprefix $(OBJDIR)/,$(LIB_SRC:.c=.o))
LIB_TOBJ := $(addprefix $(TOBJDIR)/,$(LIB_SRC:.c=.o))
TEST_OBJ := $(addprefix $(TOBJDIR)/,$(TEST_SRC:.c=.o))
RAYLIB_OBJ := $(addprefix $(OBJDIR)/,$(RAYLIB_SRC:.c=.o))
RAYGUI_OBJ := $(OBJDIR)/raygui_impl.o
SQLITE_OBJ := $(OBJDIR)/third_party/sqlite/sqlite3.o

APP_BIN := $(BINDIR)/cubing-trainer.exe
TEST_BIN := $(BINDIR)/ct_tests.exe
SPIKE_BIN := $(BINDIR)/spike_link.exe

MEMGUARD_ARGS ?=

.PHONY: all build vendor test diagrams memguard run clean spike build-headless fmt lint analyze fuzz mutate e2e visual db-verify

all: build

$(BINDIR) $(OBJDIR) $(TOBJDIR) $(DLDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CROSS_COMPILE) $(CFLAGS) -c $< -o $@

$(TOBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CROSS_COMPILE) $(TEST_CFLAGS) -c $< -o $@

$(OBJDIR)/src/%.o: CFLAGS := $(CFLAGS) $(SRC_INCLUDES)
$(OBJDIR)/third_party/raylib/%.o: CFLAGS := $(CFLAGS) $(RAYLIB_CFLAGS)
$(OBJDIR)/third_party/sqlite/%.o: CFLAGS := $(CFLAGS) $(SQLITE_CFLAGS)
$(OBJDIR)/playground/spike_link/%.o: CFLAGS := $(CFLAGS) $(SPIKE_CFLAGS)
$(TOBJDIR)/src/%.o: TEST_CFLAGS := $(TEST_CFLAGS) $(SRC_INCLUDES)
$(TOBJDIR)/tests/%.o: TEST_CFLAGS := $(TEST_CFLAGS) $(SRC_INCLUDES)

$(RAYGUI_IMPL_SRC):
	@mkdir -p $(BUILDDIR)
	echo '#define RAYGUI_IMPLEMENTATION' > $@
	echo '#include "raygui.h"' >> $@

$(RAYGUI_OBJ): $(RAYGUI_IMPL_SRC)
	@mkdir -p $(@D)
	$(CC) $(CROSS_COMPILE) $(CFLAGS) $(RAYGUI_CFLAGS) -c $< -o $@

build: $(APP_BIN)

$(APP_BIN): $(OBJDIR)/src/app/main.o $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) | $(BINDIR)
	$(CC) $(CROSS_LINK) -o $@ $(OBJDIR)/src/app/main.o $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) $(WINLIBS_LDLIBS)

build-headless:
	@echo not implemented && exit 1

spike: $(SPIKE_BIN)
	$(SPIKE_BIN)

$(SPIKE_BIN): $(OBJDIR)/playground/spike_link/main.o $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) | $(BINDIR)
	$(CC) $(CROSS_LINK) -o $@ $(OBJDIR)/playground/spike_link/main.o $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) $(WINLIBS_LDLIBS)

test: $(TEST_BIN)
	$(TEST_BIN)

test-%: $(TEST_BIN)
	$(TEST_BIN) $*

$(TEST_BIN): $(TEST_OBJ) $(LIB_TOBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) | $(BINDIR)
	$(CC) $(CROSS_LINK) -g -o $@ $(TEST_OBJ) $(LIB_TOBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) $(WINLIBS_LDLIBS)

vendor: $(DLDIR)/raylib.stamp $(DLDIR)/raygui.stamp $(DLDIR)/sqlite.stamp

$(DLDIR)/raylib.stamp: | $(DLDIR)
	curl.exe -fsIL -o NUL $(RAYLIB_URL)
	curl.exe -fsL -o $(DLDIR)/raylib.tar.gz $(RAYLIB_URL)
	mkdir -p third_party/raylib
	tar.exe -xzf $(DLDIR)/raylib.tar.gz -C third_party/raylib --strip-components=1
	echo $(RAYLIB_URL) > $@

$(DLDIR)/raygui.stamp: | $(DLDIR)
	curl.exe -fsIL -o NUL $(RAYGUI_URL)
	curl.exe -fsL -o $(DLDIR)/raygui.tar.gz $(RAYGUI_URL)
	mkdir -p third_party/raygui
	tar.exe -xzf $(DLDIR)/raygui.tar.gz -C third_party/raygui --strip-components=1
	echo $(RAYGUI_URL) > $@

$(DLDIR)/sqlite.stamp: | $(DLDIR)
	curl.exe -fsIL -o NUL $(SQLITE_URL)
	curl.exe -fsL -o $(DLDIR)/sqlite.zip $(SQLITE_URL)
	mkdir -p third_party/sqlite
	tar.exe -xf $(DLDIR)/sqlite.zip -C third_party/sqlite --strip-components=1
	if command -v python >/dev/null 2>&1; then python -I -c 'import hashlib,sys;h=hashlib.sha3_256(open(sys.argv[1],"rb").read()).hexdigest();print(h);sys.exit(0 if h=="$(SQLITE_SHA3)" else 1)' $(DLDIR)/sqlite.zip && echo sha3-256 verified > $(DLDIR)/sqlite.verify.log; else $(CC) $(CROSS_LINK) $(CFLAGS) -Ithird_party/sqlite playground/spike_link/sqlite_version.c $(SQLITE_SRC) -o $(DLDIR)/sqlite_verify.exe && test "$$($(DLDIR)/sqlite_verify.exe)" = "$(SQLITE_VERSION)" && echo version compile verified > $(DLDIR)/sqlite.verify.log; fi
	echo $(SQLITE_URL) > $@

diagrams:
	node tools/gen_diagrams.mjs

memguard:
	pwsh.exe -NoProfile -File tools/memguard.ps1 $(MEMGUARD_ARGS)

run: build
	$(APP_BIN)

fmt:
	@echo not implemented && exit 1

lint:
	@echo not implemented && exit 1

analyze:
	@echo not implemented && exit 1

fuzz:
	@echo not implemented && exit 1

mutate:
	@echo not implemented && exit 1

e2e:
	@echo not implemented && exit 1

visual:
	@echo not implemented && exit 1

db-verify:
	@echo not implemented && exit 1

clean:
	rm -rf $(BUILDDIR)
