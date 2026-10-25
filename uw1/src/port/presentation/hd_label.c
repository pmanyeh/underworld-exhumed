#include "hd_label.h"
#include "i18n/utf8.h"
#include <stdatomic.h>
#include <string.h>

static int enabled, next_align;
static HdLabelFrame work, published;
static atomic_flag busy = ATOMIC_FLAG_INIT;
static HdLabelMeasure measure;
static void *measure_context;
static HdLabelPixel read_pixel;
static HdLabelPage draw_page;
static int base_em64;

static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }
static void publish(void) { work.generation++; lock(); published = work; unlock(); }
/* FNV-1a over the box's pixels, row by row */
static uint32_t mix(uint32_t h, int v) { return (h ^ (uint32_t)(v & 255)) * 16777619u; }
/* signs l with the drawing page under its box, keeping the pixels when the box is small */
static void sign(HdLabel *l)
{
    uint32_t h = 2166136261u;
    int px, py, keep = (l->right - l->left) * (l->bottom - l->top) <= HD_LABEL_UNDER, k = 0;
    for (py = l->top; py < l->bottom; py++)
        for (px = l->left; px < l->right; px++) {
            int v = read_pixel(px, py);
            h = mix(h, v);
            if (keep) l->under[k++] = (uint8_t)v;
        }
    l->signature = h;
    l->kept = keep;
}

void hd_label_enable(int on) { enabled = on != 0; }
int hd_label_enabled(void) { return enabled && measure && read_pixel; }
void hd_label_start(HdLabelMeasure m, void *context, int em64, HdLabelPixel pixel, HdLabelPage page)
{
    draw_page = page;
    memset(&work, 0, sizeof work);
    measure = m; measure_context = context; read_pixel = pixel;
    base_em64 = em64 > 0 ? em64 : 5 * 64;
    publish();
}
void hd_label_align(int align) { next_align = align; }
void hd_label_clear(void)
{
    if (!hd_label_enabled() || !work.count) return;
    work.count = 0; publish();
}
void hd_label_forget(int left, int top, int right, int bottom)
{
    int i, gone = 0, page;
    if (!hd_label_enabled()) return;
    page = draw_page ? draw_page() : 0;
    for (i = 0; i < work.count; ) {
        HdLabel *o = &work.labels[i];
        if (o->page == page && o->left < right && left < o->right && o->top < bottom && top < o->bottom) {
            memmove(o, o + 1, sizeof *o * (size_t)(work.count - i - 1));
            work.count--;
            gone = 1;
        } else i++;
    }
    if (gone) publish();
}
void hd_label_resign(int left, int top, int right, int bottom)
{
    int i, changed = 0, page;
    if (!hd_label_enabled()) return;
    page = draw_page ? draw_page() : 0;
    for (i = 0; i < work.count; i++) {
        HdLabel *o = &work.labels[i];
        if (o->page == page && o->left < right && left < o->right && o->top < bottom && top < o->bottom) {
            sign(o);
            changed = 1;
        }
    }
    if (changed) publish();
}
void hd_label_copy(int left, int top, int right, int bottom, int from)
{
    int i, n, page;
    if (!hd_label_enabled()) return;
    page = draw_page ? draw_page() : 0;
    if (page == from) return;
    for (i = 0, n = work.count; i < n && work.count < HD_LABELS; i++) {
        HdLabel *o = &work.labels[i], *l;
        if (o->page != from || o->left < left || o->right > right || o->top < top || o->bottom > bottom) continue;
        l = &work.labels[work.count++];
        *l = *o;
        l->page = page;
        sign(l);
    }
    if (work.count != n) publish();
}
void hd_label_read(HdLabelFrame *out) { if (out) { lock(); *out = published; unlock(); } }


/* text's scalars and their advances at the base em; the count, or -1 (invalid, a newline,
   longer than a label holds) */
static int shape(const char *text, uint32_t *cps, int *adv, int *width)
{
    size_t len = strlen(text), off = 0;
    int n = 0;
    if (!utf8_validate(text, len, NULL)) return -1;
    *width = 0;
    while (n < HD_LABEL_CELLS) {
        uint32_t cp;
        int r = utf8_next(text, len, &off, &cp);
        if (r == 0) break;
        if (r < 0 || cp == '\n') return -1;
        cps[n] = cp;
        adv[n] = measure(cp, measure_context);
        if (adv[n] < 0) return -1;
        *width += adv[n++];
    }
    return off < len ? -1 : n;
}

/* the advances (measured at the base em) at em64, shrunk to fit room (the measure is linear in
   em); the em, or 0 below 3 pixels */
static int fit(int *adv, int n, int *width, int room, int em64)
{
    int i, want = *width;
    if (room > 0 && (long long)want * em64 / base_em64 > room) em64 = (int)((long long)base_em64 * room / want);
    if (em64 < 3 * 64) return 0;
    for (i = 0, *width = 0; i < n; i++) *width += adv[i] = (int)((long long)adv[i] * em64 / base_em64);
    return em64;
}

/* the label in box, signed with the screen under it now: put over another's box, it replaces
   that; the oldest goes when full */
static void add(const HdLabel *box, const uint32_t *cps, const int *adv, int n, int start, int y64, int em64, int color)
{
    HdLabel *l;
    int i;
    for (i = 0; i < work.count; ) {
        HdLabel *o = &work.labels[i];
        if (o->left < box->right && box->left < o->right && o->top < box->bottom && box->top < o->bottom) {
            memmove(o, o + 1, sizeof *o * (size_t)(work.count - i - 1));
            work.count--;
        } else i++;
    }
    if (work.count == HD_LABELS) { memmove(work.labels, work.labels + 1, sizeof work.labels[0] * (HD_LABELS - 1)); work.count--; }
    l = &work.labels[work.count];
    *l = *box;
    sign(l);
    l->page = draw_page ? draw_page() : 0;
    l->color = color;
    l->em64 = em64;
    l->y64 = y64;
    for (i = 0, l->count = 0; i < n; i++) {
        if (cps[i] != ' ') { l->cells[l->count].scalar = cps[i]; l->cells[l->count].x64 = start; l->count++; }
        start += adv[i];
    }
    work.count++;
    publish();
}

int hd_label_put(const char *text, int x, int y, int english_width, int height, int color,
                 int clip_left, int clip_right)
{
    uint32_t cps[HD_LABEL_CELLS];
    int adv[HD_LABEL_CELLS], n, width, em64, room, start, top, align = next_align;
    HdLabel box;
    next_align = HD_LABEL_LEFT;
    if (!hd_label_enabled() || !text || height <= 0 || (n = shape(text, cps, adv, &width)) < 0) return -1;
    if (clip_left < 0) clip_left = 0;
    if (clip_right > 319) clip_right = 319;
    if (align == HD_LABEL_LEFT) room = clip_right + 1 - x;
    else if (align == HD_LABEL_RIGHT) room = x + english_width - clip_left;
    else {
        int centre = x + english_width / 2, l = centre - clip_left, r = clip_right + 1 - centre;
        room = 2 * (l < r ? l : r);
    }
    /* the base em, or four fifths of a bigger font's height (fontbig) */
    em64 = height * 64 * 4 / 5 > base_em64 ? height * 64 * 4 / 5 : base_em64;
    if (!(em64 = fit(adv, n, &width, room * 64, em64))) return -1;
    start = align == HD_LABEL_LEFT ? x * 64
            : align == HD_LABEL_RIGHT ? (x + english_width) * 64 - width
            : (x * 64 + english_width * 32) - width / 2;
    top = 199 - y;
    memset(&box, 0, sizeof box);
    box.left = start / 64 < x ? start / 64 : x;
    box.right = (start + width + 63) / 64 > x + english_width ? (start + width + 63) / 64 : x + english_width;
    if (box.left < clip_left) box.left = clip_left;
    if (box.right > clip_right + 1) box.right = clip_right + 1;
    box.top = top < 0 ? 0 : top;
    box.bottom = top + height > 200 ? 200 : top + height;
    if (box.left >= box.right || box.top >= box.bottom) return -1;
    box.fill = -1;
    box.border = -1;
    add(&box, cps, adv, n, start, top * 64 + (height * 64 - em64) / 2, em64, color);
    return 0;
}

int hd_label_build(HdLabel *out, const char *text, int x, int y, int height, int color,
                   int clip_right, int em64)
{
    uint32_t cps[HD_LABEL_CELLS];
    int adv[HD_LABEL_CELLS], n, width, i, pos = x * 64;
    if (!out || !hd_label_enabled() || !text || x < 0 || x > 319 || height <= 0 ||
        (n = shape(text, cps, adv, &width)) <= 0) return -1;
    if (clip_right > 319) clip_right = 319;
    if (clip_right < x || !(em64 = fit(adv, n, &width, (clip_right + 1 - x) * 64, em64))) return -1;
    memset(out, 0, sizeof *out);
    out->left = x; out->right = x + (width + 63) / 64;
    out->top = 199 - y; out->bottom = out->top + height;
    if (out->top < 0 || out->bottom > 200) return -1;
    out->fill = -1; out->border = 0;
    out->em64 = em64; out->y64 = out->top * 64 + (height * 64 - em64) / 2;
    out->color = color; out->page = draw_page ? draw_page() : 0;
    for (i = 0; i < n; i++) {
        out->cells[i].scalar = cps[i]; out->cells[i].x64 = pos;
        pos += adv[i];
    }
    out->count = n;
    sign(out);
    return 0;
}

void hd_label_refresh(HdLabel *label) { if (label && hd_label_enabled()) sign(label); }

int hd_label_patch(const char *text, int left, int top, int right, int bottom, int color, int align)
{
    return hd_label_box(text, left, top, right, bottom, color, align, 0, HD_LABEL_COMMONEST, -1);
}

int hd_label_box(const char *text, int left, int top, int right, int bottom, int color, int align,
                 int em, int patch, float border)
{
    static int counts[256];
    uint32_t cps[HD_LABEL_CELLS];
    int adv[HD_LABEL_CELLS], n, i, width, em64, x, y, best = 0;
    HdLabel box;
    if (!hd_label_enabled() || !text || (n = shape(text, cps, adv, &width)) < 0) return -1;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > 320) right = 320;
    if (bottom > 200) bottom = 200;
    if (left >= right || top >= bottom || !(em64 = fit(adv, n, &width, (right - left) * 64, em > 0 ? em : base_em64))) return -1;
    memset(&box, 0, sizeof box);
    box.left = left; box.top = top; box.right = right; box.bottom = bottom;
    /* the fill: the box's commonest colour, the art's background under its words */
    if (patch == HD_LABEL_COMMONEST) {
        memset(counts, 0, sizeof counts);
        for (y = top; y < bottom; y++)
            for (x = left; x < right; x++) counts[read_pixel(x, y) & 255]++;
        for (i = 1; i < 256; i++) if (counts[i] > counts[best]) best = i;
    }
    box.fill = patch == HD_LABEL_COMMONEST ? best : patch >= 0 ? (patch & 255) : -1;
    box.border = border;
    x = align == HD_LABEL_LEFT ? left * 64 : align == HD_LABEL_RIGHT ? right * 64 - width : (left + right) * 32 - width / 2;
    add(&box, cps, adv, n, x, (top + bottom) * 32 - em64 / 2, em64, color);
    return 0;
}

int hd_label_shows(const uint8_t *pixels, int width, int height, const HdLabel *l, const int cursor[4])
{
    int px, py, k = 0;
    if (!pixels || l->bottom > height || l->right > width) return 0;
    if (!l->kept || !cursor || cursor[0] >= l->right || cursor[2] <= l->left || cursor[1] >= l->bottom || cursor[3] <= l->top)
        return hd_label_signature(pixels, width, height, l) == l->signature;
    for (py = l->top; py < l->bottom; py++)
        for (px = l->left; px < l->right; px++, k++)
            if (pixels[py * width + px] != l->under[k] &&
                !(px >= cursor[0] && px < cursor[2] && py >= cursor[1] && py < cursor[3])) return 0;
    return 1;
}

uint32_t hd_label_signature(const uint8_t *pixels, int width, int height, const HdLabel *l)
{
    uint32_t h = 2166136261u;
    int px, py;
    if (!pixels || l->bottom > height || l->right > width) return h ^ 1;
    for (py = l->top; py < l->bottom; py++)
        for (px = l->left; px < l->right; px++) h = mix(h, pixels[py * width + px]);
    return h;
}
