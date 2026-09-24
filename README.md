# Pin

Breadboard circuit simulator for M5Stack Cardputer, built as a standalone ESP-IDF app image. Area512 installs it to its `ota_0` partition and starts it from there.

## Build

ESP-IDF 5.5. One build per board:

| Target | Board |
|---|---|
| `make CARDPUTER` | Cardputer (GPIO matrix keyboard) |
| `make CARDPUTER_ADV` | Cardputer ADV (TCA8418 keyboard) |
| `make CAPTFT_ST7789` | Cardputer ADV + Cap TFT ST7789 |
| `make CAPTFT_ILI9341` | Cardputer ADV + Cap TFT ILI9341 |

`make build` builds all four. The app image is `build/<TARGET>/pin.bin`. `make save-firmware` copies them to `firmware/`.

The factory partition is 1984 KB, the same size as Area512's `ota_0`, so the build fails if the app image does not fit.

## Run from Area512

Copy the `pin.bin` for your board to the SD card and open it in the Filer to install it. Press `L` in the Filer to start the installed app, `U` to erase it. In Pin, `q` then `y` quits and restarts into Area512. Turning the power off and on also returns to Area512.

## Controls

- `h` / `j` / `k` / `l`, `,` / `.` / `;` / `/` or Fn + arrows: move cursor
- `a` / Enter: add a part (choose a category, then a part)
- Enter / space while placing: set the next pin; lead parts take two pins, modules drop at the cursor
- `` ` `` (ESC) or `q` while placing: cancel
- space: press a tact switch / flip a slide switch
- `+` / `=` / `-`: change resistor value, volume knob or CdS light
- `x` / BS: remove the part under the cursor
- `c`: clear the board
- `?`: help
- `q`: quit to Area512
