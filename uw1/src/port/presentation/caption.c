#include "caption.h"
#include "i18n/utf8.h"
#include <stdatomic.h>
#include <string.h>

static int enabled;
static CaptionFrame work, published;
static atomic_flag busy = ATOMIC_FLAG_INIT;
static CaptionMeasure measure;
static void *measure_context;

static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }
static void publish(void) { work.generation++; lock(); published = work; unlock(); }

void caption_enable(int on) { enabled = on != 0; }
int caption_enabled(void) { return enabled && measure; }
void caption_start(CaptionMeasure m, void *context, int em64)
{
    memset(&work, 0, sizeof work);
    measure = m; measure_context = context;
    work.em64 = em64 > 0 ? em64 : 7 * 64;
    publish();
}
void caption_clear(void)
{
    if (!caption_enabled() || !work.active) return;
    work.active = 0; work.count = 0; publish();
}
int caption_active(void) { return caption_enabled() && work.active; }
void caption_read(CaptionFrame *out) { if (out) { lock(); *out = published; unlock(); } }

/* a line may not start with closing punctuation: it stays on the line before */
static int no_start(uint32_t cp)
{
    static const uint32_t marks[] = {0xFF0C, 0x3002, 0xFF01, 0xFF1F, 0x3001, 0xFF1B, 0xFF1A, 0x300D, 0x300F,
                                     0xFF09, 0x2026, 0x2014, ',', '.', '!', '?', ';', ':', ')'};
    size_t i;
    for (i = 0; i < sizeof marks / sizeof marks[0]; i++) if (marks[i] == cp) return 1;
    return 0;
}

int caption_set(const char *text, int color, int x, int y, int w, int h)
{
    uint32_t cps[CAPTION_CELLS];
    int adv[CAPTION_CELLS], start[CAPTION_LINES + 1], width[CAPTION_LINES];
    int n = 0, lines = 0, line_w = 0, i, k, limit, pitch, bottom, centre;
    size_t len, off = 0;
    if (!caption_enabled() || !text) return -1;
    len = strlen(text);
    if (!utf8_validate(text, len, NULL)) return -1;
    while (n < CAPTION_CELLS) {
        uint32_t cp;
        int r = utf8_next(text, len, &off, &cp);
        if (r == 0) break;
        if (r < 0) return -1;
        if (cp == '\r') continue;
        cps[n] = cp;
        adv[n] = cp == '\n' ? 0 : measure(cp, measure_context);
        if (adv[n] < 0) return -1;
        n++;
    }
    limit = (w > 300 ? 300 : w) * 64;       /* the area, with a margin on the full screen */
    start[0] = 0;
    for (i = 0; i < n; i++) {
        int brk = cps[i] == '\n' || (line_w + adv[i] > limit && line_w > 0 && !no_start(cps[i]));
        if (!brk) { line_w += adv[i]; continue; }
        if (lines + 1 == CAPTION_LINES) break;            /* the last line keeps what fits */
        width[lines++] = line_w;
        start[lines] = cps[i] == '\n' ? i + 1 : i;
        line_w = cps[i] == '\n' ? 0 : adv[i];
    }
    width[lines] = line_w;
    start[++lines] = i;
    pitch = work.em64 * 13 / 10;
    bottom = (199 - (y - h + 1)) * 64 - 2 * 64;   /* top-down 26.6: the area's bottom, as CUTS.C's text sits */
    centre = (x + w) * 32;                        /* CUTS.C centres in (x + w) / 2 */
    work.count = 0;
    for (k = 0; k < lines; k++) {
        int cx = centre - width[k] / 2, top = bottom - (lines - k) * pitch;
        for (i = start[k]; i < start[k + 1] && i < n; i++) {
            if (cps[i] == '\n') continue;
            if (work.count < CAPTION_CELLS && cps[i] != ' ') {
                work.cells[work.count].scalar = cps[i];
                work.cells[work.count].x64 = cx;
                work.cells[work.count].y64 = top;
                work.count++;
            }
            cx += adv[i];
        }
    }
    work.color = color;
    work.active = 1;
    publish();
    return 0;
}
