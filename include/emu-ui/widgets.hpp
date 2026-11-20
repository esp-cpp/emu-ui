#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

#include "lvgl.h"

#include "emu-ui/theme.hpp"

/// The building blocks the launcher and the pause menu share.
namespace emu_ui {

/// An empty object: no theme style, not scrollable, not clickable.
lv_obj_t *box_create(lv_obj_t *parent);
/// A label in `font` and `color`.
lv_obj_t *label_create(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text = "");

/// A console's drawing, `h` tall (8:5), its buttons in `accent`.
lv_obj_t *icon_create(lv_obj_t *parent, Shape shape, uint32_t accent, int h);
/// A pill in the console's accent with its icon and `name`.
lv_obj_t *badge_create(lv_obj_t *parent, const ConsoleStyle &style, const char *name, int h, const lv_font_t *font);

/// The console's background tile (RGB565, made once per console and kept).
const lv_draw_buf_t *pattern_tile(const ConsoleStyle &style, int min_px = 0);

/// A console's background behind a screen or a tile: its color with its
/// pattern tiled over it. The first child of `parent`, filling it.
class Backdrop {
public:
  /// `pattern_px`: Metrics::pattern_px
  void create(lv_obj_t *parent, int pattern_px = 0);
  void set(const ConsoleStyle &style);
  const ConsoleStyle *shown() const { return shown_; }

private:
  lv_obj_t *parent_{nullptr};
  lv_obj_t *image_{nullptr};
  int pattern_px_{0};
  const ConsoleStyle *shown_{nullptr};
};

/// The button hints along the bottom of a screen: "(A) Play  (B) Settings".
class HintBar {
public:
  struct Hint {
    const char *button; ///< "A", "B", "X", "◀▶" ...
    const char *action;
  };
  void create(lv_obj_t *parent, const Metrics &m);
  void set(std::initializer_list<Hint> hints, std::string_view right = {});
  lv_obj_t *root() const { return root_; }

private:
  const Metrics *m_{nullptr};
  lv_obj_t *root_{nullptr};
};

/// The battery's symbol and percent, "--%" until the first report.
std::string battery_text(int percent, bool charging);

/// The keypad focus ring for `obj` (white outline while focused).
void add_focus_ring(lv_obj_t *obj, const Metrics &m);

} // namespace emu_ui
