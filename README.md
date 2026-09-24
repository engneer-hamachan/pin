# Pin

Breadboard circuit simulator for M5Stack Cardputer, built as a standalone ESP-IDF app image. Area512 installs it to its `ota_0` partition and starts it from there.

## Build

ESP-IDF 5.5. One build per board:

| Target | Board |
|---|---|
| `make CARDPUTER` | Cardputer (GPIO matrix keyboard) |
| `make CARDPUTER_ADV` | Cardputer ADV (TCA8418 keyboard) |
| `make TFT_ST7789` | Cardputer ADV + TFT ST7789 |
| `make TFT_ILI9341` | Cardputer ADV + TFT ILI9341 |

`make build` builds all four. The app image is `build/<TARGET>/pin.bin`. `make save-firmware` copies them to `firmware/`.

The factory partition is 1984 KB, the same size as Area512's `ota_0`, so the build fails if the app image does not fit.

## Browser build

`make wasm` builds Pin with Emscripten (`emcc` on `PATH`) into `build/wasm/`. Serve that directory over HTTP and open `index.html`. The first build downloads Emscripten's SDL2 port. The files in `wasm/` replace the display, keyboard, SD card and ESP-IDF calls. `s` / `o` (save / load) do nothing, and no hiscore is shown or saved.

`make release` runs `make wasm` and copies `index.html`, `pin.js` and `pin.wasm` to `docs/` for GitHub Pages.

## Run from Area512

Copy the `pin.bin` for your board to the SD card and open it in the Filer to install it. Press `L` in the Filer to start the installed app, `U` to erase it. In Pin, `q` then `y` quits and restarts into Area512. Turning the power off and on also returns to Area512.

## Title

At start Pin shows a title screen. `j` / `k` or arrows choose, Enter / space selects:

- START: play the game
- HOW TO PLAY: show the game rules. `j` / `k` scroll a line, `h` / `l` a page, any other key goes back
- SIMULATOR: open the breadboard simulator

`q` then `y` on the title quits and restarts into Area512.

## Game

The board starts with a 5V battery on the top-left rails. A random part is dealt from the add menu (no battery, red LED only; jumper wires come 6 times as often as any other part). Place it the same way as in the simulator, then the next part comes. Only jumper wires may go on the rails.

When an LED, RGB LED or 7-segment lights, a buzzer sounds or a motor turns, every part carrying current (except batteries) and the wires on that path are removed, 1 point each.

The game is over when a part burns out, the battery is shorted, the dealt part fits nowhere, or more than 15 parts (not counting the battery) are on the board. A score higher than the record is saved to `Pin_data/hiscore.txt`.

- hjkl / arrows: move cursor
- Enter: set the next pin
- space: press a tact switch / flip a slide switch
- `+` / `=` / `-`: change resistor value, volume knob or CdS light
- `` ` `` (ESC): redo the first pin
- `p`: pause; while paused the header shows the part under the cursor and no part can be placed
- `q`: quit to the title

## Controls

- `h` / `j` / `k` / `l`, `,` / `.` / `;` / `/` or Fn + arrows: move cursor
- `a` / Enter: add a part (choose a category, then a part)
- Enter / space while placing: set the next pin; lead parts take two pins, modules drop at the cursor
- `` ` `` (ESC) or `q` while placing: cancel
- space: press a tact switch / flip a slide switch
- `+` / `=` / `-`: change resistor value, volume knob or CdS light
- `x` / BS: remove the part under the cursor
- `c`: clear the board
- `s`: save the board to one of `Pin_data/pin.txt`, `pin2.txt`, `pin3.txt`, `pin4.txt` on the SD card
- `o`: load the board from one of those files
- `?`: help
- `q`: quit to Area512
