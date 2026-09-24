BOARDS := CARDPUTER CARDPUTER_ADV CAPTFT_ST7789 CAPTFT_ILI9341

.PHONY: build save-firmware clean $(BOARDS)

build: $(BOARDS)

$(BOARDS):
	idf.py -B build/$@ -DSDKCONFIG=build/$@/sdkconfig -DPIN_BOARD=$@ build

save-firmware:
	mkdir -p firmware
	cp build/CARDPUTER/pin.bin firmware/pin_cardputer.bin
	cp build/CARDPUTER_ADV/pin.bin firmware/pin_cardputer_adv.bin
	cp build/CAPTFT_ST7789/pin.bin firmware/pin_captft_st7789.bin
	cp build/CAPTFT_ILI9341/pin.bin firmware/pin_captft_ili9341.bin

clean:
	rm -rf build
