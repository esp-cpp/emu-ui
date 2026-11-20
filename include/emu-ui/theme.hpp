#pragma once

#include <cstdint>
#include <string_view>

#include "lvgl.h"

/// emu-ui: the launcher and pause menu shared by esp-box-emu and tab5-emu.
/// LVGL only: the boards feed it games, images and settings and get back
/// what the player chose, so the same screens run on a 320x240 S3-BOX and a
/// 1280x720 Tab5 (Metrics picks the sizes).
namespace emu_ui {

/// The UI's own colors (0xRRGGBB), the same on every screen.
namespace color {
constexpr uint32_t background = 0x101418;
constexpr uint32_t surface = 0x1B2027;      ///< panels, rows
constexpr uint32_t surface_high = 0x2A313B; ///< a row under the keypad focus
constexpr uint32_t text = 0xF2F4F7;
constexpr uint32_t text_dim = 0x9AA3B0;
constexpr uint32_t focus = 0xFFFFFF; ///< the keypad focus ring
constexpr uint32_t danger = 0xE5484D;
} // namespace color

/// The drawing for a console without art on the SD card.
enum class Shape : uint8_t { GAMEPAD, HANDHELD_TALL, HANDHELD_WIDE, DISC, COMPUTER, FANTASY };

/// The background tile behind a console's games. Small original drawings
/// that call back to the hardware (no logos).
enum class Pattern : uint8_t {
  NONE,
  RIDGES,     ///< horizontal ridges, a cartridge's grip
  DOTS,       ///< a sparse dot grid
  DOT_MATRIX, ///< a handheld's dot-matrix screen
  RIBS,       ///< vertical ribs of a console case
  PINSTRIPES, ///< diagonal pinstripes
  SCANLINES,  ///< a monitor's scanlines
  PIXELS,     ///< a checker of big pixels
};

struct ConsoleStyle {
  const char *key;     ///< rom_info's ConsoleInfo::key ("nes", "snes", ...)
  uint32_t accent;     ///< chips, the selected row, the icon's buttons
  uint32_t secondary;  ///< the console's name over its games, the pattern's marks
  uint32_t background; ///< under the pattern
  Shape shape;
  Pattern pattern;
};

/// The style of the console `key`; an unknown key gets a neutral one.
const ConsoleStyle &console_style(std::string_view key);

/// Sizes and fonts for a class of screen.
struct Metrics {
  int16_t width, height;
  const lv_font_t *font_small; ///< hints, notes, counts
  const lv_font_t *font_body;  ///< rows
  const lv_font_t *font_title; ///< headings
  const lv_font_t *font_large; ///< the selected game's title
  int16_t pad;     ///< the screen's margin
  int16_t gap;     ///< between items
  int16_t radius;  ///< rows, panels, chips
  int16_t bar_h;   ///< the status bar at the top
  int16_t hint_h;  ///< the button hints at the bottom
  int16_t row_h;   ///< a game or setting row
  int16_t focus_w; ///< the focus ring
  /// A backdrop's pattern tile repeats the pattern to at least this many
  /// pixels a side (0: one period, a few pixels). LVGL draws a tiled image
  /// one tile at a time, so a big screen covered in 6 px tiles costs tens of
  /// thousands of tiny draws per redraw; 128 px tiles take a few dozen.
  int16_t pattern_px;
  bool compact;    ///< a small screen: overlays fill it, covers are not scaled

  /// The ESP32-S3-BOX's 320x240 (Montserrat 12 / 14 / 16)
  static Metrics compact_320x240();
  /// The Tab5's 1280x720 (Montserrat 16 / 20 / 24 / 32)
  static Metrics large_1280x720();
};

inline lv_color_t rgb(uint32_t hex) { return lv_color_hex(hex); }

} // namespace emu_ui
