#include "emu-ui/launcher.hpp"

#include <algorithm>
#include <cctype>

// (LVGL keeps it in a private header)
extern "C" void lv_image_cache_drop(const void *src);

namespace emu_ui {

namespace {
// large screens: the list on the left, the selected game on the right
constexpr int kListW = 456;
constexpr int kCover = 360;
// small screens: the cover on the left, the list on the right
constexpr int kCompactCoverW = 100;
constexpr int kCompactCoverH = 136;

intptr_t index_of(lv_obj_t *obj) { return reinterpret_cast<intptr_t>(lv_obj_get_user_data(obj)); }
void set_index(lv_obj_t *obj, intptr_t i) { lv_obj_set_user_data(obj, reinterpret_cast<void *>(i)); }

bool from_keypad() {
  auto *indev = lv_indev_active();
  return indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD;
}
} // namespace

Launcher::Launcher(Config config) : cfg_(std::move(config)), m_(cfg_.metrics) { build_ui(); }

Launcher::~Launcher() {
  close_picker();
  close_settings();
  if (group_) lv_group_delete(group_);
  if (screen_) lv_obj_delete(screen_);
}

void Launcher::build_ui() {
  const int W = m_.width, H = m_.height;
  screen_ = lv_obj_create(nullptr);
  lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen_, rgb(color::background), 0);
  backdrop_.create(screen_);

  // the status bar
  bar_ = box_create(screen_);
  lv_obj_set_size(bar_, W, m_.bar_h);
  lv_obj_set_style_bg_color(bar_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(bar_, m_.compact ? LV_OPA_40 : LV_OPA_30, 0);
  battery_ = label_create(bar_, m_.font_small, color::text, battery_text(-1, false).c_str());
  lv_obj_align(battery_, LV_ALIGN_RIGHT_MID, -m_.pad, 0);
  if (m_.compact) {
    // the selected game's console: a chip (tap: the console picker)
    chip_area_ = box_create(bar_);
    lv_obj_add_flag(chip_area_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(chip_area_, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_size(chip_area_, LV_SIZE_CONTENT, m_.bar_h - 6);
    lv_obj_align(chip_area_, LV_ALIGN_LEFT_MID, m_.pad, 0);
    lv_obj_add_event_cb(chip_area_, [](lv_event_t *e) {
      static_cast<Launcher *>(lv_event_get_user_data(e))->open_picker();
    }, LV_EVENT_CLICKED, this);
    system_ = label_create(bar_, m_.font_small, color::text_dim, "");
    lv_obj_align_to(system_, chip_area_, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
  } else {
    auto *brand = label_create(bar_, m_.font_title, color::text, cfg_.brand.c_str());
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, m_.pad, 0);
    // the consoles as tabs (scroll sideways when there are many)
    tabs_ = box_create(bar_);
    lv_obj_add_flag(tabs_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(tabs_, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(tabs_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(tabs_, W - 2 * 200, m_.bar_h);
    lv_obj_set_x(tabs_, 200);
    lv_obj_set_flex_flow(tabs_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tabs_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tabs_, 10, 0);
    lv_obj_set_style_pad_hor(tabs_, 4, 0);
    // settings: a gear left of the battery
    settings_btn_ = box_create(bar_);
    lv_obj_add_flag(settings_btn_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(settings_btn_, 48, 48);
    lv_obj_set_style_radius(settings_btn_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(settings_btn_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(settings_btn_, LV_OPA_10, 0);
    lv_obj_align(settings_btn_, LV_ALIGN_RIGHT_MID, -m_.pad - 100, 0);
    auto *gear = label_create(settings_btn_, m_.font_body, color::text, LV_SYMBOL_SETTINGS);
    lv_obj_center(gear);
    lv_obj_add_event_cb(settings_btn_, [](lv_event_t *e) {
      static_cast<Launcher *>(lv_event_get_user_data(e))->open_settings();
    }, LV_EVENT_CLICKED, this);
  }

  // the game list: one keypad object; its rows are only for touch
  list_ = box_create(screen_);
  lv_obj_add_flag(list_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(list_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(list_, LV_OBJ_FLAG_USER_1); // (every arrow as a key)
  lv_obj_set_scroll_dir(list_, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list_, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(list_, m_.gap, 0);
  lv_obj_set_style_pad_all(list_, m_.focus_w + 1, 0);
  lv_obj_set_style_pad_right(list_, m_.compact ? 6 : 12, 0); // (the scrollbar)
  const int top = m_.bar_h + (m_.compact ? 2 : 24);
  const int bottom = H - m_.hint_h - (m_.compact ? 2 : 16);
  if (m_.compact) {
    lv_obj_set_pos(list_, m_.pad + kCompactCoverW + 4, top);
    lv_obj_set_size(list_, W - (m_.pad + kCompactCoverW + 4) - 2, bottom - top);
  } else {
    lv_obj_set_pos(list_, m_.pad - m_.focus_w - 1, top);
    lv_obj_set_size(list_, kListW, bottom - top);
  }
  lv_obj_add_event_cb(list_, list_event, LV_EVENT_KEY, this);

  // the selected game
  cover_ = lv_image_create(screen_);
  lv_obj_add_flag(cover_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(cover_, [](lv_event_t *e) {
    static_cast<Launcher *>(lv_event_get_user_data(e))->play();
  }, LV_EVENT_CLICKED, this);
  if (m_.compact) {
    lv_obj_set_pos(cover_, m_.pad, top + 4);
    lv_obj_set_size(cover_, kCompactCoverW, kCompactCoverH);
    // (the box's covers are made 100 px wide: shown as they are, unscaled)
    lv_image_set_inner_align(cover_, LV_IMAGE_ALIGN_CENTER);
    title_ = label_create(screen_, m_.font_small, color::text, "");
    lv_obj_set_pos(title_, m_.pad, top + 4 + kCompactCoverH + 4);
    lv_obj_set_size(title_, kCompactCoverW, 2 * lv_font_get_line_height(m_.font_small));
    lv_label_set_long_mode(title_, LV_LABEL_LONG_MODE_DOTS);
  } else {
    const int hero_x = m_.pad + kListW + 24;
    const int meta_x = hero_x + kCover + 36, meta_w = W - meta_x - m_.pad;
    const int hero_y = top + (bottom - top - kCover) / 2;
    lv_obj_set_pos(cover_, hero_x, hero_y);
    lv_obj_set_size(cover_, kCover, kCover);
    lv_image_set_inner_align(cover_, LV_IMAGE_ALIGN_CONTAIN);
    auto *meta = box_create(screen_);
    lv_obj_set_pos(meta, meta_x, hero_y);
    lv_obj_set_size(meta, meta_w, kCover);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(meta, 14, 0);
    chip_area_ = box_create(meta);
    lv_obj_set_size(chip_area_, lv_pct(100), LV_SIZE_CONTENT);
    system_ = label_create(meta, m_.font_small, color::text_dim, "");
    lv_obj_set_style_text_letter_space(system_, 3, 0);
    title_ = label_create(meta, m_.font_large, color::text, "");
    lv_obj_set_width(title_, lv_pct(100));
    lv_obj_set_style_max_height(title_, 3 * lv_font_get_line_height(m_.font_large), 0);
    lv_label_set_long_mode(title_, LV_LABEL_LONG_MODE_WRAP);
    subtitle_ = label_create(meta, m_.font_body, color::text_dim, "");
    lv_obj_set_width(subtitle_, lv_pct(100));
    lv_label_set_long_mode(subtitle_, LV_LABEL_LONG_MODE_WRAP);
    auto *spacer = box_create(meta);
    lv_obj_set_width(spacer, 1);
    lv_obj_set_flex_grow(spacer, 1);
    play_btn_ = box_create(meta);
    lv_obj_add_flag(play_btn_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(play_btn_, 220, 68);
    lv_obj_set_style_radius(play_btn_, 14, 0);
    lv_obj_set_style_bg_color(play_btn_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(play_btn_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(play_btn_, rgb(0x5A6270), LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(play_btn_, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_transform_scale(play_btn_, 245, LV_STATE_PRESSED);
    auto *play_label = label_create(play_btn_, m_.font_title, 0x111111, LV_SYMBOL_PLAY "  Play");
    lv_obj_center(play_label);
    lv_obj_add_event_cb(play_btn_, [](lv_event_t *e) {
      static_cast<Launcher *>(lv_event_get_user_data(e))->play();
    }, LV_EVENT_CLICKED, this);
  }

  hints_.create(screen_, m_);
  update_hints();

  group_ = lv_group_create();
  lv_group_add_obj(group_, list_);
}

void Launcher::show() {
  lv_screen_load(screen_);
  if (settings_) attach(settings_rows_->group());
  else if (picker_) attach(picker_group_);
  else attach(group_);
}

void Launcher::attach(lv_group_t *group) {
  if (cfg_.keypad) lv_indev_set_group(cfg_.keypad, group);
  if (group == group_) lv_group_focus_obj(list_);
}

bool Launcher::shown(size_t i) const { return !filter_ || games_[i].console_key == *filter_; }

void Launcher::set_games(std::vector<Game> games, int selected) {
  close_picker();
  games_ = std::move(games);
  if (filter_ && std::none_of(games_.begin(), games_.end(), [&](const Game &g) { return g.console_key == *filter_; })) {
    filter_.reset(); // (its console is gone from the card)
  }
  selected_ = selected;
  build_list();
  build_tabs();
}

void Launcher::build_list() {
  lv_obj_clean(list_);
  rows_.assign(games_.size(), nullptr);
  if (selected_ < 0 || selected_ >= (int)games_.size() || !shown(selected_)) {
    selected_ = -1;
    for (size_t i = 0; i < games_.size() && selected_ < 0; i++) {
      if (shown(i)) selected_ = i;
    }
  }
  std::string last;
  for (size_t i = 0; i < games_.size(); i++) {
    if (!shown(i)) continue;
    const auto &g = games_[i];
    const auto &style = console_style(g.console_key);
    if (g.console_key != last) {
      // a console's games start under its name
      last = g.console_key;
      const auto n = std::count_if(games_.begin(), games_.end(), [&](const Game &x) { return x.console_key == last; });
      auto *hdr = box_create(list_);
      lv_obj_set_width(hdr, lv_pct(100));
      lv_obj_set_height(hdr, LV_SIZE_CONTENT);
      lv_obj_set_style_pad_top(hdr, rows_.empty() || i == 0 ? 0 : (m_.compact ? 4 : 12), 0);
      lv_obj_set_style_pad_left(hdr, m_.compact ? 4 : 8, 0);
      lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
      lv_obj_set_style_pad_column(hdr, m_.compact ? 6 : 10, 0);
      auto *l = label_create(hdr, m_.font_small, style.secondary, (m_.compact ? g.console_short : g.console_name).c_str());
      lv_obj_set_style_text_letter_space(l, m_.compact ? 1 : 2, 0);
      label_create(hdr, m_.font_small, color::text_dim, std::to_string(n).c_str());
    }
    auto *row = box_create(list_);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, m_.row_h);
    lv_obj_set_style_radius(row, m_.radius, 0);
    lv_obj_set_style_bg_color(row, rgb(color::surface), 0);
    lv_obj_set_style_bg_opa(row, m_.compact ? LV_OPA_TRANSP : LV_OPA_50, 0);
    lv_obj_set_style_bg_color(row, rgb(style.accent), LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_set_style_outline_color(row, rgb(color::focus), LV_STATE_CHECKED);
    lv_obj_set_style_outline_width(row, m_.focus_w, LV_STATE_CHECKED);
    lv_obj_set_style_outline_pad(row, 0, LV_STATE_CHECKED);
    lv_obj_set_style_pad_hor(row, m_.compact ? 6 : 16, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    auto *title = label_create(row, m_.font_body, color::text, g.title.c_str());
    lv_obj_set_size(title, lv_pct(100), lv_font_get_line_height(m_.font_body));
    lv_label_set_long_mode(title, LV_LABEL_LONG_MODE_DOTS);
    if (!m_.compact && !g.subtitle.empty()) {
      auto *sub = label_create(row, m_.font_small, 0xC8CDD5, g.subtitle.c_str());
      lv_obj_set_size(sub, lv_pct(100), lv_font_get_line_height(m_.font_small));
      lv_label_set_long_mode(sub, LV_LABEL_LONG_MODE_DOTS);
    }
    set_index(row, i);
    lv_obj_add_event_cb(row, row_event, LV_EVENT_CLICKED, this);
    rows_[i] = row;
  }
  if (selected_ < 0) {
    auto *empty = label_create(list_, m_.font_body, color::text_dim, "No games found.\nAdd metadata.csv to the SD card.");
    lv_obj_set_width(empty, lv_pct(100));
    lv_label_set_long_mode(empty, LV_LABEL_LONG_MODE_WRAP);
  }
  show_selected();
  scroll_to_selected(false);
}

void Launcher::build_tabs() {
  if (!tabs_) return;
  lv_obj_clean(tabs_);
  auto add = [&](const std::string &key, const std::string &name, size_t count) {
    const auto &style = console_style(key);
    auto *tab = box_create(tabs_);
    lv_obj_add_flag(tab, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(tab, LV_SIZE_CONTENT, 44);
    lv_obj_set_style_radius(tab, 22, 0);
    lv_obj_set_style_pad_hor(tab, 16, 0);
    lv_obj_set_style_pad_column(tab, 8, 0);
    lv_obj_set_style_bg_color(tab, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(tab, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(tab, rgb(key.empty() ? 0x3A414C : style.accent), LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, LV_STATE_CHECKED);
    // the selected game's console while the list shows them all
    lv_obj_set_style_border_side(tab, LV_BORDER_SIDE_BOTTOM, LV_STATE_USER_1);
    lv_obj_set_style_border_width(tab, 3, LV_STATE_USER_1);
    lv_obj_set_style_border_color(tab, rgb(style.accent), LV_STATE_USER_1);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (!key.empty()) icon_create(tab, style.shape, style.accent, 20);
    label_create(tab, m_.font_body, color::text, name.c_str());
    label_create(tab, m_.font_small, color::text_dim, std::to_string(count).c_str());
    lv_obj_set_user_data(tab, nullptr);
    lv_obj_add_event_cb(tab, tab_event, LV_EVENT_CLICKED, this);
  };
  add("", "All", games_.size());
  for (const auto &key : consoles()) {
    const auto it = std::find_if(games_.begin(), games_.end(), [&](const Game &g) { return g.console_key == key; });
    const auto n = std::count_if(games_.begin(), games_.end(), [&](const Game &g) { return g.console_key == key; });
    add(key, it->console_short, n);
  }
  update_console_header();
}

void Launcher::scroll_to_selected(bool anim) {
  if (selected_ < 0 || !rows_[selected_]) return;
  lv_obj_update_layout(list_);
  lv_obj_scroll_to_view(rows_[selected_], anim ? LV_ANIM_ON : LV_ANIM_OFF);
}

void Launcher::show_selected() {
  for (size_t i = 0; i < rows_.size(); i++) {
    if (!rows_[i]) continue;
    if ((int)i == selected_) lv_obj_add_state(rows_[i], LV_STATE_CHECKED);
    else lv_obj_remove_state(rows_[i], LV_STATE_CHECKED);
  }
  lv_image_set_src(cover_, nullptr);
  if (selected_ < 0) {
    lv_label_set_text(title_, "");
    if (subtitle_) lv_label_set_text(subtitle_, "");
    update_console_header();
    return;
  }
  const auto &g = games_[selected_];
  lv_label_set_text(title_, g.title.c_str());
  if (subtitle_) {
    lv_label_set_text(subtitle_, play_enabled_ ? g.subtitle.c_str() : "The SD card is a USB drive: switch it off in Settings to play.");
  }
  // the console first: a board may decode its art and the cover into one
  // buffer, so the cover is the last image asked for
  update_console_header();
  if (cfg_.cover) {
    if (const void *src = cfg_.cover(selected_)) {
      lv_image_cache_drop(src); // (a board may reuse one buffer for every cover)
      lv_image_set_src(cover_, src);
    }
  }
  if (cfg_.on_select) cfg_.on_select(selected_);
}

// The selected game's console: the backdrop, the chip / the tabs, its name.
void Launcher::update_console_header() {
  const std::string key = selected_ >= 0 ? games_[selected_].console_key : (filter_ ? *filter_ : "");
  const auto &style = console_style(key);
  backdrop_.set(style);
  if (tabs_) {
    const auto present = consoles();
    for (uint32_t i = 0; i < lv_obj_get_child_count(tabs_); i++) {
      auto *tab = lv_obj_get_child(tabs_, i);
      const std::string tab_key = i == 0 ? "" : (i - 1 < present.size() ? present[i - 1] : "");
      const bool checked = filter_ ? tab_key == *filter_ : i == 0;
      if (checked) lv_obj_add_state(tab, LV_STATE_CHECKED);
      else lv_obj_remove_state(tab, LV_STATE_CHECKED);
      if (!filter_ && i > 0 && tab_key == key) {
        lv_obj_add_state(tab, LV_STATE_USER_1);
        lv_obj_scroll_to_view(tab, LV_ANIM_ON);
      } else {
        lv_obj_remove_state(tab, LV_STATE_USER_1);
      }
    }
  }
  if (selected_ < 0) {
    if (system_) lv_label_set_text(system_, "");
    if (chip_area_) lv_obj_clean(chip_area_);
    return;
  }
  const auto &g = games_[selected_];
  if (m_.compact) {
    // the chip: SD card art or the badge, outlined while filtered
    lv_obj_clean(chip_area_);
    const void *art = cfg_.console_art ? cfg_.console_art(key) : nullptr;
    lv_obj_t *chip;
    if (art) {
      chip = lv_image_create(chip_area_);
      lv_image_set_src(chip, art);
    } else {
      chip = badge_create(chip_area_, style, g.console_short.c_str(), m_.bar_h - 6, m_.font_small);
    }
    lv_obj_set_style_outline_color(chip, lv_color_white(), 0);
    lv_obj_set_style_outline_width(chip, filter_ ? 2 : 0, 0);
    lv_obj_set_style_outline_pad(chip, 1, 0);
    const auto n = std::count_if(games_.begin(), games_.end(), [&](const Game &x) { return x.console_key == key; });
    lv_label_set_text_fmt(system_, "%d %s", (int)n, n == 1 ? "game" : "games");
    lv_obj_align_to(system_, chip_area_, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
  } else {
    lv_obj_clean(chip_area_);
    if (const void *art = cfg_.console_art ? cfg_.console_art(key) : nullptr) {
      auto *img = lv_image_create(chip_area_);
      lv_image_set_src(img, art);
      lv_obj_add_flag(system_, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_remove_flag(system_, LV_OBJ_FLAG_HIDDEN);
    }
    std::string name = g.console_name;
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
    lv_label_set_text(system_, name.c_str());
    lv_obj_set_style_text_color(system_, rgb(style.secondary), 0);
  }
}

void Launcher::update_hints() {
  const char *arrows = LV_SYMBOL_LEFT LV_SYMBOL_RIGHT;
  if (settings_) {
    hints_.set({{arrows, "Change"}, {"A", "Select"}, {"B", "Back"}});
  } else if (picker_) {
    hints_.set({{"A", "Show"}, {"B", "Back"}});
  } else if (m_.compact) {
    hints_.set({{"A", "Play"}, {"B", "Settings"}, {"X", "Consoles"}, {arrows, "Console"}});
  } else {
    hints_.set({{"A", "Play"}, {"B", "Settings"}, {"X", "Consoles"}, {arrows, "Console"}}, cfg_.version);
  }
}

void Launcher::select(int index) {
  if (index < 0 || index >= (int)games_.size()) return;
  if (!shown(index)) {
    filter_.reset();
    selected_ = index;
    build_list();
    update_console_header();
    return;
  }
  selected_ = index;
  show_selected();
  scroll_to_selected(true);
}

std::vector<std::string> Launcher::consoles() const {
  std::vector<std::string> out;
  for (const auto &g : games_) {
    if (std::find(out.begin(), out.end(), g.console_key) == out.end()) out.push_back(g.console_key);
  }
  return out;
}

std::optional<std::string> Launcher::filter() const { return filter_; }

bool Launcher::set_filter(std::optional<std::string> key) {
  if (key && std::none_of(games_.begin(), games_.end(), [&](const Game &g) { return g.console_key == *key; })) {
    return false;
  }
  close_picker();
  filter_ = std::move(key);
  build_list(); // (the selected game stays if the filter shows it)
  update_console_header();
  return true;
}

void Launcher::step_console(int dir) {
  const auto present = consoles();
  if (present.size() < 2 || selected_ < 0) return;
  const auto &current = games_[selected_].console_key;
  const int n = present.size();
  const int at = std::find(present.begin(), present.end(), current) - present.begin();
  const auto &next = present[(at + (dir < 0 ? n - 1 : 1)) % n];
  if (filter_) {
    set_filter(next);
    return;
  }
  for (size_t i = 0; i < games_.size(); i++) {
    if (games_[i].console_key == next) {
      select(i);
      return;
    }
  }
}

void Launcher::play() {
  if (selected_ < 0 || !play_enabled_) return;
  if (cfg_.on_play) cfg_.on_play(selected_);
}

void Launcher::set_battery(int percent, bool charging) {
  lv_label_set_text(battery_, battery_text(percent, charging).c_str());
}

void Launcher::set_play_enabled(bool on) {
  play_enabled_ = on;
  if (play_btn_) {
    if (on) lv_obj_remove_state(play_btn_, LV_STATE_DISABLED);
    else lv_obj_add_state(play_btn_, LV_STATE_DISABLED);
  }
  show_selected();
}

void Launcher::press(uint32_t key) {
  if (settings_) {
    if (key == LV_KEY_ESC || key == LV_KEY_HOME) close_settings();
  } else if (picker_) {
    if (key == LV_KEY_ESC || key == LV_KEY_HOME) close_picker();
  } else {
    handle_list_key(key);
  }
}

void Launcher::handle_list_key(uint32_t key) {
  switch (key) {
  case LV_KEY_UP:
  case LV_KEY_PREV:
  case LV_KEY_DOWN:
  case LV_KEY_NEXT: {
    if (selected_ < 0) return;
    const int dir = (key == LV_KEY_UP || key == LV_KEY_PREV) ? -1 : 1;
    const int n = games_.size();
    int i = selected_;
    do {
      i = (i + dir + n) % n;
    } while (!shown(i) && i != selected_);
    selected_ = i;
    show_selected();
    scroll_to_selected(true);
    break;
  }
  case LV_KEY_LEFT:
  case LV_KEY_RIGHT:
    step_console(key == LV_KEY_LEFT ? -1 : 1);
    break;
  case LV_KEY_ENTER:
    play();
    break;
  case LV_KEY_ESC:
    open_settings();
    break;
  case LV_KEY_HOME:
    open_picker();
    break;
  default:
    break;
  }
}

void Launcher::list_event(lv_event_t *e) {
  auto *self = static_cast<Launcher *>(lv_event_get_user_data(e));
  self->handle_list_key(lv_event_get_key(e));
}

void Launcher::row_event(lv_event_t *e) {
  auto *self = static_cast<Launcher *>(lv_event_get_user_data(e));
  if (from_keypad()) return;
  const int i = index_of(lv_event_get_current_target_obj(e));
  // a tap selects; a tap on the selected game plays it
  if (i == self->selected_) self->play();
  else self->select(i);
}

void Launcher::tab_event(lv_event_t *e) {
  auto *self = static_cast<Launcher *>(lv_event_get_user_data(e));
  auto *tab = lv_event_get_current_target_obj(e);
  const int i = lv_obj_get_index(tab);
  const auto present = self->consoles();
  if (i == 0) self->set_filter(std::nullopt);
  else if (i - 1 < (int)present.size()) self->set_filter(present[i - 1]);
}

// The console picker: a tile per console with its count, over the list.
void Launcher::open_picker() {
  if (picker_ || settings_) return;
  const bool compact = m_.compact;
  picker_ = box_create(screen_);
  lv_obj_add_flag(picker_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(picker_, m_.width, m_.height - m_.hint_h);
  lv_obj_set_style_bg_color(picker_, lv_color_black(), 0);
  lv_obj_set_style_bg_color(picker_, rgb(color::background), 0);
  lv_obj_set_style_bg_opa(picker_, compact ? LV_OPA_COVER : LV_OPA_90, 0);
  lv_obj_add_event_cb(picker_, picker_event, LV_EVENT_CLICKED, this);

  auto *title = label_create(picker_, m_.font_title, color::text, "Show games for");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, compact ? 5 : 22);

  const int cols = compact ? 3 : 4;
  const int tw = compact ? 96 : 240, th = compact ? 50 : 112, gap = compact ? 6 : 14;
  picker_grid_ = box_create(picker_);
  lv_obj_add_flag(picker_grid_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(picker_grid_, LV_OBJ_FLAG_USER_1);
  lv_obj_set_scrollbar_mode(picker_grid_, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_width(picker_grid_, cols * tw + (cols - 1) * gap + 2 * (m_.focus_w + 2));
  const int grid_top = compact ? 28 : m_.bar_h + 8;  // (under the title)
  lv_obj_set_height(picker_grid_, m_.height - m_.hint_h - grid_top - (compact ? 4 : 24));
  lv_obj_align(picker_grid_, LV_ALIGN_TOP_MID, 0, grid_top);
  lv_obj_set_flex_flow(picker_grid_, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(picker_grid_, m_.focus_w + 2, 0);
  lv_obj_set_style_pad_row(picker_grid_, gap, 0);
  lv_obj_set_style_pad_column(picker_grid_, gap, 0);
  lv_obj_add_event_cb(picker_grid_, picker_event, LV_EVENT_KEY, this);

  picker_keys_.clear();
  picker_tiles_.clear();
  picker_backdrops_.clear();
  auto add = [&](const std::string &key, const std::string &name, size_t count) {
    const auto &style = console_style(key);
    auto *tile = box_create(picker_grid_);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(tile, tw, th);
    lv_obj_set_style_radius(tile, m_.radius + (compact ? 0 : 4), 0);
    lv_obj_set_style_clip_corner(tile, true, 0);
    lv_obj_set_style_border_color(tile, lv_color_white(), 0);
    lv_obj_set_style_border_opa(tile, LV_OPA_20, 0);
    lv_obj_set_style_border_width(tile, 1, 0);
    lv_obj_set_style_outline_color(tile, rgb(color::focus), LV_STATE_CHECKED);
    lv_obj_set_style_outline_width(tile, m_.focus_w, LV_STATE_CHECKED);
    lv_obj_set_style_outline_pad(tile, 1, LV_STATE_CHECKED);
    auto bd = std::make_unique<Backdrop>();
    bd->create(tile);
    bd->set(key.empty() ? console_style("other") : style);
    picker_backdrops_.push_back(std::move(bd));
    auto *stripe = box_create(tile);
    lv_obj_set_size(stripe, compact ? 22 : 46, compact ? 3 : 6);
    lv_obj_set_style_radius(stripe, 3, 0);
    lv_obj_set_style_bg_color(stripe, rgb(key.empty() ? 0xB0B0B0 : style.accent), 0);
    lv_obj_set_style_bg_opa(stripe, LV_OPA_COVER, 0);
    lv_obj_set_pos(stripe, compact ? 6 : 16, compact ? 6 : 16);
    if (!key.empty()) {
      auto *icon = icon_create(tile, style.shape, style.accent, compact ? 14 : 30);
      lv_obj_align(icon, LV_ALIGN_TOP_RIGHT, compact ? -6 : -16, compact ? 4 : 14);
    }
    auto *nm = label_create(tile, compact ? m_.font_small : m_.font_title, color::text, name.c_str());
    lv_obj_set_width(nm, tw - (compact ? 12 : 32));
    lv_label_set_long_mode(nm, LV_LABEL_LONG_MODE_DOTS);
    auto *ct = label_create(tile, m_.font_small, color::text_dim,
                            (std::to_string(count) + (count == 1 ? " game" : " games")).c_str());
    if (compact) {
      lv_obj_align(nm, LV_ALIGN_BOTTOM_LEFT, 6, -15);
      lv_obj_align(ct, LV_ALIGN_BOTTOM_LEFT, 6, -1);
      lv_obj_add_flag(ct, LV_OBJ_FLAG_HIDDEN); // (no room: the name only)
      lv_obj_align(nm, LV_ALIGN_BOTTOM_LEFT, 6, -5);
    } else {
      lv_obj_align(nm, LV_ALIGN_BOTTOM_LEFT, 16, -36);
      lv_obj_align(ct, LV_ALIGN_BOTTOM_LEFT, 16, -12);
    }
    set_index(tile, picker_tiles_.size());
    lv_obj_add_event_cb(tile, picker_event, LV_EVENT_CLICKED, this);
    picker_keys_.push_back(key);
    picker_tiles_.push_back(tile);
  };
  add("", "All consoles", games_.size());
  for (const auto &key : consoles()) {
    const auto it = std::find_if(games_.begin(), games_.end(), [&](const Game &g) { return g.console_key == key; });
    const auto n = std::count_if(games_.begin(), games_.end(), [&](const Game &g) { return g.console_key == key; });
    add(key, compact ? it->console_short : it->console_name, n);
  }
  int at = 0;
  for (size_t i = 0; i < picker_keys_.size(); i++) {
    if (filter_ ? picker_keys_[i] == *filter_ : picker_keys_[i].empty()) at = i;
  }
  picker_group_ = lv_group_create();
  lv_group_add_obj(picker_group_, picker_grid_);
  picker_move(at);
  attach(picker_group_);
  update_hints();
}

void Launcher::picker_move(int to) {
  const int n = picker_tiles_.size();
  if (n == 0) return;
  picker_index_ = std::clamp(to, 0, n - 1);
  for (int i = 0; i < n; i++) {
    if (i == picker_index_) lv_obj_add_state(picker_tiles_[i], LV_STATE_CHECKED);
    else lv_obj_remove_state(picker_tiles_[i], LV_STATE_CHECKED);
  }
  lv_obj_scroll_to_view(picker_tiles_[picker_index_], LV_ANIM_ON);
}

void Launcher::close_picker() {
  if (!picker_) return;
  lv_group_remove_all_objs(picker_group_);
  lv_group_delete(picker_group_);
  picker_group_ = nullptr;
  // (an event of the picker's own may be running)
  lv_obj_delete_async(picker_);
  picker_ = nullptr;
  picker_grid_ = nullptr;
  picker_tiles_.clear();
  picker_backdrops_.clear();
  attach(group_);
  update_hints();
}

void Launcher::picker_event(lv_event_t *e) {
  auto *self = static_cast<Launcher *>(lv_event_get_user_data(e));
  if (!self->picker_) return;
  auto *target = lv_event_get_current_target_obj(e);
  const auto code = lv_event_get_code(e);
  auto choose = [self](int i) {
    const std::string key = self->picker_keys_[i];
    self->close_picker();
    self->set_filter(key.empty() ? std::nullopt : std::optional<std::string>(key));
  };
  if (code == LV_EVENT_KEY) {
    const int cols = self->m_.compact ? 3 : 4;
    switch (lv_event_get_key(e)) {
    case LV_KEY_LEFT: self->picker_move(self->picker_index_ - 1); break;
    case LV_KEY_RIGHT: self->picker_move(self->picker_index_ + 1); break;
    case LV_KEY_UP: case LV_KEY_PREV: self->picker_move(self->picker_index_ - cols); break;
    case LV_KEY_DOWN: case LV_KEY_NEXT: self->picker_move(self->picker_index_ + cols); break;
    case LV_KEY_ENTER: choose(self->picker_index_); break;
    case LV_KEY_ESC: case LV_KEY_HOME: self->close_picker(); break;
    default: break;
    }
    return;
  }
  if (from_keypad()) return;
  if (target == self->picker_) {
    self->close_picker(); // a tap beside the tiles
    return;
  }
  choose(index_of(target));
}

// The settings: a panel from the right (large) or the whole screen (small).
void Launcher::open_settings() {
  if (settings_ || picker_) return;
  const bool compact = m_.compact;
  settings_ = box_create(screen_);
  lv_obj_set_size(settings_, m_.width, m_.height - m_.hint_h);
  lv_obj_add_flag(settings_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(settings_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(settings_, compact ? LV_OPA_TRANSP : LV_OPA_50, 0);
  lv_obj_add_event_cb(settings_, [](lv_event_t *e) {
    auto *self = static_cast<Launcher *>(lv_event_get_user_data(e));
    if (lv_event_get_target_obj(e) == self->settings_ && !from_keypad()) self->close_settings();
  }, LV_EVENT_CLICKED, this);

  auto *panel = box_create(settings_);
  lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE); // (a tap on it is not a tap beside it)
  const int pw = compact ? m_.width : 460;
  lv_obj_set_size(panel, pw, lv_pct(100));
  lv_obj_align(panel, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(panel, rgb(compact ? color::background : 0x15181E), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(panel, compact ? 4 : 24, 0);
  lv_obj_set_style_pad_row(panel, compact ? 2 : 16, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  if (!compact) {
    lv_obj_set_style_shadow_width(panel, 40, 0);
    lv_obj_set_style_shadow_color(panel, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_50, 0);
  }
  auto *title = label_create(panel, m_.font_title, color::text, LV_SYMBOL_SETTINGS "  Settings");
  lv_obj_set_style_pad_left(title, compact ? 4 : 4, 0);

  settings_rows_ = std::make_unique<RowList>();
  settings_rows_->create(panel, m_);
  lv_obj_set_width(settings_rows_->root(), lv_pct(100));
  lv_obj_set_flex_grow(settings_rows_->root(), 1);
  for (const auto &row : cfg_.settings) settings_rows_->add(row);
  settings_rows_->on_back = [this] { close_settings(); };
  settings_rows_->focus_first();
  attach(settings_rows_->group());
  update_hints();
}

void Launcher::close_settings() {
  if (!settings_) return;
  // (the rows' group goes with the RowList; its objects go after this event)
  lv_group_remove_all_objs(settings_rows_->group());
  attach(group_);
  settings_rows_.reset();
  lv_obj_delete_async(settings_);
  settings_ = nullptr;
  update_hints();
}

void Launcher::refresh_settings() {
  if (settings_rows_) settings_rows_->refresh();
}

} // namespace emu_ui
