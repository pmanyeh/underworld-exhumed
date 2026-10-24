/* P12: the translation of a string the game draws on its bitmap UI (string_to_screen). */
#ifndef UW1_UI_TEXT_H
#define UW1_UI_TEXT_H
/* english as the game passes it: a name get_name just wrote (port_i18n_named), a string just
   decoded (port_i18n_display), or a piece of the game's code (port_zh_literal); NULL when it
   has none, or has no letters (numbers stay in the bitmap font) */
const char *port_ui_text(const char *english);
/* The words in the statistics page's art, as HD patches (STATS.C's RedispStat). */
void port_stats_labels(void);
#endif
