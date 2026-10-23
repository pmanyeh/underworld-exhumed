/* Layout QA (P10B): each translation laid out by the HD scroll's own code (scroll_text.c),
   measured with a real font (hd_font.c), in the box the game shows it in, at the default
   text style. Input lines: key TAB box (0 the message scroll, 1 the conversation's) TAB
   text with the master's escapes. Output lines: key TAB box TAB status TAB lines TAB pages,
   status ok or error; pages counts the [MORE] waits the text alone causes. No game assets. */
#include "presentation/hd_font.h"
#include "presentation/scroll_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int more_waits;
static int measure(uint32_t cp, void *font) { return hd_scroll_advance(font, cp, 5 * 64); }
static void wait_input(int ticks, void *unused) { (void)unused; if (!ticks) more_waits++; }

/* The master's escapes: \n a line feed, \t a tab, \\ a backslash; anything else kept. */
static void unescape(char *s)
{
    char *d = s;
    for (; *s; s++) {
        if (*s == '\\' && s[1] == 'n') { *d++ = '\n'; s++; }
        else if (*s == '\\' && s[1] == 't') { *d++ = '\t'; s++; }
        else if (*s == '\\' && s[1] == '\\') { *d++ = '\\'; s++; }
        else *d++ = *s;
    }
    *d = 0;
}

int main(int argc, char **argv)
{
    static char line[70000];
    HdFont *font = NULL;
    FILE *in, *out;
    if (argc != 4 || hd_font_init(&font, argv[1])) {
        fprintf(stderr, "usage: layout_qa FONT INPUT OUTPUT\n");
        return 2;
    }
    in = fopen(argv[2], "rb");
    out = fopen(argv[3], "wb");
    if (!in || !out) return 2;
    scroll_text_enable(1);
    scroll_text_start(measure, font, wait_input, NULL);
    scroll_text_screen(4);
    while (fgets(line, sizeof line, in)) {
        char *key = strtok(line, "\t"), *box = strtok(NULL, "\t"), *text = strtok(NULL, "\r\n");
        int which, status;
        if (!key || !box || !text) continue;
        which = atoi(box) ? 1 : 0;
        unescape(text);
        /* the boxes SCROLLIO.C's HD branch gives scroll_text_box: main_scroll, npc_scroll */
        scroll_text_clear(which);
        if (which) scroll_text_box(1, 59, 53, 220, 132, 8, 0x2e);
        else scroll_text_box(0, 15, 169, 306, 199, 8, 0x2e);
        scroll_text_clear(which);
        more_waits = 0;
        status = scroll_text_write(which, text, 1, 0);
        fprintf(out, "%s\t%d\t%s\t%d\t%d\n", key, which, status ? "error" : "ok",
                scroll_text_lines(which) + 1, more_waits);
    }
    fclose(in);
    fclose(out);
    hd_font_shutdown(font);
    return 0;
}
