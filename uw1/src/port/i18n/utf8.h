/* Length-delimited UTF-8 scalar operations, independent of fonts/platforms.
   U+0000 is valid here; C-string consumers must reject embedded NUL separately.
   next/prev return 1 for a scalar, 0 at the end/start, -1 for invalid arguments
   or sequence. Offset and scalar outputs are unchanged on 0/-1.
   prev validates the immediately preceding sequence, not the whole prefix. */
#ifndef UW1_UTF8_H
#define UW1_UTF8_H
#include <stddef.h>
#include <stdint.h>

int utf8_next(const char *text, size_t length, size_t *offset, uint32_t *scalar);
int utf8_prev(const char *text, size_t length, size_t *offset, uint32_t *scalar);
/* validate: 1/0, error_offset is length on success or first invalid lead byte. */
int utf8_validate(const char *text, size_t length, size_t *error_offset);
/* count: 1/0, count is published only on success. */
int utf8_count(const char *text, size_t length, size_t *count);
/* Last valid scalar boundary <= min(offset,length). Stops at invalid input;
   never reports a boundary beyond a malformed/truncated prefix. O(offset). */
size_t utf8_boundary_before(const char *text, size_t length, size_t offset);

#endif
