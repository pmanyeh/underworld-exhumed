/* Test driver only: link the unchanged native GAMESTRN object and runtime.
   Read (block,index) decimal pairs on stdin; emit the game's result as hex.
   No decoder is duplicated here and no game data is embedded. */
#include "compat.h"
#include "ui.h"
#include "conv.h"
#include "port.h"
#include "i18n/localization.h"
#undef main
#undef fopen
#undef fread
#undef fclose
#undef fgetc
#undef fseek
#undef scanf

void port_log(const char *fmt, ...) { (void)fmt; }
void port_fatal(const char *fmt, ...) { (void)fmt; abort(); }
void port_halt(const char *why) { (void)why; abort(); }
void port_on_flip(void) { }

extern int16 OutString;
extern char str_buff[];

static int check_hook(const char *catalog, const char *log)
{
    char source[512], utf8[16385], first[512];
    char *slot;
    const char *translated;
    int before, i;
    if (port_i18n_init("zh-TW", catalog) || port_i18n_trace_open(log)) return 8;
    CutsceneOrConversationStringBlock = 3651;
    strcpy(source, read_string(3651, 3));
    translated = port_i18n_lookup(3651, 3, source);
    if (translated == source || !(translated[0] & 0x80)) return 9;
    strcpy(utf8, translated);
    if (strcmp(get_string(3), source)) return 10;
    /* a key with no translation: a conversation's input token (P11.4 never translates them; the
       cutscenes this used, block 3072, have had their subtitles since P11) */
    CutsceneOrConversationStringBlock = 3589;
    slot = get_string(50);
    if (strcmp(slot, read_string(3589, 50)) || port_i18n_lookup(3589, 50, slot) != slot) return 11;
    before = OutString;
    slot = get_string((1 << 9) | 2);
    strcpy(first, slot);
    if (slot != str_buff + before) return 12;
    for (i = 0; i < 7; i++) get_string((1 << 9) | 3);
    if (OutString != before || strcmp(slot, first)) return 13;
    if (get_string((1 << 9) | 5) != slot || !strcmp(slot, first)) return 14;
    if (strcmp(translated, utf8)) return 15;
    return 0;
}

int main(int argc, char **argv)
{
    unsigned block, index;
    unsigned char count[2];
    char dynamic[] = "dynamic-marker";
    char player[] = "player-marker";
    int dynamic_id, player_id;
    FILE *file;
    if ((argc != 3 && argc != 5) || port_load_exe(argv[1]) != 0) return 2;
    file = fopen(argv[2], "rb");
    if (!file || fread(count, 1, 2, file) != 2) return 3;
    StringsPak_NoOfNodes = count[0] | count[1] << 8;
    if (StringsPak_NoOfNodes < 1 || StringsPak_NoOfNodes > 256) return 4;
    StringsPak_Address_Indices = calloc(StringsPak_NoOfNodes, sizeof(struct StringNode));
    if (!StringsPak_Address_Indices ||
        fread(StringsPak_Address_Indices, 4, StringsPak_NoOfNodes, file) != (size_t)StringsPak_NoOfNodes)
        return 5;
    StringsPak_FileHandle = file;
    CutsceneOrConversationStringBlock = 3651;
    if (!*get_string(3) || strcmp(get_string(3), read_string(3651, 3))) return 6;
    dynamic_id = make_string(dynamic, 0x7C);
    player_id = make_string(player, 0x7D);
    if (get_string(dynamic_id) != dynamic || get_string(player_id) != player) return 7;
    if (argc == 5) {
        int status = check_hook(argv[3], argv[4]);
        if (status) return status;
        if (get_string(dynamic_id) != dynamic || get_string(player_id) != player) return 16;
    }
    while (scanf("%u %u", &block, &index) == 2) {
        const unsigned char *text = (const unsigned char *)read_string(block, index);
        printf("%u\t%u\t", block, index);
        while (*text) printf("%02x", *text++);
        putchar('\n');
    }
    free(StringsPak_Address_Indices);
    fclose(file);
    port_i18n_shutdown();
    return 0;
}
