#include "emu-ui/theme.hpp"

#include <iterator>

namespace emu_ui {

const ConsoleStyle &console_style(std::string_view key) {
  // accent / secondary / background, then the icon and the background tile
  static const ConsoleStyle styles[] = {
      {"nes", 0xB22222, 0xC9C9D1, 0x17171B, Shape::GAMEPAD, Pattern::RIDGES},
      {"snes", 0x6A5ACD, 0xBDB7E8, 0x1A1830, Shape::GAMEPAD, Pattern::DOTS},
      {"gb", 0x5B7F3A, 0x9BBC0F, 0x0F2A12, Shape::HANDHELD_TALL, Pattern::DOT_MATRIX},
      {"gbc", 0x8E44AD, 0xD2A8E6, 0x1E1228, Shape::HANDHELD_TALL, Pattern::DOT_MATRIX},
      {"gba", 0x4B3FA8, 0xA9A2F0, 0x14122E, Shape::HANDHELD_WIDE, Pattern::DOTS},
      {"sms", 0x1F5FAF, 0x9DC0EE, 0x0E1726, Shape::GAMEPAD, Pattern::RIDGES},
      {"gg", 0x4F6D8F, 0xB4C6DA, 0x121A24, Shape::HANDHELD_WIDE, Pattern::DOT_MATRIX},
      {"genesis", 0xC8102E, 0xE7A1AC, 0x0B0B0C, Shape::GAMEPAD, Pattern::RIBS},
      {"32x", 0xE0A000, 0xF2D27A, 0x141008, Shape::GAMEPAD, Pattern::RIBS},
      {"msx", 0x3A7D7D, 0x9FD3D3, 0x0D1A1A, Shape::COMPUTER, Pattern::SCANLINES},
      {"playstation", 0x2E6DB4, 0xB8C4D6, 0x171A20, Shape::DISC, Pattern::PINSTRIPES},
      {"pc", 0x5A6B7B, 0x7EF0A0, 0x0C1420, Shape::COMPUTER, Pattern::SCANLINES},
      {"pico8", 0xFF004D, 0x29ADFF, 0x1D2B53, Shape::FANTASY, Pattern::PIXELS},
  };
  static const ConsoleStyle other = {"other", 0x606060, 0xB0B0B0, color::background, Shape::GAMEPAD, Pattern::NONE};
  for (const auto &s : styles) {
    if (key == s.key) return s;
  }
  return other;
}

namespace {
// (a size the build has not enabled falls back to the default font)
#if LV_FONT_MONTSERRAT_12
#define EMU_UI_FONT_12 &lv_font_montserrat_12
#else
#define EMU_UI_FONT_12 LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_14
#define EMU_UI_FONT_14 &lv_font_montserrat_14
#else
#define EMU_UI_FONT_14 LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_16
#define EMU_UI_FONT_16 &lv_font_montserrat_16
#else
#define EMU_UI_FONT_16 LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_20
#define EMU_UI_FONT_20 &lv_font_montserrat_20
#else
#define EMU_UI_FONT_20 LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_24
#define EMU_UI_FONT_24 &lv_font_montserrat_24
#else
#define EMU_UI_FONT_24 LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_32
#define EMU_UI_FONT_32 &lv_font_montserrat_32
#else
#define EMU_UI_FONT_32 LV_FONT_DEFAULT
#endif
} // namespace

Metrics Metrics::compact_320x240() {
  return {
      .width = 320,
      .height = 240,
      .font_small = EMU_UI_FONT_12,
      .font_body = EMU_UI_FONT_14,
      .font_title = EMU_UI_FONT_16,
      .font_large = EMU_UI_FONT_16,
      .pad = 6,
      .gap = 2,
      .radius = 4,
      .bar_h = 24,
      .hint_h = 20,
      .row_h = 22,
      .focus_w = 2,
      .pattern_px = 0,
      .compact = true,
  };
}

Metrics Metrics::large_1280x720() {
  return {
      .width = 1280,
      .height = 720,
      .font_small = EMU_UI_FONT_16,
      .font_body = EMU_UI_FONT_20,
      .font_title = EMU_UI_FONT_24,
      .font_large = EMU_UI_FONT_32,
      .pad = 24,
      .gap = 6,
      .radius = 10,
      .bar_h = 72,
      .hint_h = 56,
      .row_h = 60,
      .focus_w = 3,
      .pattern_px = 64,
      .compact = false,
  };
}

} // namespace emu_ui
