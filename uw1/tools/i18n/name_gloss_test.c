/* P13: name_gloss.c, built and run by test_name_gloss.py. argv[1]: a scratch folder. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i18n/name_gloss.h"

static int failures;

static void expect(const char *what, const char *text, const char *want)
{
    char *got = name_gloss_apply(text);
    if ((got == NULL) != (want == NULL) || (got && strcmp(got, want))) {
        printf("FAIL %s: got %s, want %s\n", what, got ? got : "(none)", want ? want : "(none)");
        failures++;
    }
    free(got);
}

static void write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");
    if (!f) { printf("cannot write %s\n", path); exit(2); }
    fputs(text, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    char path[1024];
    if (argc < 2) return 2;
    snprintf(path, sizeof path, "%s/names.tsv", argv[1]);

    /* no table: nothing changes */
    expect("no table", "加拉蒙", NULL);
    if (name_gloss_load(NULL) != -1) { printf("FAIL a missing path loads\n"); failures++; }

    write_file(path, "zh_TW\tsource_en\r\n\xE5\x8A\xA0\xE6\x8B\x89\xE8\x92\x99\tGaramon\r\n"
                     "\xE5\x87\xB1\xE5\xB8\x83\xE5\x8B\x92\xE6\x96\xAF\tCabirus\n"
                     "\xE5\x87\xB1\xE5\xB8\x83\xE5\x8B\x92\xE6\x96\xAF\xE7\x88\xB5\xE5\xA3\xAB\tSir Cabirus\n");
    if (name_gloss_load(path) || name_gloss_count() != 3) { printf("FAIL load\n"); return 1; }

    name_gloss_mode(NAME_GLOSS_FIRST);
    expect("first time", "加拉蒙說：加拉蒙。", "加拉蒙（Garamon）說：加拉蒙。");
    expect("seen in the same conversation", "加拉蒙。", NULL);
    name_gloss_reset();
    expect("a new conversation", "是加拉蒙", "是加拉蒙（Garamon）");
    expect("longest name first; the shorter is the same man",
           "凱布勒斯爵士與凱布勒斯", "凱布勒斯爵士（Sir Cabirus）與凱布勒斯");
    name_gloss_reset();
    expect("the shorter first, then the longer", "凱布勒斯，凱布勒斯爵士",
           "凱布勒斯（Cabirus），凱布勒斯爵士（Sir Cabirus）");
    name_gloss_reset();
    expect("English already there", "加拉蒙（Garamon）與加拉蒙", NULL);
    expect("English text", "Garamon\\m", NULL);
    expect("escapes and other text kept", "\\P你好，加拉蒙\\m\n", NULL);
    name_gloss_reset();
    expect("escapes and other text kept", "\\P你好，加拉蒙\\m\n", "\\P你好，加拉蒙（Garamon）\\m\n");

    name_gloss_mode(NAME_GLOSS_EVERY);
    expect("every time", "加拉蒙、加拉蒙", "加拉蒙（Garamon）、加拉蒙（Garamon）");
    name_gloss_mode(NAME_GLOSS_OFF);
    name_gloss_reset();
    expect("off", "加拉蒙", NULL);
    name_gloss_mode(99);
    expect("a bad mode is the default", "加拉蒙", "加拉蒙（Garamon）");

    /* a long text grows as it must */
    {
        static char big[20000], want[40000];
        int i;
        big[0] = want[0] = 0;
        for (i = 0; i < 1000; i++) { strcat(big, "加拉蒙，"); strcat(want, "加拉蒙（Garamon），"); }
        name_gloss_mode(NAME_GLOSS_EVERY);
        expect("a long text", big, want);
    }

    write_file(path, "zh_TW\tsource_en\nno tab here\n");
    if (name_gloss_load(path) != -1 || name_gloss_count() != 0) { printf("FAIL a bad table loads\n"); failures++; }
    expect("bad table: nothing changes", "加拉蒙", NULL);

    printf(failures ? "%d failures\n" : "OK\n", failures);
    return failures != 0;
}
