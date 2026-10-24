/* The player's words in Chinese (user_text.c): the stand-ins, the side files, what is shown;
   the game's files are a folder of this test's (bc_fopen below). */
#include "i18n/user_text.h"
#include <stdio.h>
#include <string.h>

static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static char dir[512];

/* "SAVE0\NAME.UTF" -> DIR/SAVE0_NAME.UTF */
static void path_of(const char *game, char *out, size_t n)
{
    char flat[64];
    size_t i;
    snprintf(flat, sizeof flat, "%s", game);
    for (i = 0; flat[i]; i++) if (flat[i] == '\\') flat[i] = '_';
    snprintf(out, n, "%s/%s", dir, flat);
}
FILE *bc_fopen(const char *path, const char *mode) { char p[600]; path_of(path, p, sizeof p); return fopen(p, mode); }
int bc_unlink(const char *path) { char p[600]; path_of(path, p, sizeof p); return remove(p); }
static int exists(const char *game) { char p[600]; FILE *f; path_of(game, p, sizeof p); f = fopen(p, "rb"); if (f) fclose(f); return f != NULL; }
static void put(const char *game, const char *text) { FILE *f = bc_fopen(game, "wb"); fputs(text, f); fclose(f); }

int main(int argc, char **argv)
{
    snprintf(dir, sizeof dir, "%s", argc > 1 ? argv[1] : ".");
    /* the name */
    CHECK(port_user_name() == NULL && port_user_text("Avatar") == NULL);
    port_user_name_set("Avatar");                          /* ASCII: the game keeps it itself */
    CHECK(port_user_name() == NULL);
    port_user_name_set("\xE5\xB0\x8F\xE6\x98\x8E");       /* 小明 */
    CHECK(port_user_name() && !strcmp(port_user_name(), "\xE5\xB0\x8F\xE6\x98\x8E"));
    CHECK(port_user_text("Avatar") && port_user_text("AVATAR") && port_user_text("avatar"));
    CHECK(port_user_text("Avatars") == NULL && port_user_text("Avata") == NULL && port_user_text("") == NULL);
    port_user_name_save();
    CHECK(exists("SAVE0\\NAME.UTF"));
    port_user_name_set(NULL);
    CHECK(port_user_text("Avatar") == NULL);
    port_user_name_load();                                 /* after a restore */
    CHECK(port_user_name() && !strcmp(port_user_name(), "\xE5\xB0\x8F\xE6\x98\x8E"));
    port_user_name_set(NULL);
    port_user_name_save();                                 /* no Chinese name: no file */
    CHECK(!exists("SAVE0\\NAME.UTF"));
    port_user_name_set("\xFF\xFE");                        /* not UTF-8 */
    CHECK(port_user_name() == NULL);
    /* the descriptions */
    CHECK(!strcmp(port_user_desc_standin(1), "Save 1") && !strcmp(port_user_desc_standin(4), "Save 4"));
    port_user_desc_save("\xE5\x9C\xB0\xE5\xBA\x95");      /* 地底, into SAVE0 before the copy */
    CHECK(exists("SAVE0\\DESC.UTF"));
    port_user_desc_save("plain");                          /* an ASCII description: none beside it */
    CHECK(!exists("SAVE0\\DESC.UTF"));
    put("SAVE2\\DESC.UTF", "\xE5\x9C\xB0\xE5\xBA\x95\n");
    put("SAVE3\\DESC.UTF", "\xFF");
    port_user_descs_load();
    CHECK(port_user_text("Save 2") && !strcmp(port_user_text("Save 2"), "\xE5\x9C\xB0\xE5\xBA\x95"));
    CHECK(port_user_text("Save 1") == NULL && port_user_text("Save 3") == NULL && port_user_text("Save 4") == NULL);
    CHECK(port_utf8_ascii("Save 1") && !port_utf8_ascii("\xE5\x9C\xB0"));
    bc_unlink("SAVE2\\DESC.UTF");
    bc_unlink("SAVE3\\DESC.UTF");
    printf("%s %d assertions\n", failures ? "FAIL" : "PASS", checks);
    return failures != 0;
}
