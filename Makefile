CC := clang
TARGET_TRIPLE := x86_64-w64-mingw32
CROSS_COMPILE := --target=$(TARGET_TRIPLE)
CROSS_LINK := --target=$(TARGET_TRIPLE) -fuse-ld=lld
export C_INCLUDE_PATH :=
export CPLUS_INCLUDE_PATH :=
CFLAGS := -std=c23 -O2 -Wall -Wextra -Werror -Wpedantic
TEST_CFLAGS := $(CFLAGS) -g -fsanitize=undefined -fsanitize-trap=all
DEPFLAGS := -MMD -MP
WINLIBS_LDLIBS := -static-libgcc -lopengl32 -lgdi32 -lwinmm

RAYLIB_VERSION := 6.0
RAYGUI_VERSION := 5.0
SQLITE_VERSION := 3.53.4
SQLITE_CODE := 3530400
SQLITE_YEAR := 2026
SQLITE_SHA3 := 628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e
RAYLIB_SHA3 := 87fadf1148a547fda4a40dd9bcef059787085deb70515ef80a45839e27188cac
RAYGUI_SHA3 := 723b0b86b71a5df7163ee48fcdf4afd0e44abfcf547326f848161b6c5682d6b7
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
TEST_SRC := tests/test_main.c tests/framework.c $(wildcard tests/unit_*.c) $(wildcard tests/fuzz_*.c)
FMT_FILES := $(call rwildcard,src/,*.c) $(call rwildcard,src/,*.h) $(call rwildcard,tests/,*.c) $(call rwildcard,tests/,*.h)
RAYLIB_SRC := $(wildcard third_party/raylib/src/*.c)
RAYGUI_IMPL_SRC := $(BUILDDIR)/raygui_impl.c
SQLITE_SRC := third_party/sqlite/sqlite3.c

LIB_OBJ := $(addprefix $(OBJDIR)/,$(LIB_SRC:.c=.o))
LIB_TOBJ := $(addprefix $(TOBJDIR)/,$(LIB_SRC:.c=.o))
TEST_OBJ := $(addprefix $(TOBJDIR)/,$(TEST_SRC:.c=.o))
RENDER_OBJ := $(OBJDIR)/tools/render_scene.o
DBMAKE_OBJ := $(OBJDIR)/tools/make_db.o
RAYLIB_OBJ := $(addprefix $(OBJDIR)/,$(RAYLIB_SRC:.c=.o))
RAYGUI_OBJ := $(OBJDIR)/raygui_impl.o
SQLITE_OBJ := $(OBJDIR)/third_party/sqlite/sqlite3.o
DEPFILES := $(LIB_OBJ:.o=.d) $(LIB_TOBJ:.o=.d) $(TEST_OBJ:.o=.d) $(RAYLIB_OBJ:.o=.d) $(RAYGUI_OBJ:.o=.d)
DEPFILES += $(SQLITE_OBJ:.o=.d) $(RENDER_OBJ:.o=.d) $(DBMAKE_OBJ:.o=.d)
-include $(DEPFILES)

APP_BIN := $(BINDIR)/cubing-trainer.exe
TEST_BIN := $(BINDIR)/ct_tests.exe
SPIKE_BIN := $(BINDIR)/spike_link.exe
MUTATE_BIN := $(BINDIR)/mutate.exe
RENDER_BIN := $(BINDIR)/render_scene.exe
DBMAKE_BIN := $(BINDIR)/make_db.exe
MEMGUARD_SLEEPER_BIN := $(BINDIR)/memguard_sleeper.exe

RELEASE_DIR := $(BUILDDIR)/release
RELEASE_PKG := $(RELEASE_DIR)/cubing-trainer
RELEASE_EXE := $(RELEASE_PKG)/dist/bin/cubing-trainer.exe
RELEASE_NAME := cubing-trainer-$(shell git describe --tags --always)
RELEASE_ZIP := $(BUILDDIR)/$(RELEASE_NAME).zip
RELEASE_CASE := $(BUILDDIR)/release_smoke.txt

GOLDEN_DIR := tests/golden
E2E_DIR := tests/e2e
VISUAL_TMP := $(BUILDDIR)/visual
DBVERIFY_DIR := $(BUILDDIR)/dbverify

MEMGUARD_ARGS ?=

CT_FUZZ_LOOPS ?= 10
SCOPE ?= src/srs
SUITE ?=
CT_FUZZ_SUITES := fuzz_notation_random_tokens fuzz_notation_garbage fuzz_notation_random_algs
CT_FUZZ_SUITES += fuzz_transform_inverse fuzz_transform_simplify fuzz_transform_rotate_y
CT_FUZZ_SUITES += fuzz_cube_invariants fuzz_cube_mirror
CT_FUZZ_SUITES += fuzz_srs_walk fuzz_srs_elapsed fuzz_srs_grade_space
CT_FUZZ_SUITES += fuzz_db_card_strings fuzz_db_review_rows fuzz_db_settings_keys
CT_FUZZ_SUITES += fuzz_config_keys fuzz_config_moves

.PHONY: all build vendor vendor-verify-selftest release release-verify-selftest test diagrams memguard memguard-selftest run clean spike build-headless fmt fmt-check lint analyze fuzz mutate e2e visual visual-selftest db-verify

all: build

$(BINDIR) $(OBJDIR) $(TOBJDIR) $(DLDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CROSS_COMPILE) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TOBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CROSS_COMPILE) $(TEST_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJDIR)/src/%.o: CFLAGS := $(CFLAGS) $(SRC_INCLUDES)
$(OBJDIR)/third_party/raylib/%.o: CFLAGS := $(CFLAGS) $(RAYLIB_CFLAGS)
$(OBJDIR)/third_party/sqlite/%.o: CFLAGS := $(CFLAGS) $(SQLITE_CFLAGS)
$(OBJDIR)/playground/spike_link/%.o: CFLAGS := $(CFLAGS) $(SPIKE_CFLAGS)
$(TOBJDIR)/src/%.o: TEST_CFLAGS := $(TEST_CFLAGS) $(SRC_INCLUDES)
$(TOBJDIR)/tests/%.o: TEST_CFLAGS := $(TEST_CFLAGS) $(SRC_INCLUDES)
$(OBJDIR)/tools/%.o: CFLAGS := $(CFLAGS) $(SRC_INCLUDES)

$(RAYGUI_IMPL_SRC):
	@mkdir -p $(BUILDDIR)
	echo '#define RAYGUI_IMPLEMENTATION' > $@
	echo '#include "raygui.h"' >> $@

$(RAYGUI_OBJ): $(RAYGUI_IMPL_SRC)
	@mkdir -p $(@D)
	$(CC) $(CROSS_COMPILE) $(CFLAGS) $(DEPFLAGS) $(RAYGUI_CFLAGS) -c $< -o $@

build: $(APP_BIN)

$(APP_BIN): $(OBJDIR)/src/app/main.o $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) | $(BINDIR)
	$(CC) $(CROSS_LINK) -o $@ $(OBJDIR)/src/app/main.o $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) $(WINLIBS_LDLIBS)

build-headless: $(RENDER_BIN)

$(RENDER_BIN): $(RENDER_OBJ) $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) | $(BINDIR)
	$(CC) $(CROSS_LINK) -o $@ $(RENDER_OBJ) $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) $(WINLIBS_LDLIBS)

$(DBMAKE_BIN): $(DBMAKE_OBJ) $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) | $(BINDIR)
	$(CC) $(CROSS_LINK) -o $@ $(DBMAKE_OBJ) $(LIB_OBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ) $(WINLIBS_LDLIBS)

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
	python -I tools/vendor_verify.py $(DLDIR)/raylib.tar.gz $(RAYLIB_SHA3)
	python -I tools/vendor_verify.py $(DLDIR)/raygui.tar.gz $(RAYGUI_SHA3)
	python -I tools/vendor_verify.py $(DLDIR)/sqlite.zip $(SQLITE_SHA3)

vendor-verify-selftest:
	python -I tools/vendor_verify_selftest.py

$(DLDIR)/raylib.stamp: | $(DLDIR)
	curl.exe -fsIL -o NUL $(RAYLIB_URL)
	curl.exe -fsL -o $(DLDIR)/raylib.tar.gz $(RAYLIB_URL)
	python -I tools/vendor_verify.py $(DLDIR)/raylib.tar.gz $(RAYLIB_SHA3)
	mkdir -p third_party/raylib
	tar.exe -xzf $(DLDIR)/raylib.tar.gz -C third_party/raylib --strip-components=1
	echo $(RAYLIB_URL) > $@

$(DLDIR)/raygui.stamp: | $(DLDIR)
	curl.exe -fsIL -o NUL $(RAYGUI_URL)
	curl.exe -fsL -o $(DLDIR)/raygui.tar.gz $(RAYGUI_URL)
	python -I tools/vendor_verify.py $(DLDIR)/raygui.tar.gz $(RAYGUI_SHA3)
	mkdir -p third_party/raygui
	tar.exe -xzf $(DLDIR)/raygui.tar.gz -C third_party/raygui --strip-components=1
	echo $(RAYGUI_URL) > $@

$(DLDIR)/sqlite.stamp: | $(DLDIR)
	curl.exe -fsIL -o NUL $(SQLITE_URL)
	curl.exe -fsL -o $(DLDIR)/sqlite.zip $(SQLITE_URL)
	python -I tools/vendor_verify.py $(DLDIR)/sqlite.zip $(SQLITE_SHA3)
	mkdir -p third_party/sqlite
	rm -rf $(DLDIR)/sqlite_x
	python -I -m zipfile -e $(DLDIR)/sqlite.zip $(DLDIR)/sqlite_x
	mv $(DLDIR)/sqlite_x/sqlite-amalgamation-$(SQLITE_CODE)/* third_party/sqlite/
	rm -rf $(DLDIR)/sqlite_x
	echo $(SQLITE_URL) > $@

diagrams:
	node tools/gen_diagrams.mjs

memguard:
	pwsh.exe -NoProfile -File tools/memguard.ps1 $(MEMGUARD_ARGS)

memguard-selftest: $(MEMGUARD_SLEEPER_BIN)
	pwsh.exe -NoProfile -File tools/memguard.ps1 -SelfTest -SleeperPath $(MEMGUARD_SLEEPER_BIN)

$(MEMGUARD_SLEEPER_BIN): tools/memguard_sleeper.c | $(BINDIR)
	$(CC) $(CROSS_COMPILE) $(CFLAGS) tools/memguard_sleeper.c -o $@

run: build
	$(APP_BIN)

release: build
	rm -rf $(RELEASE_DIR)
	mkdir -p $(RELEASE_PKG)/dist/bin $(RELEASE_PKG)/db/seeds
	cp $(APP_BIN) $(RELEASE_EXE)
	cp -r db/migrations $(RELEASE_PKG)/db/migrations
	cp -r db/seeds/packs $(RELEASE_PKG)/db/seeds/packs
	rm -f $(RELEASE_ZIP)
	pwsh.exe -NoProfile -Command "Compress-Archive -Path '$(RELEASE_DIR)/*' -DestinationPath '$(RELEASE_ZIP)'"
	printf 'restart\n' > $(RELEASE_CASE)
	python -I tools/release_verify.py $(RELEASE_PKG) $(RELEASE_ZIP) $(RELEASE_CASE)

release-verify-selftest:
	python -I tools/release_verify_selftest.py

fmt:
	clang-format --style=file -i $(FMT_FILES)

fmt-check:
	clang-format --style=file --dry-run -Werror $(FMT_FILES)

lint:
	pwsh.exe -NoProfile -ExecutionPolicy Bypass -File tools/run_lint.ps1

analyze:
	pwsh.exe -NoProfile -ExecutionPolicy Bypass -File tools/run_analyze.ps1

fuzz: $(TEST_BIN)
	for suite in $(CT_FUZZ_SUITES); do CT_FUZZ_LOOPS=$(CT_FUZZ_LOOPS) $(TEST_BIN) $$suite || exit 1; done

fuzz-%: $(TEST_BIN)
	CT_FUZZ_LOOPS=$(CT_FUZZ_LOOPS) $(TEST_BIN) $*

$(MUTATE_BIN): tools/mutate.c | $(BINDIR)
	$(CC) $(CROSS_COMPILE) $(CFLAGS) tools/mutate.c -o $@

mutate: $(MUTATE_BIN) $(TEST_OBJ) $(LIB_TOBJ) $(RAYGUI_OBJ) $(RAYLIB_OBJ) $(SQLITE_OBJ)
	CT_MUTATE_CC="$(CC)" CT_MUTATE_CFLAGS="$(CROSS_COMPILE) $(TEST_CFLAGS) $(SRC_INCLUDES)" \
	CT_MUTATE_LDFLAGS="$(CROSS_LINK) -g" CT_MUTATE_LDLIBS="$(WINLIBS_LDLIBS)" \
	$(MUTATE_BIN) $(SCOPE) $(BUILDDIR) $(SUITE)

e2e: build
	@if [ -z "$$(ls $(E2E_DIR)/*.txt 2>/dev/null)" ]; then \
		echo "e2e: no script cases under $(E2E_DIR), script runner pending" >&2; \
		exit 1; \
	fi
	@for case in $(E2E_DIR)/*.txt; do \
		echo "e2e: $$case"; \
		$(APP_BIN) --headless --script "$$case" || exit 1; \
	done

visual-selftest:
	python -I tools/visual_selftest.py

visual: visual-selftest $(RENDER_BIN)
	rm -rf $(VISUAL_TMP) && mkdir -p $(VISUAL_TMP)
	python -I tools/run_visual.py $(RENDER_BIN) $(GOLDEN_DIR) $(VISUAL_TMP)

db-verify: $(DBMAKE_BIN)
	rm -rf $(DBVERIFY_DIR) && mkdir -p $(DBVERIFY_DIR)
	$(DBMAKE_BIN) $(DBVERIFY_DIR)/migrated.db
	cp $(DBVERIFY_DIR)/migrated.db $(DBVERIFY_DIR)/copy.db
	python -I tools/db_verify.py tools/db_schema_check.sql $(DBVERIFY_DIR)/copy.db db/migrations

clean:
	rm -rf $(BUILDDIR)
