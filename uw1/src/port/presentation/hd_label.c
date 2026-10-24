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
static int base_em64;

static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }
static void publish(void) { work.generation++; lock(); published = work; unlock(); }

void hd_label_enable(int on) { enabled = on != 0; }
int hd_label_enabled(void) { return enabled && measure && read_pixel; }
void hd_label_start(HdLabelMeasure m, void *context, int em64, HdLabelPixel pixel)
{
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
void hd_label_read(HdLabelFrame *out) { if (out) { lock(); *out = published; unlock(); } }

/* FNV-1a over the box's pixels, row by row */
static uint32_t mix(uint32_t h, int v) { return (h ^ (uint32_t)(v & 255)) * 16777619u; }

int hd_label_put(const char *text, int x, int y, int english_width, int height, int color,
                 int clip_left, int clip_right)
{
    uint32_t cps[HD_LABEL_CELLS];
    int adv[HD_LABEL_CELLS], n = 0, i, width = 0, em64, room, start, top, align = next_align;
    size_t len, off = 0;
    HdLabel *l, box;
    next_align = HD_LABEL_LEFT;
    if (!hd_label_enabled() || !text || height <= 0) return -1;
    len = strlen(text);
    if (!utf8_validate(text, len, NULL)) return -1;
    while (n < HD_LABEL_CELLS) {
        uint32_t cp;
        int r = utf8_next(text, len, &off, &cp);
        if (r == 0) break;
        if (r < 0 || cp == '\n') return -1;
        cps[n] = cp;
        adv[n] = measure(cp, measure_context);
        if (adv[n] < 0) return -1;
        width += adv[n++];
    }
    if (off < len) return -1;                           /* longer than a label holds */
    if (clip_left < 0) clip_left = 0;
    if (clip_right > 319) clip_right = 319;
    room = (align == HD_LABEL_LEFT ? clip_right + 1 - x
            : align == HD_LABEL_RIGHT ? x + english_width - clip_left
            : 2 * ((x + english_width / 2 - clip_left) < (clip_right + 1 - x - english_width / 2)
                   ? x + english_width / 2 - clip_left : clip_right + 1 - x - english_width / 2)) * 64;
    em64 = base_em64;
    if (width > room && room > 0) {                     /* the measure is linear in em */
        em64 = (int)((long long)base_em64 * room / width);
        if (em64 < 3 * 64) return -1;
        for (i = 0, width = 0; i < n; i++) width += adv[i] = (int)((long long)adv[i] * em64 / base_em64);
    }
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
    for (i = 0; i < work.count; ) {     /* a label put over another's box replaces it */
        HdLabel *o = &work.labels[i];
        if (o->left < box.right && box.left < o->right && o->top < box.bottom && box.top < o->bottom) {
            memmove(o, o + 1, sizeof *o * (size_t)(work.count - i - 1));
            work.count--;
        } else i++;
    }
    if (work.count == HD_LABELS) { memmove(work.labels, work.labels + 1, sizeof work.labels[0] * (HD_LABELS - 1)); work.count--; }
    l = &work.labels[work.count];
    *l = box;
    {
        uint32_t h = 2166136261u;
        int px, py;
        for (py = l->top; py < l->bottom; py++)
            for (px = l->left; px < l->right; px++) h = mix(h, read_pixel(px, py));
        l->signature = h;
    }
    l->color = color;
    l->em64 = em64;
    l->y64 = top * 64 + (height * 64 - em64) / 2;
    for (i = 0, l->count = 0; i < n; i++) {
        if (cps[i] != ' ') { l->cells[l->count].scalar = cps[i]; l->cells[l->count].x64 = start; l->count++; }
        start += adv[i];
    }
    work.count++;
    publish();
    return 0;
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
