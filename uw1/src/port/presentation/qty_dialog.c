#include "presentation/hd_font.h"
#include "qty_dialog.h"
#include "i18n/phrases.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

static int enabled;
static void (*send_key)(uint8_t scancode);
/* the game's side: open, the stack's count and the entry's starting count; a new opening has
   a new serial */
static atomic_int open_max, open_initial, open_serial;
/* the backend's side; untouched until the player changes the count, when the first digit
   replaces it as it does the entry's starting text */
static int seen_serial, value = 1, untouched, dragging;

/* The panel, over the message scroll (game pixels, top-down): the prompt, the slider and the
   count on its first row, the buttons on its second */
enum { PX = 14, PY = 166, PW = 292, PH = 31, SX = 104, SY = 170, SW = 140, SH = 6 };
enum { B_MINUS, B_PLUS, B_HALF, B_ALL, B_OK, B_CANCEL, BUTTONS };
static const char *const button_key[BUTTONS] = { "qty.minus", "qty.plus", "qty.half", "qty.all", "qty.ok", "qty.cancel" };
static int button_x(int b) { return PX + 6 + b * 47; }
enum { BY = 182, BW = 43, BH = 12 };

void qty_dialog_enable(int on, void (*key)(uint8_t))
{
    enabled = on && key;
    send_key = key;
}

void qty_dialog_begin(int max, int initial)
{
    if (!enabled) return;
    max = max < 1 ? 1 : max > 999 ? 999 : max;
    atomic_store(&open_initial, initial < 1 ? 1 : initial > max ? max : initial);
    atomic_store(&open_max, max);
    atomic_fetch_add(&open_serial, 1);
}
void qty_dialog_end(void) { if (enabled) atomic_store(&open_max, 0); }
int qty_dialog_open(void) { return enabled && atomic_load(&open_max) > 0; }

static int active(void)
{
    int serial = atomic_load(&open_serial);
    if (!enabled || atomic_load(&open_max) <= 0) return 0;
    if (serial != seen_serial) {        /* the game's entry starts at 1, or the whole stack (split-all) */
        seen_serial = serial;
        value = atomic_load(&open_initial);
        untouched = 1;
        dragging = 0;
    }
    return 1;
}

static void tap(uint8_t sc) { send_key(sc); send_key((uint8_t)(sc | 0x80)); }

/* the entry made to say v: its three characters rubbed out, then v's digits */
static void type_value(int v)
{
    static const uint8_t digit[10] = { 0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    char text[8];
    int i;
    for (i = 0; i < 3; i++) tap(0x0E);
    snprintf(text, sizeof text, "%d", v);
    for (i = 0; text[i]; i++) tap(digit[text[i] - '0']);
}

static void set_value(int v)
{
    int max = atomic_load(&open_max);
    if (v < 1) v = 1;
    if (v > max) v = max;
    untouched = 0;
    if (v == value) return;
    value = v;
    type_value(v);
}

static void slide(float x)
{
    int max = atomic_load(&open_max);
    float t = (x - SX) / SW;
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    set_value(1 + (int)(t * (max - 1) + 0.5f));
}

int qty_dialog_pointer(const PlatPointer *ev)
{
    float x = ev->x, y = ev->y;
    int inside, b;
    if (!active()) return 0;
    inside = ev->absolute && x >= PX && x < PX + PW && y >= PY && y < PY + PH;
    if (ev->type == PLAT_POINTER_UP) {
        int was = dragging;
        dragging = 0;
        return was || inside;
    }
    if (ev->type == PLAT_POINTER_MOVE) {
        if (dragging && (ev->buttons & PLAT_BUTTON_LEFT)) slide(x);
        return 0;       /* the game's cursor follows the pointer */
    }
    if (!inside) return 0;                  /* a click elsewhere is the game's (1, or all) */
    if (ev->button != PLAT_BUTTON_LEFT) return 1;
    if (y >= SY - 3 && y < SY + SH + 3 && x >= SX - 4 && x < SX + SW + 4) {
        dragging = 1;
        slide(x);
        return 1;
    }
    for (b = 0; b < BUTTONS; b++)
        if (x >= button_x(b) && x < button_x(b) + BW && y >= BY && y < BY + BH) {
            int max = atomic_load(&open_max);
            switch (b) {
            case B_MINUS: set_value(value - 1); break;
            case B_PLUS: set_value(value + 1); break;
            case B_HALF: set_value((max + 1) / 2); break;
            case B_ALL: set_value(max); break;
            case B_OK: tap(0x1C); break;
            case B_CANCEL: tap(0x01); break;
            }
        }
    return 1;
}

/* the player's digits and Backspace go to the count, which then types itself into the entry */
int qty_dialog_key(uint8_t sc)
{
    static const signed char digit_of[0x0C] = { -1, -1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    uint8_t make = (uint8_t)(sc & 0x7F);
    if (!active()) return 0;
    if (make >= 0x02 && make <= 0x0B) {
        if (!(sc & 0x80)) {
            int v = value * 10 + digit_of[make];
            int max = atomic_load(&open_max);
            if ((value == 1 || untouched) && !dragging) v = digit_of[make];   /* the first digit replaces the 1 */
            set_value(v > max ? max : v);
        }
        return 1;
    }
    if (make == 0x0E) {
        if (!(sc & 0x80)) set_value(value / 10);
        return 1;
    }
    return 0;
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

static float label(HdFont *font, const PlatOverlay *f, const char *s, float x, float y, float em, HdColor c, int centre, float w)
{
    HdTextMetrics m;
    if (centre && hd_text_measure_logical(font, f, s, strlen(s), em, 0, &m) == HD_OK) x += (w - m.width) / 2;
    hd_text_draw(font, f, s, strlen(s), x, y, em, 0, c);
    return hd_text_measure_logical(font, f, s, strlen(s), em, 0, &m) == HD_OK ? m.width : 0;
}

int qty_dialog_draw(HdFont *font, const PlatOverlay *f)
{
    static const HdColor back = { 0x5A, 0x43, 0x2C, 255 }, edge = { 0x2A, 0x1E, 0x12, 255 },
                         ink = { 0xF0, 0xE2, 0xBE, 255 }, gold = { 0xF2, 0xC6, 0x6D, 255 },
                         well = { 0x2A, 0x23, 0x18, 255 }, face = { 0x7A, 0x5C, 0x3A, 255 };
    char count[24];
    int max, b;
    float filled;
    if (!font || !active()) return 0;
    max = atomic_load(&open_max);
    box(f, PX - 1, PY - 1, PW + 2, PH + 2, edge);
    box(f, PX, PY, PW, PH, back);
    label(font, f, port_zh_lit("qty.prompt"), PX + 6, PY + 2, 6, ink, 0, 0);
    box(f, SX, SY, SW, SH, well);
    filled = max > 1 ? (float)(value - 1) / (max - 1) : 1;
    box(f, SX, SY, SW * filled, SH, gold);
    box(f, SX + SW * filled - 1.5f, SY - 2, 3, SH + 4, ink);
    snprintf(count, sizeof count, "%d / %d", value, max);
    label(font, f, count, SX + SW + 8, PY + 2, 6, gold, 0, 0);
    for (b = 0; b < BUTTONS; b++) {
        box(f, button_x(b), BY, BW, BH, b == B_OK ? gold : face);
        label(font, f, port_zh_lit(button_key[b]), button_x(b), BY + 3, 5.5f, b == B_OK ? edge : ink, 1, BW);
    }
    return 1;
}
