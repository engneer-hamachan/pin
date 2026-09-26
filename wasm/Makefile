# Run from the project root: make wasm

EMCC ?= emcc
EMXX ?= em++

BUILD_DIR := build/wasm
OBJ_DIR := $(BUILD_DIR)/obj
M5GFX_DIR := managed_components/m5stack__m5gfx/src
LGFX_DIR := $(M5GFX_DIR)/lgfx
PICORUBY_DIR := R2P2-ESP32/components/picoruby-esp32/picoruby
PICORUBY_BUILD_NAME := pin-wasm
PICORUBY_CONFIG := $(abspath build_config/$(PICORUBY_BUILD_NAME).rb)
LIBMRUBY := $(PICORUBY_DIR)/build/$(PICORUBY_BUILD_NAME)/lib/libmruby.a
PICORUBY_TI_DIR := picoruby-ti
PICORUBY_TI_SOURCE_DIRS := \
  base builtin context diagnostic eval eval/method_evaluator generated hover \
  suggest

APP_SOURCES := \
  $(filter-out main/storage.c,$(wildcard main/*.c)) \
  $(wildcard game/*.c) \
  $(shell find editor -name '*.c')

PICORUBY_SOURCES := \
  $(foreach dir,$(PICORUBY_TI_SOURCE_DIRS),$(wildcard $(PICORUBY_TI_DIR)/src/$(dir)/*.c)) \
  $(PICORUBY_DIR)/mrbgems/picoruby-machine/ports/common/machine.c \
  $(PICORUBY_DIR)/mrbgems/picoruby-io-console/ports/esp32/io-console.c \
  $(PICORUBY_DIR)/mrbgems/picoruby-env/ports/esp32/env.c \
  $(PICORUBY_DIR)/mrbgems/picoruby-require/ports/esp32/platform.c

WASM_SOURCES := $(wildcard wasm/*.c wasm/*.cpp)

LGFX_SOURCES := \
  $(LGFX_DIR)/v1/LGFXBase.cpp \
  $(LGFX_DIR)/v1/LGFX_Sprite.cpp \
  $(LGFX_DIR)/v1/lgfx_fonts.cpp \
  $(LGFX_DIR)/v1/panel/Panel_Device.cpp \
  $(LGFX_DIR)/v1/platforms/sdl/common.cpp \
  $(wildcard $(LGFX_DIR)/v1/misc/*.cpp) \
  $(wildcard $(LGFX_DIR)/utility/*.c) \
  $(wildcard $(LGFX_DIR)/Fonts/efont/*.c) \
  $(wildcard $(LGFX_DIR)/Fonts/IPA/*.c)

SOURCES := $(APP_SOURCES) $(PICORUBY_SOURCES) $(WASM_SOURCES) $(LGFX_SOURCES)
OBJECTS := $(SOURCES:%=$(OBJ_DIR)/%.o)

PICORUBY_INCLUDE_FLAGS := \
  -I$(PICORUBY_DIR)/include \
  -I$(PICORUBY_DIR)/build/$(PICORUBY_BUILD_NAME)/include \
  -I$(PICORUBY_DIR)/build/prism/include \
  -I$(PICORUBY_DIR)/mrbgems/picoruby-mruby/lib/mruby/include \
  -I$(PICORUBY_DIR)/mrbgems/picoruby-mruby/lib/mruby/mrbgems/mruby-task/include \
  -I$(PICORUBY_DIR)/mrbgems/mruby-compiler/include \
  -I$(PICORUBY_DIR)/mrbgems/mruby-compiler/lib/prism/include \
  -I$(PICORUBY_DIR)/mrbgems/picoruby-machine/include \
  -I$(PICORUBY_DIR)/mrbgems/picoruby-gpio/include \
  -I$(PICORUBY_TI_DIR)/include \
  $(foreach dir,$(PICORUBY_TI_SOURCE_DIRS),-I$(PICORUBY_TI_DIR)/src/$(dir))
PICORUBY_DEFINES := \
  -DMRB_TICK_UNIT=10 \
  -DMRB_TIMESLICE_TICK_COUNT=1 \
  -DMRB_UTF8_STRING \
  -DMRB_INT64 \
  -DMRB_NO_BOXING \
  -DMRB_32BIT \
  -DMRB_CONSTRAINED_BASELINE_PROFILE=1 \
  -DMRB_USE_TASK_SCHEDULER \
  -DPICORB_VM_MRUBY \
  -DPICORB_ALLOC_ESTALLOC \
  -DPICORB_ALLOC_ALIGN=8 \
  -DPRISM_XALLOCATOR \
  -DPRISM_BUILD_MINIMAL \
  -DMRC_TARGET_MRUBY \
  -DMRC_PRISM_ARENA \
  -DMRB_USE_DEBUG_HOOK

# LGFX picks its SDL platform when SDL headers are found; only its timing and
# GPIO stubs are linked, no SDL window is opened.
COMPILE_FLAGS := -O2 -sUSE_SDL=2 -DPIN_BOARD_WASM -Iwasm/include -Imain -Igame -Ieditor -I$(M5GFX_DIR) $(PICORUBY_INCLUDE_FLAGS) $(PICORUBY_DEFINES)
CFLAGS := $(COMPILE_FLAGS) -MMD -MP -std=gnu17
CXXFLAGS := $(COMPILE_FLAGS) -MMD -MP -std=gnu++17
LDFLAGS := \
  -O2 \
  --profiling-funcs \
  -sUSE_SDL=2 \
  -sASYNCIFY \
  -sALLOW_MEMORY_GROWTH \
  -sEXPORTED_RUNTIME_METHODS=HEAPU8

.PHONY: all

PWA_FILES := manifest.webmanifest sw.js icon-192.png icon-512.png apple-touch-icon.png

all: $(BUILD_DIR)/pin.js $(BUILD_DIR)/index.html $(addprefix $(BUILD_DIR)/,$(PWA_FILES))

$(BUILD_DIR)/pin.js: $(OBJECTS) $(LIBMRUBY)
	$(EMXX) $(LDFLAGS) $^ -o $@

$(LIBMRUBY): $(PICORUBY_CONFIG)
	cd $(PICORUBY_DIR) && MRUBY_CONFIG=$(PICORUBY_CONFIG) rake

$(OBJECTS): | $(LIBMRUBY)

$(BUILD_DIR)/index.html: wasm/index.html
	mkdir -p $(@D)
	cp $< $@

$(addprefix $(BUILD_DIR)/,$(PWA_FILES)): $(BUILD_DIR)/%: wasm/%
	mkdir -p $(@D)
	cp $< $@

$(OBJ_DIR)/%.c.o: %.c
	mkdir -p $(@D)
	$(EMCC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.cpp.o: %.cpp
	mkdir -p $(@D)
	$(EMXX) $(CXXFLAGS) -c $< -o $@

-include $(OBJECTS:.o=.d)
