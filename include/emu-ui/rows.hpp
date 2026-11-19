#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lvgl.h"

#include "emu-ui/theme.hpp"

namespace emu_ui {

/// One row of a settings list or a menu. The board supplies get() / set();
/// the row shows the value and changes it from the keypad or a tap.
struct Row {
  enum class Kind {
    SLIDER, ///< min..max in steps; left / right
    TOGGLE, ///< 0 / 1; A or left / right flips it
    CHOICE, ///< an index into choices; left / right, A for the next one
    NUMBER, ///< min..max shown as a number; left / right
    ACTION, ///< A or a tap runs set(0)
  };
  std::string label;
  Kind kind{Kind::ACTION};
  int min{0}, max{100}, step{10};
  std::vector<std::string> choices;
  std::function<int()> get;
  std::function<void(int)> set;
  /// Text under the row (a hint, a status); re-read by refresh()
  std::function<std::string()> note;
  const char *symbol{nullptr}; ///< an LV_SYMBOL_* before the label
  bool danger{false};          ///< a red action (Quit)
};

/// A scrolling column of Rows in a keypad group: up / down move between
/// them, left / right change the focused one, A acts, B calls on_back.
/// Rows can be added to a group the caller already uses for other objects.
class RowList {
public:
  /// `group`: the keypad group the rows join (made if nullptr)
  void create(lv_obj_t *parent, const Metrics &m, lv_group_t *group = nullptr);
  ~RowList();
  /// Add a row; returns its index
  int add(Row row);
  /// A divider with an optional heading
  void add_heading(const char *text);
  /// Re-read every row's value and note (after a change elsewhere)
  void refresh();
  void refresh(int index);
  void focus(int index);
  void focus_first();
  lv_obj_t *root() const { return root_; }
  lv_group_t *group() const { return group_; }
  /// The row at `index`'s object (to place other things beside it)
  lv_obj_t *row_obj(int index) const;
  std::function<void()> on_back;
  /// Up past the first row or down past the last (false: wrap)
  std::function<bool(int dir)> on_edge;

private:
  struct Item {
    Row row;
    lv_obj_t *obj{nullptr}, *value{nullptr}, *slider{nullptr}, *sw{nullptr}, *note{nullptr};
  };
  static void event_cb(lv_event_t *e);
  void handle_key(int index, uint32_t key);
  void change(int index, int dir);
  void activate(int index);
  void show(int index);

  const Metrics *m_{nullptr};
  lv_obj_t *root_{nullptr};
  lv_group_t *group_{nullptr};
  bool own_group_{false};
  std::vector<Item> items_;
};

} // namespace emu_ui
