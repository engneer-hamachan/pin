BOARDS := CARDPUTER CARDPUTER_ADV TFT_ST7789 TFT_ILI9341
PICORUBY_BUILD_DIR := R2P2-ESP32/components/picoruby-esp32/picoruby/build

.PHONY: build wasm release save-firmware gendb clean $(BOARDS)

build: $(BOARDS)

$(BOARDS):
	idf.py -B build/$@ -DSDKCONFIG=build/$@/sdkconfig -DPIN_BOARD=$@ build

wasm:
	$(MAKE) -f wasm/Makefile

release: wasm
	mkdir -p docs
	cp build/wasm/index.html build/wasm/pin.js build/wasm/pin.wasm docs/

save-firmware:
	mkdir -p firmware
	cp build/CARDPUTER/pin.bin firmware/pin_cardputer.bin
	cp build/CARDPUTER_ADV/pin.bin firmware/pin_cardputer_adv.bin
	cp build/TFT_ST7789/pin.bin firmware/pin_tft_st7789.bin
	cp build/TFT_ILI9341/pin.bin firmware/pin_tft_ili9341.bin

gendb:
	ruby ./picoruby-ti/tidbgen/main.rb --sig-dir ./sig --out ./picoruby-ti/src/generated

clean:
	rm -rf build
	rm -rf $(PICORUBY_BUILD_DIR)/pin-esp32 $(PICORUBY_BUILD_DIR)/pin-wasm
