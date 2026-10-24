/* P12: the player's own words in Chinese: the character's name and the saved games'
   descriptions. The game keeps an ASCII stand-in where it keeps them (player->name, SAVEn\DESC),
   so its files are as the original's and UTF-8 never enters its buffers; the Chinese is kept
   beside them, in the save's folder (SAVE0\NAME.UTF, SAVEn\DESC.UTF, which travel with the
   save as the game copies SAVE0), and shown in place of the stand-in. */
#ifndef UW1_USER_TEXT_H
#define UW1_USER_TEXT_H
#define USER_NAME_STANDIN "Avatar"

/* the character's name: zh, NULL for none (an ASCII name the game keeps itself) */
void port_user_name_set(const char *zh);
const char *port_user_name(void);
/* SAVE0\NAME.UTF written (or removed, without a Chinese name), and read after a restore */
void port_user_name_save(void);
void port_user_name_load(void);
/* a slot's (1..4) description: SAVEn\DESC.UTF read for the lists; written into SAVE0 before the
   game copies it to the slot (zh NULL removes it) */
void port_user_descs_load(void);
void port_user_desc_save(const char *zh);
/* a slot's stand-in: "Save N" */
const char *port_user_desc_standin(int slot);
/* what to show for english, a string the game is about to show: the Chinese name for the
   name's stand-in (any case), a slot's Chinese description for its stand-in; else NULL */
const char *port_user_text(const char *english);
/* whether utf8 is all ASCII */
int port_utf8_ascii(const char *utf8);
#endif
