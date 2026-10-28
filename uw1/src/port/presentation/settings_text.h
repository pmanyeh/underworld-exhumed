/* P12: the settings screen in the HD font (settings_text.c). */
#ifndef UW1_SETTINGS_TEXT_H
#define UW1_SETTINGS_TEXT_H
#include <stdint.h>
struct HdFont;
/* Before the screen is first drawn: font (the presentation thread's; NULL: the screen's own
   bitmap font only), whether the screen is in Chinese, and the HD text's antialiasing now. */
void settings_text_start(struct HdFont *font, int chinese, int smooth);
/* whether the screen is in Chinese (a font, and the game in zh-TW) */
int settings_text_chinese(void);
/* The Language tab's sample (settab.c's rows' preview) */
void port_text_preview(uint32_t *px, int x, int y, int w, int h, int value);
#endif
