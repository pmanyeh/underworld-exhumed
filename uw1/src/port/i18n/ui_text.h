/* P12: the translation of a string the game draws on its bitmap UI (string_to_screen). */
#ifndef UW1_UI_TEXT_H
#define UW1_UI_TEXT_H
/* english as the game passes it: a name get_name just wrote (port_i18n_named), a string just
   decoded (port_i18n_display), or a piece of the game's code (port_zh_literal); NULL when it
   has none, or has no letters (numbers stay in the bitmap font) */
const char *port_ui_text(const char *english);
/* Main menu button i (0..3) in Chinese over the menu's background, its colour the picture's
   letters' (the selected picture's when it is selected); 1: the caller does not show the
   picture. x, y, w, h as the menu shows it (y bottom-up, the top row). */
int port_menu_button(int i, int x, int y, int w, int h, const unsigned char *img, int selected);
/* The options panel (OPTIONS.C), after it shows OPTBTNS.GR picture pic: as the panel's
   background (0 the icons, 1..5 a page's), as the button of row row, or as the detail page's
   title; its words as HD patches. */
void port_options_back(int pic);
void port_options_btn(int row, int pic);
void port_options_title(int pic);
/* A line the game composes itself: when it next draws english, the label shows zh (once). */
void port_ui_next(const char *english, const char *zh);
/* The end of the game's statistics (SKILLS.C's game_stats): the level and class line and the
   days line, composed in Chinese for the English line about to be drawn. */
void port_end_level(const char *english, int level, int pclass);
void port_end_days(const char *english, int days);
/* The words in the statistics page's art, as HD patches (STATS.C's RedispStat). */
void port_stats_labels(void);
#endif
