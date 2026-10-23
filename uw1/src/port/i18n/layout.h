/* Pure, reentrant logical layout. Input bytes are borrowed only during the call;
   output owns units/lines, not text. Keep original text if byte spans are used.
   Every input byte belongs to exactly one scalar/newline/control unit.
   No rendering, control execution, font loading or global state. */
#ifndef UW1_LAYOUT_H
#define UW1_LAYOUT_H
#include <stddef.h>
#include <stdint.h>

enum { LAYOUT_GLYPH, LAYOUT_NEWLINE, LAYOUT_CONTROL };
enum { LAYOUT_SPACE = 1, LAYOUT_CJK = 2, LAYOUT_NO_START = 4,
       LAYOUT_NO_END = 8, LAYOUT_TRIMMED = 16 };
enum { LAYOUT_HARD_BREAK = 1, LAYOUT_OVERFLOW = 2 };
enum { LAYOUT_OK = 0, LAYOUT_BAD_TEXT = -1, LAYOUT_BAD_ARGUMENT = -2,
       LAYOUT_NO_MEMORY = -3, LAYOUT_BAD_WIDTH = -4 };

struct layout_unit {
    size_t byte_offset, byte_length;
    uint32_t scalar;          /* Control: ASCII escape letter/digit. Newline: LF. */
    int advance;              /* Callback width, zero for control/newline. */
    unsigned kind, flags;
    unsigned break_before;   /* Internal break metadata, not a renderer contract. */
};

struct layout_line {
    size_t unit_start, unit_count;
    int64_t width;            /* Excludes TRIMMED edge spaces and controls. */
    unsigned flags;
};

struct text_layout {
    struct layout_unit *units;
    size_t unit_count;
    struct layout_line *lines;
    size_t line_count;
};

/* Return nonnegative advance in caller-selected pixel/fixed-point units.
   max_width uses the same units. Called exactly once per glyph (including
   ASCII space/tab), never for game escapes/newlines. Negative means failure. */
typedef int (*layout_measure_fn)(uint32_t scalar, void *context);

/* out must be zero-initialized/empty; failures leave it empty. A nonempty out
   is rejected without modifying it. Text is limited to 1 MiB and must be
   strict UTF-8 without NUL. Empty text produces one empty line. ASCII edge
   spaces/tabs are retained as units but marked TRIMMED. Opening/closing
   punctuation stays together even when that requires flagged OVERFLOW.
   Basic codepoint wrapping, not full grapheme segmentation/shaping/UAX #14. */
int layout_text(const char *text, size_t length, int max_width,
                layout_measure_fn measure, void *context,
                struct text_layout *out, size_t *error_offset);
void layout_free(struct text_layout *layout);
/* Same contract; controls=0 treats backslashes literally (e.g. saved names). */
int layout_text_controls(const char *text, size_t length, int max_width,
                         layout_measure_fn measure, void *context, int controls,
                         struct text_layout *out, size_t *error_offset);
/* The same, with letter added after each glyph but a space and latin_extra more where a
   Latin letter or digit meets a Chinese character (both in the measure's units, may be
   negative; an advance never goes below zero). */
int layout_text_spaced(const char *text, size_t length, int max_width,
                       layout_measure_fn measure, void *context, int controls,
                       int letter, int latin_extra, struct text_layout *out, size_t *error_offset);
/* 1 when a and b, in that order, meet across the Chinese/Latin boundary. */
int layout_latin_boundary(uint32_t a, uint32_t b);

#endif
