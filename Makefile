BOARDS := CARDPUTER CARDPUTER_ADV TFT_ST7789 TFT_ILI9341

.PHONY: build wasm release save-firmware clean $(BOARDS)

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

clean:
	rm -rf build
