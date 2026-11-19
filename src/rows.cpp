#include "emu-ui/rows.hpp"

#include <algorithm>

#include "emu-ui/widgets.hpp"

namespace emu_ui {

void RowList::create(lv_obj_t *parent, const Metrics &m, lv_group_t *group) {
  m_ = &m;
  root_ = box_create(parent);
  lv_obj_add_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(root_, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(root_, m.gap, 0);
  // (room for the focus ring inside the scrolling area)
  lv_obj_set_style_pad_all(root_, m.focus_w + 1, 0);
  if (group) {
    group_ = group;
  } else {
    group_ = lv_group_create();
    lv_group_set_wrap(group_, true);
    own_group_ = true;
  }
}

RowList::~RowList() {
  if (own_group_ && group_) {
    lv_group_delete(group_);
  }
}

int RowList::add(Row row) {
  const int index = items_.size();
  Item item;
  item.row = std::move(row);
  const auto &r = item.row;
  const bool compact = m_->compact;

  auto *obj = box_create(root_);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  // (every arrow comes to the row as a key: see the boards' keypads)
  lv_obj_add_flag(obj, LV_OBJ_FLAG_USER_1);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_width(obj, lv_pct(100));
  lv_obj_set_height(obj, LV_SIZE_CONTENT);
  lv_obj_set_style_min_height(obj, m_->row_h, 0);
  lv_obj_set_style_radius(obj, m_->radius, 0);
  lv_obj_set_style_bg_color(obj, rgb(color::surface), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
  lv_obj_set_style_bg_color(obj, rgb(color::surface_high), LV_STATE_FOCUSED);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_STATE_FOCUSED);
  lv_obj_set_style_pad_hor(obj, compact ? 6 : 16, 0);
  lv_obj_set_style_pad_ver(obj, compact ? 2 : 6, 0);
  lv_obj_set_style_pad_row(obj, 0, 0);
  add_focus_ring(obj, *m_);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  // the label and the value on one line, a note (if any) under them
  auto *line = box_create(obj);
  lv_obj_set_size(line, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(line, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(line, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(line, compact ? 6 : 12, 0);
  std::string text = r.symbol ? std::string(r.symbol) + "  " + r.label : r.label;
  auto *label = label_create(line, m_->font_body, r.danger ? color::danger : color::text, text.c_str());
  // (grows into the room the value leaves, one line, cut with dots)
  lv_obj_set_size(label, 1, lv_font_get_line_height(m_->font_body));
  lv_obj_set_flex_grow(label, 1);
  lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);

  switch (r.kind) {
  case Row::Kind::SLIDER: {
    item.slider = lv_slider_create(line);
    lv_obj_set_width(item.slider, lv_pct(compact ? 40 : 42));
    lv_obj_set_height(item.slider, compact ? 6 : 10);
    lv_slider_set_range(item.slider, r.min, r.max);
    lv_obj_remove_flag(item.slider, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_pad_all(item.slider, compact ? 3 : 8, LV_PART_KNOB);
    lv_obj_set_style_margin_right(item.slider, compact ? 4 : 10, 0);
    lv_obj_add_event_cb(item.slider, event_cb, LV_EVENT_VALUE_CHANGED, this);
    break;
  }
  case Row::Kind::TOGGLE: {
    item.sw = lv_switch_create(line);
    lv_obj_set_size(item.sw, compact ? 34 : 60, compact ? 18 : 32);
    lv_obj_remove_flag(item.sw, LV_OBJ_FLAG_CLICKABLE); // (a tap on the row flips it)
    break;
  }
  case Row::Kind::CHOICE:
  case Row::Kind::NUMBER: {
    item.value = label_create(line, m_->font_body, color::text, "");
    break;
  }
  case Row::Kind::ACTION:
    break;
  }
  if (r.note) {
    item.note = label_create(obj, m_->font_small, color::text_dim, "");
    lv_obj_set_width(item.note, lv_pct(100));
    lv_label_set_long_mode(item.note, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_pad_bottom(item.note, compact ? 2 : 6, 0);
  }

  lv_obj_set_user_data(obj, reinterpret_cast<void *>(static_cast<intptr_t>(index)));
  lv_obj_add_event_cb(obj, event_cb, LV_EVENT_KEY, this);
  lv_obj_add_event_cb(obj, event_cb, LV_EVENT_CLICKED, this);
  lv_group_add_obj(group_, obj);
  item.obj = obj;
  items_.push_back(std::move(item));
  show(index);
  return index;
}

void RowList::add_heading(const char *text) {
  auto *l = label_create(root_, m_->font_small, color::text_dim, text);
  lv_obj_set_style_pad_top(l, m_->compact ? 4 : 14, 0);
  lv_obj_set_style_pad_left(l, m_->compact ? 4 : 8, 0);
}

lv_obj_t *RowList::row_obj(int index) const {
  return index >= 0 && index < (int)items_.size() ? items_[index].obj : nullptr;
}

void RowList::refresh() {
  for (size_t i = 0; i < items_.size(); i++) show(i);
}

void RowList::refresh(int index) { show(index); }

void RowList::focus(int index) {
  if (auto *o = row_obj(index)) lv_group_focus_obj(o);
}

void RowList::focus_first() { focus(0); }

void RowList::show(int index) {
  auto &it = items_[index];
  const auto &r = it.row;
  const int v = r.get ? r.get() : 0;
  switch (r.kind) {
  case Row::Kind::SLIDER:
    lv_slider_set_value(it.slider, v, LV_ANIM_OFF);
    break;
  case Row::Kind::TOGGLE:
    if (v) lv_obj_add_state(it.sw, LV_STATE_CHECKED);
    else lv_obj_remove_state(it.sw, LV_STATE_CHECKED);
    break;
  case Row::Kind::CHOICE: {
    const auto &c = r.choices;
    const std::string text = c.empty() ? "" : LV_SYMBOL_LEFT " " + c[std::clamp(v, 0, (int)c.size() - 1)] + " " LV_SYMBOL_RIGHT;
    lv_label_set_text(it.value, text.c_str());
    break;
  }
  case Row::Kind::NUMBER:
    lv_label_set_text_fmt(it.value, LV_SYMBOL_LEFT " %d " LV_SYMBOL_RIGHT, v);
    break;
  case Row::Kind::ACTION:
    break;
  }
  if (it.note) {
    const auto text = r.note();
    lv_label_set_text(it.note, text.c_str());
    if (text.empty()) lv_obj_add_flag(it.note, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(it.note, LV_OBJ_FLAG_HIDDEN);
  }
}

void RowList::change(int index, int dir) {
  auto &r = items_[index].row;
  if (!r.get || !r.set) return;
  const int v = r.get();
  int next = v;
  switch (r.kind) {
  case Row::Kind::SLIDER:
    next = std::clamp(v + dir * r.step, r.min, r.max);
    break;
  case Row::Kind::NUMBER:
    next = v + dir * r.step;
    if (next > r.max) next = r.min;
    if (next < r.min) next = r.max;
    break;
  case Row::Kind::TOGGLE:
    next = v ? 0 : 1;
    break;
  case Row::Kind::CHOICE: {
    const int n = r.choices.size();
    if (n == 0) return;
    next = (v + dir + n) % n;
    break;
  }
  case Row::Kind::ACTION:
    return;
  }
  if (next != v) r.set(next);
  show(index);
}

void RowList::activate(int index) {
  auto &r = items_[index].row;
  switch (r.kind) {
  case Row::Kind::ACTION:
    if (r.set) r.set(0);
    break;
  case Row::Kind::TOGGLE:
  case Row::Kind::CHOICE:
    change(index, 1);
    break;
  case Row::Kind::SLIDER:
  case Row::Kind::NUMBER:
    break;
  }
}

void RowList::handle_key(int index, uint32_t key) {
  switch (key) {
  case LV_KEY_UP:
  case LV_KEY_DOWN: {
    const int dir = key == LV_KEY_UP ? -1 : 1;
    const int last = items_.size() - 1;
    if ((index == 0 && dir < 0) || (index == last && dir > 0)) {
      if (on_edge && on_edge(dir)) return;
    }
    if (dir < 0) lv_group_focus_prev(group_);
    else lv_group_focus_next(group_);
    break;
  }
  case LV_KEY_LEFT:
  case LV_KEY_RIGHT:
    change(index, key == LV_KEY_LEFT ? -1 : 1);
    break;
  case LV_KEY_ENTER:
    activate(index);
    break;
  case LV_KEY_ESC:
    if (on_back) on_back();
    break;
  default:
    break;
  }
}

void RowList::event_cb(lv_event_t *e) {
  auto *self = static_cast<RowList *>(lv_event_get_user_data(e));
  auto *target = lv_event_get_current_target_obj(e);
  const auto code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    // a slider dragged by touch
    for (size_t i = 0; i < self->items_.size(); i++) {
      auto &it = self->items_[i];
      if (it.slider == target && it.row.set) {
        it.row.set(lv_slider_get_value(it.slider));
        return;
      }
    }
    return;
  }
  const int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(target)));
  if (index < 0 || index >= (int)self->items_.size()) return;
  if (code == LV_EVENT_KEY) {
    self->handle_key(index, lv_event_get_key(e));
  } else if (code == LV_EVENT_CLICKED) {
    // (the keypad's A came as LV_KEY_ENTER already)
    auto *indev = lv_indev_active();
    if (indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) return;
    lv_group_focus_obj(target);
    self->activate(index);
  }
}

} // namespace emu_ui
