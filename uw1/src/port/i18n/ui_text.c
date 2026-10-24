#include "i18n/ui_text.h"
#include "i18n/localization.h"
#include "i18n/phrases.h"
#include <stddef.h>

const char *port_ui_text(const char *english)
{
    const char *p, *zh;
    if (!english || !port_i18n_enabled()) return NULL;
    for (p = english; *p; p++)
        if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) break;
    if (!*p) return NULL;
    if ((zh = port_i18n_named(english)) != NULL) return zh;
    if ((zh = port_i18n_display(english)) != english) return zh;
    return port_zh_literal(english);
}
