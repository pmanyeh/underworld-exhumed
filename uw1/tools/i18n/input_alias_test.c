/* P11.4: input_alias.c's loading and mapping, on a small table written here; no game assets. */
#include "i18n/input_alias.h"
#include <stdio.h>
#include <string.h>

static int failures, checks;
static void expect(int slot, const char *typed, const char *want, int replaced)
{
    char out[64];
    int got = input_alias_map(slot, typed, out, sizeof out);
    checks++;
    if (strcmp(out, want) || got != replaced) {
        failures++;
        printf("FAIL slot %d \"%s\": \"%s\" (%d), wanted \"%s\" (%d)\n", slot, typed, out, got, want, replaced);
    }
}

int main(int argc, char **argv)
{
    char path[1024], out[8];
    FILE *f;
    if (argc != 2) return 2;
    snprintf(path, sizeof path, "%s/aliases.tsv", argv[1]);
    f = fopen(path, "wb");
    if (!f) return 2;
    fputs("slot\tblock\tindex\tsource_hash\ttoken\taliases\n"
          "27\t3611\t24\tx\tTalismans\t聖物|八聖物\n"
          "27\t3611\t32\tx\tvolcano\t火山|火山口\n"
          "113\t3697\t33\tx\tsword\t劍\n"
          "113\t3697\t34\tx\tblade\t劍刃\n"
          "21\t3605\t6\tx\tDeco Morono\t德柯莫羅諾\n"
          "24\t3608\t50\tx\tsseth\t\n", f);
    fclose(f);
    checks++;
    if (input_alias_load(path) || !input_alias_loaded()) { puts("FAIL load"); return 1; }
    expect(27, "聖物", "Talismans", 1);
    expect(27, "八聖物", "Talismans", 1);                 /* the longer alias, not 聖物 inside it */
    expect(27, "把聖物丟進火山口", "Talismans volcano", 2); /* other text is dropped */
    expect(27, "the 聖物", "the Talismans", 1);
    expect(113, "劍刃", "blade", 1);                      /* longest first */
    expect(113, "劍", "sword", 1);
    expect(27, "劍", "", 0);                              /* another conversation's words do not apply */
    expect(21, "德柯莫羅諾", "Deco Morono", 1);
    expect(24, "sseth", "sseth", 0);                      /* typed as shown */
    expect(27, "  lava   pit ", "lava pit", 0);
    expect(27, "不知道", "", 0);
    checks++;
    if (input_alias_map(27, "聖物", out, sizeof out) != 1 || strcmp(out, "Talisma")) { failures++; puts("FAIL truncation"); }
    snprintf(path, sizeof path, "%s/bad.tsv", argv[1]);
    f = fopen(path, "wb");
    if (!f) return 2;
    fputs("slot\tblock\n27\t3611\n", f);
    fclose(f);
    checks++;
    if (!input_alias_load(path) || input_alias_loaded()) { failures++; puts("FAIL malformed table loaded"); }
    input_alias_free();
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
