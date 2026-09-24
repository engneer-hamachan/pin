# Run from the project root: make wasm

EMCC ?= emcc
EMXX ?= em++

BUILD_DIR := build/wasm
OBJ_DIR := $(BUILD_DIR)/obj
M5GFX_DIR := managed_components/m5stack__m5gfx/src
LGFX_DIR := $(M5GFX_DIR)/lgfx

APP_SOURCES := \
  $(filter-out main/storage.c,$(wildcard main/*.c)) \
  $(wildcard game/*.c)

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

SOURCES := $(APP_SOURCES) $(WASM_SOURCES) $(LGFX_SOURCES)
OBJECTS := $(SOURCES:%=$(OBJ_DIR)/%.o)

# LGFX picks its SDL platform when SDL headers are found; only its timing and
# GPIO stubs are linked, no SDL window is opened.
COMPILE_FLAGS := -O2 -sUSE_SDL=2 -DPIN_BOARD_WASM -Iwasm/include -Imain -Igame -I$(M5GFX_DIR)
CFLAGS := $(COMPILE_FLAGS) -std=gnu17
CXXFLAGS := $(COMPILE_FLAGS) -std=gnu++17
LDFLAGS := \
  -O2 \
  -sUSE_SDL=2 \
  -sASYNCIFY \
  -sALLOW_MEMORY_GROWTH \
  -sEXPORTED_RUNTIME_METHODS=HEAPU8

.PHONY: all

all: $(BUILD_DIR)/pin.js $(BUILD_DIR)/index.html

$(BUILD_DIR)/pin.js: $(OBJECTS)
	$(EMXX) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/index.html: wasm/index.html
	mkdir -p $(@D)
	cp $< $@

$(OBJ_DIR)/%.c.o: %.c
	mkdir -p $(@D)
	$(EMCC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.cpp.o: %.cpp
	mkdir -p $(@D)
	$(EMXX) $(CXXFLAGS) -c $< -o $@
