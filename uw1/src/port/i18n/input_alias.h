/* P11.4: words the player types in a conversation (babl_ask) in the locale's own language,
   turned into the English word the script compares before the script sees them. The table is
   the locale's input-aliases.tsv: slot, block, index, source_hash, token, aliases ("|" between).
   No SDL/platform types; one game thread. The script, its comparisons and saves are unchanged:
   only ASCII leaves input_alias_map. */
#ifndef UW1_INPUT_ALIAS_H
#define UW1_INPUT_ALIAS_H
#include <stddef.h>

/* Loads path (replacing any table before); 0 on success, -1 when unreadable or malformed. */
int input_alias_load(const char *path);
int input_alias_loaded(void);
void input_alias_free(void);
/* text (UTF-8) typed in conversation slot: each alias of that slot found in it becomes its
   token (the longest first), then anything not ASCII is dropped and white space collapsed.
   out gets at most n - 1 bytes and a 0. Returns the number of aliases replaced. */
int input_alias_map(int slot, const char *text, char *out, size_t n);
#endif
