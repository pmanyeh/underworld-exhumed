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

/* Phase 2 observes the resolved key and retrieves UTF-8 for a diagnostic trace.
   The mutable legacy string is returned unchanged. UTF-8 is not copied into
   fixed game buffers or sent to the legacy renderer. */
char *port_i18n_observe(unsigned block, unsigned index, char *fallback);
int port_i18n_trace_open(const char *path);

#endif
