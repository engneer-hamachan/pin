# Pin

Breadboard circuit simulator for M5Stack Cardputer, built as a standalone ESP-IDF app image. Area512 installs it to its `ota_0` partition and starts it from there.

## Build

ESP-IDF 5.5, Ruby and rake. `R2P2-ESP32` (cloned with `--recursive`) and `picoruby-ti` must be in the project root. Run `make gendb` once before the first build; it writes the type database for the editor from `sig/*.rbs` into `picoruby-ti/src/generated/`. The first build of each target also builds PicoRuby (mruby VM) with `build_config/pin-esp32.rb` or `build_config/pin-wasm.rb`.

One build per board:

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

- SIMULATOR: open the breadboard simulator
- GAME: play the game
- HOW TO PLAY: show the game rules. `j` / `k` scroll a line, `h` / `l` a page, any other key goes back

`q` then `y` on the title quits and restarts into Area512.

## Game

The board starts with a 5V battery on the top-left rails. A random part is dealt from the add menu (no battery, red LED only; jumper wires come 6 times as often as any other part). Place it the same way as in the simulator, then the next part comes. Only jumper wires may go on the rails.

When an LED, RGB LED or 7-segment lights, a buzzer sounds or a motor turns, every part carrying current (except batteries) and the wires on that path are removed, 1 point each.

The game is over when a part burns out, the battery is shorted, the dealt part fits nowhere, or more than 15 parts (not counting the battery) are on the board. A score higher than the record is saved to `Pin_data/hiscore.txt`.

- hjkl / arrows: move cursor
- Enter: set the next pin
- space: press a tact switch / flip a slide switch
- `+` / `=` / `-`: change resistor value, volume knob or CdS light
- `` ` `` (ESC): redo the first pin; before the first pin is set, put the part down. While it is down the header shows the part under the cursor, as in the simulator
- Enter while the part is down: pick it up again
- `i` while the part is down: show what the part under the cursor is, its pins and keys. `j` / `k` scroll a line, `h` / `l` a page, any other key goes back
- `?` while the part is down: help
- `q`: quit to the title

## Controls

- `h` / `j` / `k` / `l`, `,` / `.` / `;` / `/` or Fn + arrows: move cursor
- `a` / Enter: add a part (choose a category, then a part)
- Enter / space while placing: set the next pin; lead parts take two pins, modules drop at the cursor
- `` ` `` (ESC) or `q` while placing: cancel
- space: press a tact switch / flip a slide switch
- `+` / `=` / `-`: change resistor value, volume knob or CdS light
- `i`: show what the part under the cursor is, its pins and keys. `j` / `k` scroll a line, `h` / `l` a page, any other key goes back
- `x` / BS: remove the part under the cursor
- `c`: clear the board
- `e`: edit the pino program `Pin_data/app.rb`
- `v`: show the pino program output
- `s`: save the board to one of `Pin_data/pin.txt`, `pin2.txt`, `pin3.txt`, `pin4.txt` on the SD card
- `o`: load the board from one of those files
- `?`: help
- `q`: quit to Area512

## Pino

Pino is a Raspberry Pi Pico for the breadboard (add menu: Microcontroller). It lies with the USB end to the left and straddles the groove, pins 40-21 on row c and pins 1-20 on row h. Only one pino can be on the board. It is powered only by wires: connect battery + to VSYS (or to VBUS, which feeds VSYS through a diode) and battery - to GND. Every GND and AGND pin is joined. The pino is on while VSYS is 1.8V or more; then 3V3(OUT) gives 3.3V and the program runs. Without power, 3V3(OUT), the GPIO pins and the on-board LED are off and a running program stops (the output shown by `v` says `power off`).

`e` opens `Pin_data/app.rb` in a vim-like editor (`:w` save, `:q` quit). Typing `.` lists the methods of the receiver, typing a capital letter lists the classes. Each time the pino gets power, it compiles and runs `app.rb` with PicoRuby (mruby VM); leaving the editor runs it again if the pino has power. `GPIO` drives the pino pins, not the Cardputer's own pins:

```ruby
led = GPIO.new(15, GPIO::OUT)
button = GPIO.new(14, GPIO::IN | GPIO::PULL_UP)

loop do
  led.write(button.read == 0 ? 1 : 0)
  sleep_ms 10
end
```

- GPIO numbers are the Pico's (GP0-GP22, GP26-GP28); `"GP15"` also works. GP25 is the on-board LED, GP24 reads 1 while VBUS is 1.8V or more.
- An output pin is 3.3V or 0V through 50 ohm; a pull-up / pull-down is 50k ohm.
- `puts` goes to the output shown by `v` (last 8 lines). Errors are shown there too.
- Removing the pino, clearing the board, losing power or opening the editor stops the program.
