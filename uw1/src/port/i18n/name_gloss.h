/* P13: proper names shown with their English the first time a conversation or a book names
   them, "加拉蒙（Garamon）", for the player to match the original and its guides. The names are
   i18n/<locale>/proper-names.tsv (tools/i18n/proper_names.py: the authority's people, places
   and organizations). Display only: the text the game keeps is never changed. */
#ifndef UW1_NAME_GLOSS_H
#define UW1_NAME_GLOSS_H

#define NAME_GLOSS_OFF 0
#define NAME_GLOSS_FIRST 1              /* the first time in a conversation or a book */
#define NAME_GLOSS_EVERY 2

/* path: a header line, then zh<TAB>English a line. 0 when read; -1 leaves no names. */
int name_gloss_load(const char *path);
void name_gloss_free(void);
int name_gloss_count(void);
void name_gloss_mode(int mode);
/* a new conversation or book: every name is new again */
void name_gloss_reset(void);
/* text with the English after each name due one, as a new string the caller frees; NULL when
   nothing is added (no names, off, or none due) */
char *name_gloss_apply(const char *text);

#endif
