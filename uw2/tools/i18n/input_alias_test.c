/* UW2's input-aliases.tsv through the runtime's input_alias.c: each typed Chinese word maps to the
   word its conversation compares, and only in its own conversation (tools/i18n/test_input_alias.py). */
#include "i18n/input_alias.h"
#include <stdio.h>
#include <string.h>

static int fails;
static void want(int slot, const char *typed, const char *expect)
{
    char out[64];
    input_alias_map(slot, typed, out, sizeof out);
    if (strcmp(out, expect)) { printf("FAIL slot %d '%s' -> '%s' (want '%s')\n", slot, typed, out, expect); fails++; }
}

int main(int argc, char **argv)
{
    if (argc < 2 || input_alias_load(argv[1])) { puts("FAIL load"); return 1; }
    want(17, "\xe7\x83\xbd\xe7\x81\xab", "beacon");                       /* 烽火 */
    want(45, "\xe7\x95\xa2\xe5\xa4\x8f\xe6\x99\xae", "bishop");           /* 畢夏普 */
    want(96, "\xe8\x87\xb4\xe5\x91\xbd\xe8\xbf\xbd\xe8\xb9\xa4\xe5\xbd\x88\xe6\xac\x8a\xe6\x9d\x96", "deadly seeker"); /* 致命追蹤彈權杖 */
    want(105, "\xe5\xa4\xa7\xe9\x86\xab\xe7\x99\x82\xe8\xa1\x93", "greater heal");  /* 大醫療術 */
    want(105, "\xe5\xb0\x8f\xe9\x86\xab\xe7\x99\x82\xe8\xa1\x93", "lesser heal");   /* 小醫療術 */
    want(105, "\xe8\x80\x90\xe7\x81\xab", "flameproof");                 /* 耐火 */
    want(130, "\xe7\x8e\x8b\xe5\xba\xa7\xe5\xbb\xb3", "throne");          /* 王座廳 */
    want(130, "\xe5\x87\x8c\xe6\x99\xa8\xe5\x9b\x9b\xe9\xbb\x9e", "four"); /* 凌晨四點 */
    want(9, "xyzzy", "xyzzy");
    want(17, "\xe7\x8e\x8b\xe5\xba\xa7", "");                               /* another slot's word: nothing */
    printf(fails ? "FAILED\n" : "PASS\n");
    return fails != 0;
}
