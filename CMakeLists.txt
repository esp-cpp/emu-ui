# emu-ui: the launcher and pause menu shared by esp-box-emu and tab5-emu
# (LVGL only; see README.md). tools/preview renders them on a desktop.
idf_component_register(
  INCLUDE_DIRS "include"
  SRCS "src/theme.cpp" "src/widgets.cpp" "src/rows.cpp" "src/launcher.cpp" "src/pause_menu.cpp" "src/saves.cpp"
  REQUIRES lvgl)
