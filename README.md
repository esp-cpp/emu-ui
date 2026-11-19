# emu-ui

The launcher and pause menu shared by
[esp-box-emu](https://github.com/esp-cpp/esp-box-emu) (ESP32-S3-BOX, 320x240)
and [tab5-emu](https://github.com/esp-cpp/tab5-emu) (M5Stack Tab5, 1280x720).
An ESP-IDF component that depends on LVGL (9.2+) only: the boards give it
the games, the images and the settings, and it gives back what the player
chose.

- `emu_ui::Launcher`: the games grouped by console over the console's
  background, the selected game's cover and title, the console picker, a
  settings panel, button hints.
- `emu_ui::PauseMenu`: the paused frame, the save slot with its screenshot,
  resume / save / load / reset / quit and the board's settings.
- `emu_ui::RowList`: the rows both use for settings (sliders, toggles,
  choices, numbers, actions), driven by a keypad or touch.
- `emu_ui::console_style()`: each console's accent, colors, icon and
  background pattern (small original drawings, no logos), by the console
  key the boards' `rom_info` uses.
- `emu_ui::Metrics`: sizes and fonts for a small (320x240) or large
  (1280x720) screen.

## Keys

The views take LVGL keypad keys from the board's keypad input device:
up / down, left / right, `LV_KEY_ENTER` (A), `LV_KEY_ESC` (B) and
`LV_KEY_HOME` (X). The list, the rows and the picker set
`LV_OBJ_FLAG_USER_1`: they want every arrow as a key, not as a focus move
(the Tab5's keypad turns arrows into focus moves for other objects). A
button the keypad does not deliver can be passed in with
`Launcher::press()`.

## Fonts

Montserrat 12, 14 and 16 on a small screen; 16, 20, 24 and 32 on a large
one (`CONFIG_LV_FONT_MONTSERRAT_*`). A size that is not enabled falls back
to the default font.

## Preview on a desktop

`tools/preview` renders every screen at both sizes into PPM files, with
placeholder covers, without a board:

```sh
cmake -S tools/preview -B build/preview -DLVGL_DIR=<an emulator build>/managed_components/lvgl__lvgl
cmake --build build/preview -j
./build/preview/preview out/
```
