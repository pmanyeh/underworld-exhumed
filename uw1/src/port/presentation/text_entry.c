#include "hd_font.h"
#include "text_entry.h"
#include "i18n/utf8.h"
#include "i18n/user_text.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int enabled;
static void (*send_key)(uint8_t scancode);
static atomic_int open_now;                 /* the game's side opened it */
static atomic_int answered;                 /* the backend has typed the answer */
static atomic_flag busy = ATOMIC_FLAG_INIT;
/* under the lock: the panel and the text */
static int kind, max_chars, px, py, pw, ph;
static char prompt[160], text[160], standin[40], result[160];
static int has_result;

static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }

void text_entry_enable(int on, void (*key)(uint8_t))
{
    enabled = on && key;
    send_key = key;
}

static int chars(const char *s)
{
    int n = 0;
    for (; *s; s++) n += ((unsigned char)*s & 0xC0) != 0x80;
    return n;
}

void text_entry_begin(int k, const char *p, const char *initial, int max, const char *stand, int x, int y, int w, int h)
{
    const char *seed = initial;
    size_t n;
    if (!enabled) return;
    lock();
    kind = k; max_chars = max; px = x; py = y; pw = w; ph = h;
    snprintf(prompt, sizeof prompt, "%s", p ? p : "");
    /* A test's committed text stands in for the OS input method. Apply the same bounds. */
    if (getenv("EXHUME_TEST_CHEATS") && getenv("UW1_TEST_TYPED")) seed = getenv("UW1_TEST_TYPED");
    if (!seed || !utf8_validate(seed, strlen(seed), NULL)) seed = "";
    n = utf8_boundary_before(seed, strlen(seed), sizeof text - 1);
    memcpy(text, seed, n); text[n] = 0;
    while (chars(text) > max_chars) {                  /* an initial text too long: its end goes */
        n = strlen(text);
        while (n && ((unsigned char)text[n - 1] & 0xC0) == 0x80) n--;
        if (n) n--;
        text[n] = 0;
    }
    snprintf(standin, sizeof standin, "%s", stand ? stand : "");
    has_result = 0;
    unlock();
    atomic_store(&answered, 0);
    plat_text_input(1);
    atomic_store(&open_now, 1);
}

const char *text_entry_end(void)
{
    const char *out;
    if (!enabled) return NULL;
    atomic_store(&open_now, 0);
    plat_text_input(0);
    lock(); out = has_result ? result : NULL; unlock();
    return out;
}

int text_entry_open(void) { return enabled && atomic_load(&open_now); }

static void tap(uint8_t sc) { send_key(sc); send_key((uint8_t)(sc | 0x80)); }

/* a printable ASCII character as the PC keyboard types it (US layout, set 1) */
static void type_char(char c)
{
    static const char plain[] = "1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./ ";
    static const char shifted[] = "!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>? ";
    static const uint8_t code[] = { 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
        0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2B,
        0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x39 };
    const char *at;
    if ((at = strchr(plain, c)) != NULL && c) tap(code[at - plain]);
    else if ((at = strchr(shifted, c)) != NULL && c) {
        send_key(0x2A);
        tap(code[at - shifted]);
        send_key(0xAA);
    }
}

/* Enter: the answer typed into the game's entry */
static void answer(void)
{
    const char *typed;
    size_t i;
    lock();
    if (!port_utf8_ascii(text)) { snprintf(result, sizeof result, "%s", text); has_result = 1; typed = standin; }
    else { has_result = 0; typed = text; }
    unlock();
    if (kind == TEXT_ENTRY_LINE) {                      /* wdialog: Ctrl+A to the start, Ctrl+K the rest away */
        send_key(0x1D); tap(0x1E); tap(0x25); send_key(0x9D);
    }
    for (i = 0; typed[i]; i++) type_char(typed[i]);
    tap(0x1C);
    atomic_store(&answered, 1);
}

static void take_typed(void)
{
    char got[64];
    size_t i;
    if (!plat_text_take(got, sizeof got)) return;
    lock();
    for (i = 0; got[i]; ) {
        size_t n = 1, len = strlen(text);
        while (got[i + n] && ((unsigned char)got[i + n] & 0xC0) == 0x80) n++;
        if ((unsigned char)got[i] >= 0x20 && chars(text) < max_chars && len + n < sizeof text) {
            memcpy(text + len, got + i, n);
            text[len + n] = 0;
        }
        i += n;
    }
    unlock();
}

int text_entry_key(uint8_t sc)
{
    uint8_t make = (uint8_t)(sc & 0x7F);
    if (!enabled || !atomic_load(&open_now) || atomic_load(&answered)) return 0;
    if (sc == 0xE0 || sc == 0xE1) return 1;
    take_typed();
    if (sc & 0x80) return 1;
    if (make == 0x0E) {                                 /* Backspace: the last character */
        size_t n;
        lock();
        n = strlen(text);
        while (n && ((unsigned char)text[n - 1] & 0xC0) == 0x80) n--;
        if (n) n--;
        text[n] = 0;
        unlock();
    } else if (make == 0x1C) {
        int empty;
        lock(); empty = !text[0]; unlock();
        if (!empty || kind == TEXT_ENTRY_LINE || kind == TEXT_ENTRY_MAP) answer();
    } else if (make == 0x01) {
        atomic_store(&answered, 1);
        tap(0x01);
    }
    return 1;                                           /* the characters come from the input method */
}

static void box(const PlatOverlay *f, float x, float y, float w, float h, HdColor c)
{
    float sx = f->viewport_w / f->logical_width, sy = f->viewport_h / f->logical_height;
    int x0 = (int)(f->viewport_x + x * sx), y0 = (int)(f->viewport_y + y * sy);
    int x1 = (int)(f->viewport_x + (x + w) * sx + 0.5f), y1 = (int)(f->viewport_y + (y + h) * sy + 0.5f), i, j;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > f->width) x1 = f->width;
    if (y1 > f->height) y1 = f->height;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++) {
            uint8_t *p = f->rgba + (size_t)j * (size_t)f->pitch + (size_t)i * 4;
            p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = 255;
        }
}

int text_entry_draw(HdFont *font, const PlatOverlay *f)
{
    static const HdColor back = { 0x5A, 0x43, 0x2C, 255 }, edge = { 0x2A, 0x1E, 0x12, 255 },
                         ink = { 0xF0, 0xE2, 0xBE, 255 }, gold = { 0xF2, 0xC6, 0x6D, 255 },
                         well = { 0x2A, 0x23, 0x18, 255 };
    char line[200], p[160];
    HdTextMetrics m;
    float em, x, y, w, h, pwid = 0;
    if (!font || !enabled || !atomic_load(&open_now)) return 0;
    take_typed();
    lock();
    x = (float)px; y = (float)py; w = (float)pw; h = (float)ph;
    snprintf(p, sizeof p, "%s", prompt);
    snprintf(line, sizeof line, "%s_", text);
    unlock();
    em = h >= 20 ? 6.0f : 5.5f;
    box(f, x - 1, y - 1, w + 2, h + 2, edge);
    box(f, x, y, w, h, back);
    if (p[0]) {
        hd_text_draw(font, f, p, strlen(p), x + 4, y + (h >= 20 ? 2 : (h - em) / 2), em, 0, ink);
        if (hd_text_measure_logical(font, f, p, strlen(p), em, 0, &m) == HD_OK) pwid = m.width + 6;
    }
    if (h >= 20) {                                      /* two rows: the prompt, then the answer */
        box(f, x + 4, y + h / 2 + 1, w - 8, h / 2 - 3, well);
        hd_text_draw(font, f, line, strlen(line), x + 6, y + h / 2 + 2, em, 0, gold);
    } else {
        box(f, x + 4 + pwid, y + 2, w - 8 - pwid, h - 4, well);
        hd_text_draw(font, f, line, strlen(line), x + 6 + pwid, y + (h - em) / 2, em, 0, gold);
    }
    return 1;
}
