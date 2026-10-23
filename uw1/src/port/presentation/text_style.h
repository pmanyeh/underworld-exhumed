/* text_style: the HD Chinese text parameters (P10T), named as in the Ultima VIII translation's
   scummvm.ini: font_cjk_hd_<item> for every surface, and font_cjk_<surface>_<item> for one
   (message: the message scroll, with the conversation's choices; talk: the conversation's own
   box; book: books and scrolls, not drawn in HD yet). Sizes and spacings are in game pixels
   (the screen is 320 wide), fractions allowed.

   They come, each over the one before: the defaults here, the locale's settings file
   (i18n/<locale>/text-settings.ini), the player's settings file (uw1port.cfg in the home
   folder), the command line (--font). Both files are lines of key=value, the last of a key
   winning, # starting a comment line. No game, SDL or FreeType types. */
#ifndef UW1_TEXT_STYLE_H
#define UW1_TEXT_STYLE_H
#include <stddef.h>

#define TEXT_STYLE_DEFAULT_FONT "jf-openhuninn-2.1.ttf"

typedef struct HdTextStyle {
    char file[512];                     /* a font file's name (searched for) or path */
    char fallback_file[512];            /* one for the characters file lacks; "" none */
    double size;                        /* the em; default 5 */
    double letter_spacing;              /* after each glyph but a space; default 0 */
    double latin_spacing;               /* more where Latin meets Chinese; default 0 */
    double line_spacing;                /* more between lines; default 0 */
    double baseline;                    /* the glyphs moved down (up when negative), drawing only; default 0 */
    double border;                      /* a black outline's width, 0 none; default 0 */
    int antialiasing;                   /* default 1 */
} HdTextStyle;

enum { TEXT_STYLE_MESSAGE, TEXT_STYLE_TALK, TEXT_STYLE_BOOK, TEXT_STYLE_SURFACES };

/* The defaults. */
void text_style_defaults(HdTextStyle *style);
/* PATH's values over *style: surface -1 the font_cjk_hd_ keys, else the surface's own. A
   value out of range or unreadable is reported on stderr (with PATH) and left as it was. A
   missing file changes nothing. Returns the number of values taken. */
int text_style_load(const char *path, int surface, HdTextStyle *style);
/* The font file to open for NAME: a path (with a slash, a backslash or a colon) as it is,
   else the first of HOME/fonts, fonts and tools/fonts that has it. 0 when found. */
int text_style_font_path(const char *home, const char *name, char *out, size_t outsz);
/* The locale's settings file: the first of HOME/i18n/<locale>/text-settings.ini and
   i18n/<locale>/text-settings.ini that exists, else "". */
void text_style_locale_file(const char *home, const char *locale, char *out, size_t outsz);
#endif
