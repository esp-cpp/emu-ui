#include "emu-ui/pause_menu.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

// (LVGL keeps it in a private header)
extern "C" void lv_image_cache_drop(const void *src);

namespace emu_ui {

namespace {
// large screens: the paused frame on the left, the menu on the right
constexpr int kFrameW = 560, kFrameH = 420;
constexpr int kSlotW = 160, kSlotH = 120;
// small screens
constexpr int kCompactW = 128, kCompactH = 96;
} // namespace

PauseMenu::PauseMenu(Config config) : cfg_(std::move(config)), m_(cfg_.metrics) { build_ui(); }

PauseMenu::~PauseMenu() {
  if (rows_ && cfg_.keypad && lv_indev_get_group(cfg_.keypad) == rows_->group()) {
    lv_indev_set_group(cfg_.keypad, nullptr);
  }
  rows_.reset();
  if (screen_) lv_obj_delete(screen_);
}

void PauseMenu::build_ui() {
  const int W = m_.width, H = m_.height;
  const bool compact = m_.compact;
  screen_ = lv_obj_create(nullptr);
  lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen_, rgb(color::background), 0);
  backdrop_.create(screen_, m_.pattern_px);

  auto *bar = box_create(screen_);
  const int bar_h = compact ? 20 : m_.bar_h;
  lv_obj_set_size(bar, W, bar_h);
  lv_obj_set_style_bg_color(bar, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_40, 0);
  lv_obj_set_style_pad_hor(bar, m_.pad, 0);
  lv_obj_set_style_pad_column(bar, compact ? 6 : 16, 0);
  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  label_create(bar, compact ? m_.font_body : m_.font_title, color::text, LV_SYMBOL_PAUSE "  Paused");
  title_ = label_create(bar, m_.font_small, color::text_dim, "");
  lv_obj_set_size(title_, 1, lv_font_get_line_height(m_.font_small));
  lv_obj_set_flex_grow(title_, 1);
  // (scrolls when it does not fit beside "Paused", the FPS and the battery)
  lv_label_set_long_mode(title_, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
  fps_ = label_create(bar, m_.font_small, color::text_dim, "");
  battery_ = label_create(bar, m_.font_small, color::text, battery_text(-1, false).c_str());

  const int top = bar_h + (compact ? 4 : 24);
  const int bottom = H - m_.hint_h - (compact ? 2 : 16);
  pause_image_ = lv_image_create(screen_);
  slot_image_ = lv_image_create(screen_);
  for (auto *img : {pause_image_, slot_image_}) {
    lv_obj_set_style_bg_color(img, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(img, LV_OPA_60, 0);
    lv_obj_set_style_radius(img, m_.radius, 0);
    lv_obj_set_style_clip_corner(img, true, 0);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CONTAIN);
  }
  int rows_x;
  if (compact) {
    lv_obj_set_pos(pause_image_, m_.pad, top);
    lv_obj_set_size(pause_image_, kCompactW, kCompactH);
    lv_obj_set_pos(slot_image_, m_.pad, top + kCompactH + 4);
    lv_obj_set_size(slot_image_, kCompactW, bottom - (top + kCompactH + 4));
    slot_caption_ = label_create(slot_image_, m_.font_small, color::text, "");
    lv_obj_set_style_bg_color(slot_caption_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(slot_caption_, LV_OPA_60, 0);
    lv_obj_set_style_pad_hor(slot_caption_, 4, 0);
    lv_obj_align(slot_caption_, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    rows_x = m_.pad + kCompactW + 4;
  } else {
    lv_obj_set_pos(pause_image_, m_.pad, top);
    lv_obj_set_size(pause_image_, kFrameW, kFrameH);
    const int slot_y = top + kFrameH + 16;
    lv_obj_set_pos(slot_image_, m_.pad, slot_y);
    lv_obj_set_size(slot_image_, kSlotW, std::min(kSlotH, bottom - slot_y));
    slot_caption_ = label_create(screen_, m_.font_title, color::text, "");
    lv_obj_set_pos(slot_caption_, m_.pad + kSlotW + 20, slot_y + 8);
    rows_x = m_.pad + kFrameW + 32;
  }

  rows_ = std::make_unique<RowList>();
  rows_->create(screen_, m_);
  lv_obj_set_pos(rows_->root(), rows_x - m_.focus_w - 1, top - m_.focus_w - 1);
  lv_obj_set_size(rows_->root(), W - rows_x - (compact ? 2 : m_.pad) + m_.focus_w + 1, bottom - top + 2 * (m_.focus_w + 1));
  auto action = [this](Action a) {
    return [this, a](int) {
      if (cfg_.on_action) cfg_.on_action(a);
    };
  };
  rows_->add({.label = "Resume", .kind = Row::Kind::ACTION, .set = action(Action::RESUME), .symbol = LV_SYMBOL_PLAY});
  rows_->add({.label = "Save", .kind = Row::Kind::ACTION, .set = action(Action::SAVE), .symbol = LV_SYMBOL_SAVE});
  rows_->add({.label = "Load", .kind = Row::Kind::ACTION, .set = action(Action::LOAD), .symbol = LV_SYMBOL_UPLOAD});
  std::vector<std::string> slots;
  for (int i = 0; i < cfg_.slots; i++) slots.push_back(std::to_string(i));
  slot_row_ = rows_->add({.label = "Slot",
                          .kind = Row::Kind::CHOICE,
                          .choices = slots,
                          .get = [this] { return slot_; },
                          .set = [this](int v) { set_slot(v); },
                          .symbol = LV_SYMBOL_DRIVE});
  rows_->add({.label = "Reset", .kind = Row::Kind::ACTION, .set = action(Action::RESET), .symbol = LV_SYMBOL_REFRESH});
  rows_->add({.label = "Quit", .kind = Row::Kind::ACTION, .set = action(Action::QUIT), .symbol = LV_SYMBOL_CLOSE,
              .danger = true});
  if (!cfg_.settings.empty()) {
    rows_->add_heading("SETTINGS");
    for (const auto &row : cfg_.settings) rows_->add(row);
  }
  rows_->on_back = [this] {
    if (cfg_.on_action) cfg_.on_action(Action::RESUME);
  };

  hints_.create(screen_, m_);
  hints_.set({{LV_SYMBOL_LEFT LV_SYMBOL_RIGHT, "Change"}, {"A", "Select"}, {"B", "Resume"}});
  update_slot_text();
}

void PauseMenu::show() {
  rows_->refresh();
  lv_screen_load(screen_);
  if (cfg_.keypad) lv_indev_set_group(cfg_.keypad, rows_->group());
  rows_->focus_first();
  lv_obj_scroll_to_y(rows_->root(), 0, LV_ANIM_OFF);
}

void PauseMenu::set_game(const std::string &title, const std::string &console_key) {
  lv_label_set_text(title_, title.c_str());
  backdrop_.set(console_style(console_key));
}

void PauseMenu::set_pause_image(const void *src) {
  lv_image_set_src(pause_image_, nullptr);
  if (src) {
    lv_image_cache_drop(src);
    lv_image_set_src(pause_image_, src);
  }
}

void PauseMenu::set_slot_image(const void *src) {
  lv_image_set_src(slot_image_, nullptr);
  if (src) {
    lv_image_cache_drop(src);
    lv_image_set_src(slot_image_, src);
  }
  update_slot_text();
}

void PauseMenu::set_slot(int slot) {
  if (slot < 0) slot = cfg_.slots - 1;
  if (slot >= cfg_.slots) slot = 0;
  const bool changed = slot != slot_;
  slot_ = slot;
  if (slot_row_ >= 0) rows_->refresh(slot_row_);
  update_slot_text();
  if (changed && cfg_.on_slot) cfg_.on_slot(slot_);
}

void PauseMenu::update_slot_text() {
  const bool saved = lv_image_get_src(slot_image_) != nullptr;
  char text[48];
  snprintf(text, sizeof(text), m_.compact ? "Slot %d%s" : "Slot %d\n%s", slot_, saved ? "" : (m_.compact ? ": empty" : "Empty"));
  lv_label_set_text(slot_caption_, text);
}

void PauseMenu::set_fps(float fps) {
  // (snprintf, not lv_label_set_text_fmt: LVGL's own printf may be built
  // without floats, as on the box, and then prints no number)
  char text[16];
  std::snprintf(text, sizeof(text), "%.1f FPS", fps);
  lv_label_set_text(fps_, text);
}

void PauseMenu::set_battery(int percent, bool charging) {
  lv_label_set_text(battery_, battery_text(percent, charging).c_str());
}

void PauseMenu::refresh_settings() { rows_->refresh(); }

void PauseMenu::click(Action action) {
  if (cfg_.on_action) cfg_.on_action(action);
}

bool load_screenshot(const char *path, std::span<uint8_t> storage, lv_image_dsc_t &desc) {
  // stdio, not ifstream: a read error from the card makes libstdc++'s filebuf
  // throw, and exceptions are off
  FILE *file = fopen(path, "rb");
  if (!file) return false;
  uint8_t header[4];
  const bool header_ok = fread(header, 1, sizeof(header), file) == sizeof(header);
  const uint16_t w = (header[0] << 8) | header[1];
  const uint16_t h = (header[2] << 8) | header[3];
  const size_t size = static_cast<size_t>(w) * h * 2;
  if (!header_ok || w == 0 || h == 0 || w > 2048 || h > 2048 || size > storage.size()) {
    fclose(file);
    return false;
  }
  const bool data_ok = fread(storage.data(), 1, size, file) == size;
  fclose(file);
  if (!data_ok) return false;
  memset(&desc, 0, sizeof(desc));
  desc.header.magic = LV_IMAGE_HEADER_MAGIC;
  desc.header.cf = LV_COLOR_FORMAT_RGB565;
  desc.header.w = w;
  desc.header.h = h;
  desc.header.stride = w * 2;
  desc.data_size = size;
  desc.data = storage.data();
  return true;
}

} // namespace emu_ui
