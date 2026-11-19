#include "emu-ui/widgets.hpp"

#include <algorithm>
#include <cstdio>
#include <map>

namespace emu_ui {

lv_obj_t *box_create(lv_obj_t *parent) {
  auto *o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
  return o;
}

lv_obj_t *label_create(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text) {
  auto *l = lv_label_create(parent);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, rgb(color), 0);
  return l;
}

namespace {

lv_obj_t *shape(lv_obj_t *parent, int x, int y, int w, int h, int radius, lv_color_t color) {
  auto *o = box_create(parent);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_bg_color(o, color, 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  return o;
}

// a + shaped d-pad centered at (cx, cy), `s` across
void dpad(lv_obj_t *parent, int cx, int cy, int s, lv_color_t color) {
  const int t = std::max(2, s / 3);
  shape(parent, cx - s / 2, cy - t / 2, s, t, 1, color);
  shape(parent, cx - t / 2, cy - s / 2, t, s, 1, color);
}

void dot(lv_obj_t *parent, int cx, int cy, int d, lv_color_t color) {
  shape(parent, cx - d / 2, cy - d / 2, d, d, LV_RADIUS_CIRCLE, color);
}

} // namespace

lv_obj_t *icon_create(lv_obj_t *parent, Shape kind, uint32_t accent_hex, int h) {
  const int w = h * 8 / 5;
  auto *box = box_create(parent);
  lv_obj_set_size(box, w, h);

  const lv_color_t body = lv_color_white();
  const lv_color_t accent = rgb(accent_hex);
  const lv_color_t dark = rgb(0x202428);

  switch (kind) {
  case Shape::GAMEPAD: {
    // a wide rounded controller: d-pad left, two buttons right
    const int bh = h * 6 / 10, by = (h - bh) / 2;
    shape(box, 0, by, w, bh, bh / 2, body);
    dpad(box, w * 27 / 100, h / 2, bh * 6 / 10, accent);
    dot(box, w * 66 / 100, h / 2 + bh / 8, bh / 4, accent);
    dot(box, w * 80 / 100, h / 2 - bh / 8, bh / 4, accent);
    break;
  }
  case Shape::HANDHELD_TALL: {
    // a portrait handheld: screen on top, d-pad and buttons below it
    const int bw = h * 66 / 100, bx = (w - bw) / 2;
    shape(box, bx, 0, bw, h, bw / 6, body);
    shape(box, bx + bw / 8, h / 10, bw * 6 / 8, h * 4 / 10, 2, dark);
    dpad(box, bx + bw * 30 / 100, h * 72 / 100, bw * 3 / 10, accent);
    dot(box, bx + bw * 68 / 100, h * 76 / 100, bw / 6, accent);
    dot(box, bx + bw * 84 / 100, h * 66 / 100, bw / 6, accent);
    break;
  }
  case Shape::HANDHELD_WIDE: {
    // a landscape handheld: screen in the middle, d-pad and buttons beside it
    const int bh = h * 7 / 10, by = (h - bh) / 2;
    shape(box, 0, by, w, bh, bh * 4 / 10, body);
    shape(box, w * 30 / 100, by + bh / 6, w * 40 / 100, bh * 2 / 3, 2, dark);
    dpad(box, w * 15 / 100, h / 2, bh * 4 / 10, accent);
    dot(box, w * 82 / 100, h / 2 + bh / 10, bh / 5, accent);
    dot(box, w * 90 / 100, h / 2 - bh / 10, bh / 5, accent);
    break;
  }
  case Shape::DISC: {
    // a CD: rim, label ring, hole
    const int d = h, x = (w - d) / 2;
    shape(box, x, 0, d, d, LV_RADIUS_CIRCLE, body);
    dot(box, w / 2, h / 2, d * 45 / 100, accent);
    dot(box, w / 2, h / 2, d * 15 / 100, body);
    break;
  }
  case Shape::COMPUTER: {
    // a monitor on a stand
    const int mw = w * 70 / 100, mh = h * 66 / 100, mx = (w - mw) / 2;
    shape(box, mx, 0, mw, mh, 3, body);
    shape(box, mx + mw / 12, mh / 8, mw * 10 / 12, mh * 6 / 8, 1, accent);
    shape(box, w / 2 - w / 14, mh, w / 7, h / 6, 0, body);
    shape(box, w / 2 - w / 5, h - h / 8, w * 2 / 5, h / 8, 2, body);
    break;
  }
  case Shape::FANTASY: {
    // a tiny cartridge-console: a square with a screen of palette dots
    const int s = h, x = (w - s) / 2;
    shape(box, x, 0, s, s, s / 6, body);
    const int sx = x + s / 8, sy = s / 8, ss = s * 6 / 8;
    shape(box, sx, sy, ss, ss * 3 / 4, 2, dark);
    const uint32_t palette[] = {0xFF004D, 0xFFA300, 0xFFEC27, 0x00E436};
    const int p = std::max(2, ss / 6);
    for (int i = 0; i < 4; i++) {
      shape(box, sx + ss / 10 + i * (p + p / 3), sy + ss * 3 / 8 - p / 2, p, p, 0, rgb(palette[i]));
    }
    break;
  }
  }
  return box;
}

lv_obj_t *badge_create(lv_obj_t *parent, const ConsoleStyle &style, const char *name, int h, const lv_font_t *font) {
  auto *badge = box_create(parent);
  lv_obj_set_size(badge, LV_SIZE_CONTENT, h);
  lv_obj_set_style_radius(badge, h / 2, 0);
  lv_obj_set_style_bg_color(badge, rgb(style.accent), 0);
  lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_hor(badge, h / 3, 0);
  lv_obj_set_style_pad_column(badge, h / 4, 0);
  lv_obj_set_flex_flow(badge, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(badge, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  icon_create(badge, style.shape, style.accent, h * 6 / 10);
  label_create(badge, font, 0xFFFFFF, name);
  return badge;
}

namespace {

uint16_t rgb565(uint32_t hex) {
  const uint8_t r = hex >> 16, g = hex >> 8, b = hex;
  return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// `a` moved toward `b` by `t` / 256
uint32_t mix(uint32_t a, uint32_t b, int t) {
  auto ch = [&](int s) {
    const int x = (a >> s) & 0xFF, y = (b >> s) & 0xFF;
    return static_cast<uint32_t>(x + (y - x) * t / 256) << s;
  };
  return ch(16) | ch(8) | ch(0);
}

lv_draw_buf_t *make_tile(const ConsoleStyle &s) {
  // (the marks are the secondary color, faint over the background, so text
  // on top stays readable)
  const uint16_t bg = rgb565(s.background);
  const uint16_t mark = rgb565(mix(s.background, s.secondary, 22));
  const uint16_t strong = rgb565(mix(s.background, s.secondary, 40));
  int w = 16, h = 16;
  switch (s.pattern) {
  case Pattern::RIDGES: w = 8; h = 9; break;
  case Pattern::DOTS: w = h = 22; break;
  case Pattern::DOT_MATRIX: w = h = 6; break;
  case Pattern::RIBS: w = 14; h = 8; break;
  case Pattern::PINSTRIPES: w = h = 12; break;
  case Pattern::SCANLINES: w = 8; h = 4; break;
  case Pattern::PIXELS: w = h = 16; break;
  case Pattern::NONE: w = h = 8; break;
  }
  auto *buf = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
  if (!buf) return nullptr;
  for (int y = 0; y < h; y++) {
    auto *row = reinterpret_cast<uint16_t *>(buf->data + y * buf->header.stride);
    for (int x = 0; x < w; x++) {
      uint16_t c = bg;
      switch (s.pattern) {
      case Pattern::RIDGES: c = y < 3 ? mark : bg; break;
      case Pattern::DOTS: c = (x >= 10 && x <= 11 && y >= 10 && y <= 11) ? strong : bg; break;
      case Pattern::DOT_MATRIX: c = (x >= 2 && x <= 3 && y >= 2 && y <= 3) ? mark : bg; break;
      case Pattern::RIBS: c = x < 2 ? mark : bg; break;
      case Pattern::PINSTRIPES: c = ((x + y) % 12) == 0 ? mark : bg; break;
      case Pattern::SCANLINES: c = y == 0 ? mark : bg; break;
      case Pattern::PIXELS: c = ((x / 8) ^ (y / 8)) & 1 ? rgb565(mix(s.background, s.secondary, 10)) : bg; break;
      case Pattern::NONE: break;
      }
      row[x] = c;
    }
  }
  return buf;
}

} // namespace

const lv_draw_buf_t *pattern_tile(const ConsoleStyle &style) {
  static std::map<const ConsoleStyle *, lv_draw_buf_t *> tiles;
  auto &tile = tiles[&style];
  if (!tile) tile = make_tile(style);
  return tile;
}

void Backdrop::create(lv_obj_t *parent) {
  parent_ = parent;
  image_ = lv_image_create(parent);
  lv_obj_remove_flag(image_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(image_, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_size(image_, lv_pct(100), lv_pct(100));
  lv_image_set_inner_align(image_, LV_IMAGE_ALIGN_TILE);
  lv_obj_move_to_index(image_, 0);
}

void Backdrop::set(const ConsoleStyle &style) {
  if (shown_ == &style || !parent_) return;
  shown_ = &style;
  lv_obj_set_style_bg_color(parent_, rgb(style.background), 0);
  lv_obj_set_style_bg_opa(parent_, LV_OPA_COVER, 0);
  if (style.pattern == Pattern::NONE) {
    lv_obj_add_flag(image_, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_remove_flag(image_, LV_OBJ_FLAG_HIDDEN);
  lv_image_set_src(image_, pattern_tile(style));
}

void HintBar::create(lv_obj_t *parent, const Metrics &m) {
  m_ = &m;
  root_ = box_create(parent);
  lv_obj_set_size(root_, lv_pct(100), m.hint_h);
  lv_obj_align(root_, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(root_, LV_OPA_50, 0);
  lv_obj_set_style_pad_hor(root_, m.pad, 0);
  lv_obj_set_style_pad_column(root_, m.compact ? 10 : 28, 0);
  lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

void HintBar::set(std::initializer_list<Hint> hints, std::string_view right) {
  lv_obj_clean(root_);
  const int k = m_->compact ? 14 : 30; // the button's disc
  for (const auto &h : hints) {
    auto *item = box_create(root_);
    lv_obj_set_size(item, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(item, m_->compact ? 3 : 8, 0);
    auto *disc = box_create(item);
    // one letter: a disc; a word or arrows: a rounded tag
    const bool letter = std::string_view(h.button).size() == 1;
    lv_obj_set_size(disc, letter ? k : LV_SIZE_CONTENT, k);
    lv_obj_set_style_pad_hor(disc, letter ? 0 : k / 4, 0);
    lv_obj_set_style_radius(disc, letter ? LV_RADIUS_CIRCLE : k / 4, 0);
    lv_obj_set_style_bg_color(disc, rgb(0xE8EAEE), 0);
    lv_obj_set_style_bg_opa(disc, LV_OPA_COVER, 0);
    auto *b = label_create(disc, m_->font_small, 0x111111, h.button);
    lv_obj_center(b);
    label_create(item, m_->font_small, 0xD6DAE0, h.action);
  }
  if (!right.empty()) {
    auto *spacer = box_create(root_);
    lv_obj_set_height(spacer, 1);
    lv_obj_set_flex_grow(spacer, 1);
    label_create(root_, m_->font_small, color::text_dim, std::string(right).c_str());
  }
}

std::string battery_text(int percent, bool charging) {
  if (percent < 0) return LV_SYMBOL_BATTERY_EMPTY " --%";
  const char *symbol = charging        ? LV_SYMBOL_CHARGE
                       : percent > 90 ? LV_SYMBOL_BATTERY_FULL
                       : percent > 65 ? LV_SYMBOL_BATTERY_3
                       : percent > 40 ? LV_SYMBOL_BATTERY_2
                       : percent > 15 ? LV_SYMBOL_BATTERY_1
                                      : LV_SYMBOL_BATTERY_EMPTY;
  char text[24];
  snprintf(text, sizeof(text), "%d%%  %s", percent, symbol);
  return text;
}

void add_focus_ring(lv_obj_t *obj, const Metrics &m) {
  lv_obj_set_style_outline_color(obj, rgb(color::focus), LV_STATE_FOCUSED);
  lv_obj_set_style_outline_width(obj, m.focus_w, LV_STATE_FOCUSED);
  lv_obj_set_style_outline_pad(obj, 0, LV_STATE_FOCUSED);
  lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_STATE_FOCUSED);
}

} // namespace emu_ui
