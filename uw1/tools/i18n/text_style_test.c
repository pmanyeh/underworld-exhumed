/* P10T: the HD text settings, read from files in a scratch folder (argv[1]); no game assets,
   no FreeType. */
#include "presentation/text_style.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0777)
#endif
static int checks;
#define CHECK(c) do {checks++;if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);}}while(0)
static char dir[512];

static const char *write_file(const char *name, const char *text)
{
    static char path[2][1024];
    static int k;
    FILE *f;
    k ^= 1;
    snprintf(path[k], sizeof path[k], "%s/%s", dir, name);
    f = fopen(path[k], "wb");
    CHECK(f);
    fputs(text, f);
    fclose(f);
    return path[k];
}

int main(int argc, char **argv)
{
    HdTextStyle s, t;
    char out[1024], path[1024];
    const char *locale, *player;
    CHECK(argc == 2);
    snprintf(dir, sizeof dir, "%s", argv[1]);

    /* the defaults: today's look, the U8 translation's font */
    text_style_defaults(&s);
    CHECK(!strcmp(s.file, "jf-openhuninn-2.1.ttf") && !s.fallback_file[0] && s.size == 5);
    CHECK(!s.letter_spacing && !s.latin_spacing && !s.line_spacing && !s.baseline && !s.border && s.antialiasing);

    /* no file, an empty one: nothing changes */
    t = s;
    CHECK(text_style_load(NULL, -1, &t) == 0 && !memcmp(&t, &s, sizeof s));
    CHECK(text_style_load("", -1, &t) == 0 && !memcmp(&t, &s, sizeof s));
    snprintf(path, sizeof path, "%s/absent.cfg", dir);
    CHECK(text_style_load(path, -1, &t) == 0 && !memcmp(&t, &s, sizeof s));
    CHECK(text_style_load(write_file("empty.cfg", ""), -1, &t) == 0 && !memcmp(&t, &s, sizeof s));

    /* every key, the U8 names; comments and other settings ignored; CRLF; the last line wins */
    locale = write_file("locale.cfg",
        "# a comment\r\nvolume=50\r\nfont_cjk_hd_file=A.ttf\r\nfont_cjk_hd_fallback_file=B.otf\r\n"
        "font_cjk_hd_size=6\r\nfont_cjk_hd_size=6.5\r\nfont_cjk_hd_letter_spacing=0.3\r\n"
        "font_cjk_hd_latin_spacing=1\r\nfont_cjk_hd_line_spacing=-1.5\r\nfont_cjk_hd_baseline=0.25\r\n"
        "font_cjk_hd_border=0.5\r\nfont_cjk_hd_antialiasing=false\r\n"
        "font_cjk_talk_size=4\r\nfont_cjk_book_size=7\r\n");
    t = s;
    CHECK(text_style_load(locale, -1, &t) == 9);
    CHECK(!strcmp(t.file, "A.ttf") && !strcmp(t.fallback_file, "B.otf") && t.size == 6.5);
    CHECK(t.letter_spacing == 0.3 && t.latin_spacing == 1 && t.line_spacing == -1.5 && t.baseline == 0.25);
    CHECK(t.border == 0.5 && !t.antialiasing);
    /* a surface's own keys over the shared ones, the others untouched */
    CHECK(text_style_load(locale, TEXT_STYLE_TALK, &t) == 1 && t.size == 4 && t.letter_spacing == 0.3);
    CHECK(text_style_load(locale, TEXT_STYLE_MESSAGE, &t) == 0 && t.size == 4);
    CHECK(text_style_load(locale, TEXT_STYLE_BOOK, &t) == 1 && t.size == 7);

    /* the player's file over the locale's, as main applies them */
    player = write_file("uw1port.cfg", "font_cjk_hd_size=8\nfont_cjk_talk_antialiasing=true\n");
    t = s;
    text_style_load(locale, -1, &t); text_style_load(locale, TEXT_STYLE_TALK, &t);
    text_style_load(player, -1, &t); text_style_load(player, TEXT_STYLE_TALK, &t);
    CHECK(t.size == 8 && t.antialiasing && !strcmp(t.file, "A.ttf"));

    /* bad values: reported, the value before kept, the rest still read */
    t = s;
    CHECK(text_style_load(write_file("bad.cfg",
        "font_cjk_hd_size=big\nfont_cjk_hd_letter_spacing=1x\nfont_cjk_hd_line_spacing=nan\n"
        "font_cjk_hd_border=3\nfont_cjk_hd_size=1\nfont_cjk_hd_baseline=-9\nfont_cjk_hd_antialiasing=maybe\n"
        "font_cjk_hd_file=\nfont_cjk_hd_latin_spacing= 2 \n"), -1, &t) == 1);   /* spaces around a number are fine */
    CHECK(t.size == 5 && !t.letter_spacing && !t.line_spacing && !t.border && !t.baseline && t.antialiasing);
    CHECK(!strcmp(t.file, "jf-openhuninn-2.1.ttf") && t.latin_spacing == 2);
    /* the edges of each range are allowed */
    CHECK(text_style_load(write_file("edges.cfg",
        "font_cjk_hd_size=16\nfont_cjk_hd_letter_spacing=-2\nfont_cjk_hd_line_spacing=16\n"
        "font_cjk_hd_border=2\nfont_cjk_hd_baseline=-4\n"), -1, &t) == 5);
    CHECK(t.size == 16 && t.letter_spacing == -2 && t.line_spacing == 16 && t.border == 2 && t.baseline == -4);

    /* fonts: a path as it is; a name from HOME/fonts first, then fonts, then tools/fonts */
    snprintf(path, sizeof path, "%s/fonts", dir); MKDIR(path);
    write_file("fonts/F.ttf", "x");
    CHECK(text_style_font_path(dir, "F.ttf", out, sizeof out) == 0);
    snprintf(path, sizeof path, "%s/fonts/F.ttf", dir); CHECK(!strcmp(out, path));
    CHECK(text_style_font_path(dir, path, out, sizeof out) == 0 && !strcmp(out, path));
    CHECK(text_style_font_path(dir, "missing.ttf", out, sizeof out) == -1);
    CHECK(text_style_font_path(dir, "", out, sizeof out) == -1);
    snprintf(path, sizeof path, "%s/none/F.ttf", dir);
    CHECK(text_style_font_path(dir, path, out, sizeof out) == -1);

    /* the locale's file: HOME/i18n/<locale>/text-settings.ini, else none here */
    text_style_locale_file(dir, "xx-YY", out, sizeof out);
    CHECK(out[0] == 0);
    snprintf(path, sizeof path, "%s/i18n", dir); MKDIR(path);
    snprintf(path, sizeof path, "%s/i18n/xx-YY", dir); MKDIR(path);
    write_file("i18n/xx-YY/text-settings.ini", "font_cjk_hd_size=9\n");
    text_style_locale_file(dir, "xx-YY", out, sizeof out);
    snprintf(path, sizeof path, "%s/i18n/xx-YY/text-settings.ini", dir);
    CHECK(!strcmp(out, path));
    t = s; CHECK(text_style_load(out, -1, &t) == 1 && t.size == 9);

    printf("PASS %d assertions: defaults, every key, surfaces, file order, bad values and ranges, font search, locale file\n", checks);
    return 0;
}
