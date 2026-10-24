/* UW1-owned localization. No SDL/platform types and no DOS dependency.
   Init/shutdown and observe/trace belong to one game thread. Never reload or
   free a catalog while readers are running. After init, lookup is read-only;
   returned translations are immutable and valid until shutdown, while a miss
   returns the caller's fallback pointer with its original lifetime. */
#ifndef UW1_LOCALIZATION_H
#define UW1_LOCALIZATION_H

int port_i18n_init(const char *locale, const char *catalog_path);
int port_i18n_enabled(void);
const char *port_i18n_lookup(unsigned block, unsigned index, const char *fallback);
void port_i18n_shutdown(void);
/* P12: the translation of (block, index) without the English to check it against, for a line
   the port composes itself; NULL when the catalog has none. */
const char *port_i18n_static(unsigned block, unsigned index);

/* Phase 2 observes the resolved key and retrieves UTF-8 for a diagnostic trace.
   The mutable legacy string is returned unchanged. UTF-8 is not copied into
   fixed game buffers or sent to the legacy renderer. */
char *port_i18n_observe(unsigned block, unsigned index, char *fallback);
/* Display-only provenance for the eight decoded ring slots. No gameplay string
   replacement. Templates containing @ retain their expanded English fallback. */
const char *port_i18n_display(const char *legacy);
/* P12: as port_i18n_display, but legacy may since have been upper-cased in place; NULL on a miss. */
const char *port_i18n_display_nocase(const char *legacy);
/* P11.1: a template's translation (with its @-variables), for display-only expansion
   (BABL.C's port_convert_display); NULL when legacy has none. */
const char *port_i18n_template(const char *legacy);
/* P11.2: get_name's English output and its name's translation, the last eight, so that the
   name printed by itself shows translated (port_i18n_named: the translation, else NULL). */
void port_i18n_note_name(const char *english, const char *translated);
const char *port_i18n_named(const char *english);
void port_i18n_forget(const char *legacy);
int port_i18n_trace_open(const char *path);
/* Phase 9 QA trace (JSONL, game thread): every resolved lookup as hit, template
   (translated but displayed through the English BABL expansion), miss, drift
   (catalog source differs), invalid (unresolved/out-of-range key), and dynamic
   (made-block strings, which bypass static lookup). Open after init. */
int port_i18n_qa_open(const char *path);
void port_i18n_note_dynamic(unsigned block, unsigned index);

#endif
