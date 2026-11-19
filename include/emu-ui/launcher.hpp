#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lvgl.h"

#include "emu-ui/rows.hpp"
#include "emu-ui/theme.hpp"
#include "emu-ui/widgets.hpp"

namespace emu_ui {

/// A game in the launcher's list.
struct Game {
  std::string title{};
  std::string subtitle{};      ///< under the title ("2 saves"), may be empty
  std::string console_key{};   ///< rom_info's ConsoleInfo::key: its style and tab
  std::string console_name{};  ///< "Super Nintendo"
  std::string console_short{}; ///< "SNES" (small screens)
};

/// The game launcher: the games grouped by console over the console's
/// background, the selected game's cover and title, a console picker and a
/// settings panel. On a large screen the consoles are tabs along the top
/// and the settings slide in from the right; on a small one the console is
/// a chip in the status bar and the settings fill the screen.
///
/// Keys (from the board's keypad group): up / down the list, left / right
/// the console, ENTER (A) play, ESC (B) settings, HOME (X) the console
/// picker. Touch: a tap selects a game, a tap on the selected one or on
/// Play plays it, a tap on a console tab / the chip filters / picks.
class Launcher {
public:
  struct Config {
    Metrics metrics{Metrics::large_1280x720()};
    std::string brand{};            ///< the status bar's name (large screens)
    lv_indev_t *keypad{nullptr};    ///< gets the launcher's groups
    /// The image source (lv_draw_buf_t* / lv_image_dsc_t*) of game `index`'s
    /// cover, valid until the next call; nullptr for none
    std::function<const void *(size_t index)> cover{};
    /// Optional art for a console (boxart/consoles/<key>.jpg), valid until
    /// the next call; nullptr: the firmware's badge
    std::function<const void *(const std::string &key)> console_art{};
    std::function<void(size_t index)> on_play{};
    std::function<void(size_t index)> on_select{};
    /// The settings panel's rows
    std::vector<Row> settings{};
    std::string version{}; ///< bottom right on large screens
  };

  explicit Launcher(Config config);
  ~Launcher();

  lv_obj_t *screen() const { return screen_; }
  /// Load the screen and give the keypad its group (after a game)
  void show();

  /// The games, grouped by console in the order given; `selected` stays
  /// selected if shown (else the first game shown)
  void set_games(std::vector<Game> games, int selected);
  const std::vector<Game> &games() const { return games_; }
  int selected() const { return selected_; }
  /// Select game `index` (a game the filter hides lifts the filter)
  void select(int index);

  /// The consoles that have games, in list order (their keys)
  std::vector<std::string> consoles() const;
  /// The console whose games the list shows, if filtered
  std::optional<std::string> filter() const;
  /// Show only `key`'s games (nullopt: all); false if it has none
  bool set_filter(std::optional<std::string> key);
  /// The previous / next console: its first game, or the filter moves there
  void step_console(int dir);

  void open_picker();
  void close_picker();
  bool picker_open() const { return picker_ != nullptr; }
  void open_settings();
  void close_settings();
  bool settings_open() const { return settings_ != nullptr; }
  /// Re-read the settings' values (changed elsewhere: the USB link, a key)
  void refresh_settings();

  /// -1: not known yet
  void set_battery(int percent, bool charging);
  /// Off while the SD card is a USB drive (the games are not there)
  void set_play_enabled(bool on);
  /// A key the keypad does not deliver (the box's X is polled): LV_KEY_*
  void press(uint32_t key);

private:
  void build_ui();
  void build_list();
  void build_tabs();
  void show_selected();
  void update_hints();
  void update_console_header();
  void scroll_to_selected(bool anim);
  void attach(lv_group_t *group);
  bool shown(size_t index) const;
  void play();
  void handle_list_key(uint32_t key);
  static void list_event(lv_event_t *e);
  static void row_event(lv_event_t *e);
  static void tab_event(lv_event_t *e);
  static void picker_event(lv_event_t *e);

  Config cfg_;
  const Metrics &m_;
  std::vector<Game> games_;
  int selected_{-1};
  std::optional<std::string> filter_;
  bool play_enabled_{true};

  lv_obj_t *screen_{nullptr};
  Backdrop backdrop_;
  lv_obj_t *bar_{nullptr}, *tabs_{nullptr}, *battery_{nullptr}, *chip_area_{nullptr};
  lv_obj_t *list_{nullptr};
  std::vector<lv_obj_t *> rows_; ///< per game; nullptr while filtered out
  lv_obj_t *cover_{nullptr}, *system_{nullptr}, *title_{nullptr}, *subtitle_{nullptr};
  lv_obj_t *play_btn_{nullptr}, *settings_btn_{nullptr};
  HintBar hints_;
  lv_group_t *group_{nullptr}; ///< the list

  // the console picker, while open: its tiles' consoles ("" = all)
  lv_obj_t *picker_{nullptr};
  lv_obj_t *picker_grid_{nullptr};
  lv_group_t *picker_group_{nullptr};
  std::vector<std::string> picker_keys_;
  std::vector<lv_obj_t *> picker_tiles_;
  std::vector<std::unique_ptr<Backdrop>> picker_backdrops_;
  int picker_index_{0};
  void picker_move(int to);

  // the settings panel, while open
  lv_obj_t *settings_{nullptr};
  std::unique_ptr<RowList> settings_rows_;
};

} // namespace emu_ui
