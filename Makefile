PLATFORM ?= ps2
VERSION ?= anniversary

# The default goal of make is the first target of the first makefile it reads.
# The dependency files of the objects are included below, and the first one of
# them would become the default goal: "make" would then build a single object
# file instead of the program. The goal is therefore set explicitly.
.DEFAULT_GOAL := build

# Third Strike Upper (3s') balance patch.
# 0 = vanilla build (byte-matches the retail THIRD_U.BIN on the ps2 platform)
# 1 = 3s' build (balance/bugfix changes enabled, no longer byte-matching)
UPPER_3S ?= 0

# Dumps the character data of the disc to upper3s_dump.txt. Only useful with
# the disc image in rom/THIRD, and only for the tools that derive the
# balance changes from the retail values.
UPPER3S_DUMP ?= 0

ifeq ($(UPPER_3S),1)
UPPER_3S_DEFINE := -DUPPER_3S
else
UPPER_3S_DEFINE :=
endif

ifeq ($(UPPER3S_DUMP),1)
UPPER_3S_DEFINE += -DUPPER3S_DUMP
endif

# Binaries

ifeq ($(PLATFORM),ps2)
	MAIN := THIRD_U.BIN
else ifeq ($(PLATFORM),windows)
	MAIN := sf33rd.exe
else ifeq ($(PLATFORM),web)
	MAIN := sf33rd.js
else
	MAIN := sf33rd
endif

# Export so that Python could see those
export PLATFORM
export VERSION
export MAIN

# Directories

BIN_DIR := bin
BUILD_DIR := build/$(VERSION)/$(PLATFORM)
SRC_DIR := src/$(VERSION)
ASM_DIR := asm/$(VERSION)
INCLUDE_DIR := include
ASSETS_DIR := assets/$(VERSION)
CONFIG_DIR := config/$(VERSION)
TOOLS_DIR := tools
SDL3_WASM_DIR ?= third_party/sdl3-wasm
EEGCC_PATHS := PATH=$(BIN_DIR)/ee/bin:$(BIN_DIR)/lib/gcc-lib/ee/2.96-ee-001003-1

# Tools

WIBO := $(BIN_DIR)/wibo
MWCCPS2 := $(BIN_DIR)/mwccps2.exe
MWLDPS2 := MWCIncludes=$(BIN_DIR) $(WIBO) $(BIN_DIR)/mwldps2.exe

EEGCC := $(BIN_DIR)/ee/bin/gcc

CCPS2 := MWCIncludes=$(BIN_DIR) $(WIBO) $(MWCCPS2)

CC := $(CCPS2)
AS := mipsel-linux-gnu-as
LD := $(MWLDPS2)

PYTHON := python3
SPLICE_ELF := $(PYTHON) $(TOOLS_DIR)/splice_elf.py
COMPARE_BYTES := $(PYTHON) $(TOOLS_DIR)/compare_bytes.py
PATCH_ALIGNMENT := $(PYTHON) $(TOOLS_DIR)/patch_alignment.py
GENERATE_LCF := $(PYTHON) -m tools.lcf.generate_lcf

# Flags

COMMON_INCLUDES := -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/sdk -I$(INCLUDE_DIR)/cri -I$(INCLUDE_DIR)/cri/ee -Izlib
PS2_INCLUDES := $(COMMON_INCLUDES) -I$(INCLUDE_DIR)/gcc
PS2_DEFINES := -DTARGET_PS2 -DDEBUG $(UPPER_3S_DEFINE)
MWCCPS2_FLAGS := -gccinc $(PS2_INCLUDES) -O0,p -c -lang c -str readonly -fl divbyzerocheck -sdatathreshold 128 $(PS2_DEFINES)
EEGCC_FLAGS := $(PS2_INCLUDES) -O2 -G0 -c $(PS2_DEFINES) -DXPT_TGT_EE

AS_FLAGS += -EL -I $(INCLUDE_DIR) -G 128 -march=r5900 -mabi=eabi -no-pad-sections
LD_FLAGS := -main func_00100008 -map

# Host (native port) and web build flags

CLANG_WARNINGS := -Wall -Werror
CLANG_WARNINGS += -Wno-macro-redefined -Wno-deprecated-non-prototype
CLANG_WARNINGS += -Wno-missing-braces -Wno-self-assign -Wno-format
CLANG_WARNINGS += -Wno-tautological-constant-out-of-range-compare -Wno-tautological-overlap-compare
CLANG_WARNINGS += -Wno-gcc-compat -Wno-unknown-pragmas -Wno-c2x-extensions
CLANG_WARNINGS += -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-comparison -Wno-unused-function

# These should be enabled in the future
CLANG_WARNINGS += -Wno-int-to-pointer-cast
CLANG_WARNINGS += -Wno-pointer-to-int-cast
CLANG_WARNINGS += -Wno-int-conversion
CLANG_WARNINGS += -Wno-incompatible-pointer-types
CLANG_WARNINGS += -Wno-incompatible-function-pointer-types
CLANG_WARNINGS += -Wno-pointer-sign
CLANG_WARNINGS += -Wno-shift-count-overflow
# The decompilation uses 255 in s8 tables to mean -1.
CLANG_WARNINGS += -Wno-constant-conversion

CLANG_DEFINES := -DTARGET_SDL3 -DSOUND_DISABLED -DXPT_TGT_EE -D_POSIX_C_SOURCE -DDEBUG $(UPPER_3S_DEFINE)
CLANG_INCLUDES := $(COMMON_INCLUDES) -Ilibco
CLANG_FLAGS := $(CLANG_INCLUDES) $(CLANG_WARNINGS) $(CLANG_DEFINES) -std=gnu11 -O0
# The signatures of a few functions do not match the ones of their callers,
# which is an error in WebAssembly. See port/port_fixups.h.
CLANG_FLAGS += -include port/port_fixups.h

CLANG_LINKER_FLAGS := -lm -g

# The port builds track the headers they depend on, so that a change of a
# header rebuilds the sources that include it.
CLANG_FLAGS += -MMD -MP

PORT_PACKAGES := sdl3 libavcodec libavutil libavformat libswresample
CLANG_FLAGS += $(shell pkg-config --cflags $(PORT_PACKAGES))
CLANG_LINKER_FLAGS += $(shell pkg-config --libs $(PORT_PACKAGES))

# Web (emscripten) flags

EMCC := emcc
EM_DEFINES := -DTARGET_SDL3 -DTARGET_WEB -DSOUND_DISABLED -DXPT_TGT_EE -DDEBUG $(UPPER_3S_DEFINE)

# The browser build blocks the main thread, so the page cannot take a
# screenshot of the canvas on its own. With this flag the build takes the
# screenshot itself and sends it to the dev server, which lets the
# verification script look at a frame of the running game.
WEB_SHOT ?= 0
ifeq ($(WEB_SHOT),1)
EM_DEFINES += -DWEB_AUTO_SHOT -DWEB_AUTO_SHOT_FRAME=600
endif
EM_FLAGS := $(CLANG_INCLUDES) $(CLANG_WARNINGS) $(EM_DEFINES) -std=gnu11 -O2
EM_FLAGS += -I$(SDL3_WASM_DIR)/include
# The signatures of a few functions do not match the ones of their callers,
# which is an error in WebAssembly. See port/port_fixups.h.
EM_FLAGS += -include port/port_fixups.h
EM_FLAGS += -MMD -MP

# ASYNCIFY lets the blocking game loop yield to the browser event loop, so
# input keeps flowing while the game is running.
EM_LINKER_FLAGS := -lm
EM_LINKER_FLAGS += -sASYNCIFY=1
# The game loop unwinds the whole call stack of a frame when it lets the
# browser breathe, so the stack that holds the unwound frames has to be big
# enough for the deepest frame of the game.
EM_LINKER_FLAGS += -sASYNCIFY_STACK_SIZE=262144
EM_LINKER_FLAGS += -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=2gb
EM_LINKER_FLAGS += -sINITIAL_MEMORY=268435456
EM_LINKER_FLAGS += -sSTACK_SIZE=1048576
EM_LINKER_FLAGS += -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,FS,HEAPU8,UTF8ToString
EM_LINKER_FLAGS += -sASSERTIONS=0
# The symbol map lists the wasm functions with their indices, which is the
# only way to turn the indices of a wasm stack trace into source names.
EM_LINKER_FLAGS += --emit-symbol-map
EM_LINKER_FLAGS += -I$(SDL3_WASM_DIR)/include -L$(SDL3_WASM_DIR)/lib -lSDL3

# The loose disc files are small and are packaged into the module. The AFS
# archive is hundreds of megabytes and is streamed over HTTP instead.
WEB_PRELOAD := $(if $(wildcard rom/THIRD/IOP),--preload-file rom/THIRD/IOP@rom/THIRD/IOP)
EM_LINKER_FLAGS += $(WEB_PRELOAD)

# Files

MAIN_TARGET := $(BUILD_DIR)/$(MAIN)

S_FILES := $(shell find $(ASM_DIR) -name '*.s' -not -path *nonmatchings* 2>/dev/null)
GAME_C_FILES := $(shell find $(SRC_DIR)/sf33rd -name '*.c' -not -path '*/Compress/zlib/*' 2>/dev/null)
# The Third Strike Upper layer lives in its own directory so that the vanilla
# build keeps exactly the same object files - and therefore the same bytes -
# as the retail THIRD_U.BIN.
UPPER3S_DIR := $(SRC_DIR)/sf33rd/Source/Game/upper3s
UPPER3S_C_FILES := $(shell find $(UPPER3S_DIR) -name '*.c' 2>/dev/null)
ifneq ($(UPPER_3S),1)
GAME_C_FILES := $(filter-out $(UPPER3S_C_FILES),$(GAME_C_FILES))
endif
ZLIB_C_FILES := $(shell find $(SRC_DIR)/sf33rd/Source/Compress/zlib -name '*.c' 2>/dev/null)
CRI_C_FILES := $(shell find $(SRC_DIR)/cri -name '*.c' 2>/dev/null)
BIN2OBJ_C_FILES := $(shell find $(SRC_DIR)/bin2obj -name '*.c' 2>/dev/null)
PORT_C_FILES := $(shell find $(SRC_DIR)/port -name '*.c' 2>/dev/null)
ifeq ($(PLATFORM),web)
# The ADX music player needs ffmpeg; the web build uses a stub instead.
PORT_C_FILES := $(filter-out $(SRC_DIR)/port/sdl/sdl_adx_sound.c,$(PORT_C_FILES))
else
# The web backends need the emscripten headers.
PORT_C_FILES := $(filter-out $(SRC_DIR)/port/web/%,$(PORT_C_FILES))
endif
LIBCO_C_FILES := libco/libco.c
ifeq ($(PLATFORM),web)
# libco switches the stack with setjmp/longjmp, which does not survive
# ASYNCIFY. The browser build uses the fiber API of emscripten instead, which
# provides the same functions (see port/web/web_co.c).
LIBCO_C_FILES :=
endif

ASM_O_FILES := $(patsubst %.s,%.s.o,$(S_FILES))
ASM_O_FILES := $(addprefix $(BUILD_DIR)/,$(ASM_O_FILES))
GAME_O_FILES := $(patsubst %.c,%.c.o,$(GAME_C_FILES))
GAME_O_FILES := $(addprefix $(BUILD_DIR)/,$(GAME_O_FILES))
ZLIB_O_FILES := $(patsubst %.c,%.c.o,$(ZLIB_C_FILES))
ZLIB_O_FILES := $(addprefix $(BUILD_DIR)/,$(ZLIB_O_FILES))
CRI_O_FILES := $(patsubst %.c,%.c.o,$(CRI_C_FILES))
CRI_O_FILES := $(addprefix $(BUILD_DIR)/,$(CRI_O_FILES))
BIN2OBJ_O_FILES := $(patsubst %.c,%.c.o,$(BIN2OBJ_C_FILES))
BIN2OBJ_O_FILES := $(addprefix $(BUILD_DIR)/,$(BIN2OBJ_O_FILES))
PORT_O_FILES := $(patsubst %.c,%.c.o,$(PORT_C_FILES))
PORT_O_FILES := $(addprefix $(BUILD_DIR)/,$(PORT_O_FILES))
LIBCO_O_FILES := $(patsubst %.c,%.c.o,$(LIBCO_C_FILES))
LIBCO_O_FILES := $(addprefix $(BUILD_DIR)/,$(LIBCO_O_FILES))

ifeq ($(PLATFORM),ps2)
	ALL_O_FILES := $(GAME_O_FILES) $(ZLIB_O_FILES) $(CRI_O_FILES) $(BIN2OBJ_O_FILES) $(ASM_O_FILES)
	EEGCC_O_FILES := $(CRI_O_FILES) $(BIN2OBJ_O_FILES)
else
	ALL_O_FILES := $(GAME_O_FILES) $(ZLIB_O_FILES) $(CRI_O_FILES) $(BIN2OBJ_O_FILES) $(PORT_O_FILES) $(LIBCO_O_FILES)
	EEGCC_O_FILES := $(CRI_O_FILES) $(BIN2OBJ_O_FILES)
endif

LINKER_SCRIPT := $(BUILD_DIR)/$(MAIN).lcf

# The dependency files of the port builds.
-include $(ALL_O_FILES:.o=.d)

COMPILER_TAR := mwcps2-3.0b38-030307.tar.gz
EE_COMPILER_TAR := ee-gcc2.96.tar.xz

build: $(MAIN_TARGET)

# Etc. rules

split:
	@mkdir -p $(dir $(LINKER_SCRIPT))
	$(GENERATE_LCF) $(LINKER_SCRIPT)

split_cps3:
	cargo build --release --manifest-path ./tools/saturn-splitter/rust-dis/Cargo.toml
	python3 ./tools/combine-and-decrypt.py ./sfiii3n/ ./decrypted.bin
	cargo run --release --manifest-path ./tools/saturn-splitter/rust-dis/Cargo.toml ./config/cps3/sfiii3n.yaml

clean: ##@ clean extracted files, assets, and build artifacts
	git clean -fdx $(ASSETS_DIR)/
	git clean -fdx $(ASM_DIR)/
	git clean -fdx build/
	git clean -fdx .splache

# Build

ifeq ($(PLATFORM),ps2)

$(MAIN_TARGET): $(ALL_O_FILES) $(LINKER_SCRIPT)
	@$(LD) $(LD_FLAGS) -o $@ \
		$(LINKER_SCRIPT) \
		$(shell find $(BUILD_DIR) -name '*.o')

	$(COMPARE_BYTES) $(MAIN) $(MAIN_TARGET) 0x80 0x478A00

$(BUILD_DIR)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(AS_FLAGS) -o $@ $<
	$(PATCH_ALIGNMENT) $@

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	python3 tools/mwccgap/mwccgap.py $< $@ --mwcc-path $(MWCCPS2) --macro-inc-path $(INCLUDE_DIR)/macro.inc --use-wibo --wibo-path $(WIBO) --as-march r5900 --as-mabi eabi --target-encoding shift_jis $(MWCCPS2_FLAGS)
	$(PATCH_ALIGNMENT) $@

$(CRI_O_FILES): $(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(EEGCC_PATHS) $(EEGCC) $< -o $@ $(EEGCC_FLAGS)

else ifeq ($(PLATFORM),web)

$(MAIN_TARGET): $(ALL_O_FILES)
	@mkdir -p $(dir $@)
	$(EMCC) $(ALL_O_FILES) $(EM_LINKER_FLAGS) -o $@
	$(PYTHON) $(TOOLS_DIR)/web/make_web_shell.py $@ $(BUILD_DIR)/index.html

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(EMCC) -c $< -o $@ $(EM_FLAGS)

else

$(MAIN_TARGET): $(ALL_O_FILES)
	clang $(ALL_O_FILES) $(CLANG_LINKER_FLAGS) -o $@

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	clang -g -c $< -o $@ $(CLANG_FLAGS)

endif

# Tools

setup_tools: $(MWCCPS2) $(WIBO) $(EEGCC)

# Prints the C sources that go into the retail PS2 build.
print-ps2-sources:
	@echo $(GAME_C_FILES) $(ZLIB_C_FILES) $(CRI_C_FILES) $(BIN2OBJ_C_FILES)

# Prints the object files of the build, one per line.
print-o-files:
	@echo $(ALL_O_FILES) | tr ' ' '\n'

.PHONY: print-ps2-sources print-o-files

# Third Strike Upper tests
#
# The tests link the 3s' layer directly, without the rest of the game, so
# that the data override machinery can be exercised on a synthetic
# character. See tests/upper3s/README.md.

TEST_DIR := $(BUILD_DIR)/tests
UPPER3S_TEST_SRC := tests/upper3s/test_upper3s.c $(UPPER3S_C_FILES)
UPPER3S_TEST_FLAGS := $(COMMON_INCLUDES) -DTARGET_SDL3 -DSOUND_DISABLED -DXPT_TGT_EE -D_POSIX_C_SOURCE -DDEBUG -DUPPER_3S -std=gnu11 -O0 -g -Wno-macro-redefined

# Every test file that has a main() of its own is a program of its own, so
# that a test of one mechanic does not have to link against the tests of the
# others.
UPPER3S_TEST_PROGRAMS := $(patsubst tests/upper3s/%.c,$(TEST_DIR)/%,$(wildcard tests/upper3s/test_upper3s_*.c))

# The program that reads the dump of the disc is a tool and not a test: the
# dump is not part of the repository, so a run without it checks nothing.
# It is run on its own when the dump is there, and the check fails when it
# does not agree with the offline decoder.
UPPER3S_DUMP_PROGRAM := $(TEST_DIR)/test_upper3s_chain_dump
UPPER3S_TEST_RUNS := $(filter-out $(UPPER3S_DUMP_PROGRAM),$(UPPER3S_TEST_PROGRAMS))

test: $(TEST_DIR)/upper3s_test $(UPPER3S_TEST_PROGRAMS)
	$(TEST_DIR)/upper3s_test
	@for program in $(UPPER3S_TEST_RUNS); do \
		echo "== $$program"; \
		$$program || exit 1; \
	done
	@if [ -f upper3s_dump.txt ]; then \
		echo "== the chain dump of the disc"; \
		$(UPPER3S_DUMP_PROGRAM) > $(BUILD_DIR)/chain_startup.txt || exit 1; \
		echo "== the comparison of the walk of the game with the offline decoder"; \
		python3 tools/upper3s/compare_startup.py upper3s_dump.txt $(BUILD_DIR)/chain_startup.txt || exit 1; \
	else \
		echo "== the chain dump of the disc is skipped: the dump is not there"; \
	fi

$(TEST_DIR)/upper3s_test: $(UPPER3S_TEST_SRC)
	@mkdir -p $(dir $@)
	clang $(UPPER3S_TEST_FLAGS) $(UPPER3S_TEST_SRC) -o $@ -lm

$(TEST_DIR)/test_upper3s_%: tests/upper3s/test_upper3s_%.c $(UPPER3S_C_FILES)
	@mkdir -p $(dir $@)
	clang $(UPPER3S_TEST_FLAGS) $< $(UPPER3S_C_FILES) -o $@ -lm

.PHONY: test

# Build SDL3 for the web target. Emscripten has no SDL3 port of its own, so
# SDL3 is compiled from source with its Emscripten backend.
SDL3_VERSION := release-3.4.2

setup_sdl3_wasm:
	git clone --depth 1 -b $(SDL3_VERSION) https://github.com/libsdl-org/SDL.git $(BUILD_DIR)/sdl3
	mkdir -p $(BUILD_DIR)/sdl3-build
	cd $(BUILD_DIR)/sdl3-build && emcmake cmake ../sdl3 \
		-DCMAKE_BUILD_TYPE=Release \
		-DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_EXAMPLES=OFF -DSDL_TESTS=OFF \
		-DCMAKE_INSTALL_PREFIX=$(abspath $(SDL3_WASM_DIR))
	cd $(BUILD_DIR)/sdl3-build && cmake --build . --target install

$(WIBO):
	@mkdir -p $(BIN_DIR)
	wget -O $@ https://github.com/decompals/wibo/releases/download/0.6.13/wibo
	chmod +x $@

$(MWCCPS2):
	@mkdir -p $(BIN_DIR)
	wget -O- https://github.com/decompme/compilers/releases/download/compilers/$(COMPILER_TAR) | tar xzv -C $(BIN_DIR)

$(EEGCC):
	@mkdir -p $(BIN_DIR)
	wget -O- https://github.com/decompme/compilers/releases/download/compilers/$(EE_COMPILER_TAR) | tar xJv -C $(BIN_DIR)
