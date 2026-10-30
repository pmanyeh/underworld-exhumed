/* --enhance flask-numbers: the vitality and mana flasks' values as "25/32" on the flasks, and
   each change floating up from its flask ("-3" red, "+2" green), in the HD font. The game
   thread gives the values each frame the flasks are drawn (INTERACT.C's display_scr); the
   presentation thread draws the published snapshot, and nothing once the game has stopped
   giving them (another screen: the map, a cutscene). Display only. Owned, bounded state, as
   caption.h; no game, SDL or FreeType types in the state. */
#ifndef UW1_FLASK_TEXT_H
#define UW1_FLASK_TEXT_H
#include <stdint.h>
#define FLASK_FLOATS 12
#define FLASK_FLOAT_LIFE 384            /* game time: a second and a half (256 a second) */
typedef struct FlaskFloat { int which, delta, age; } FlaskFloat;
typedef struct FlaskFrame {
    uint64_t generation;
    int active, value[2], max[2], poisoned, count;
    FlaskFloat floats[FLASK_FLOATS];
} FlaskFrame;

void flask_text_enable(int on);         /* before threads */
int flask_text_enabled(void);
/* Game thread, each frame the flasks are up: the values and the game's clock (GAME_TIME). */
void flask_text_update(int hp, int max_hp, int mana, int max_mana, int poisoned, uint32_t now);
/* Game thread, when flask-numbers is turned off in play: nothing shown, no floats from the old values */
void flask_text_clear(void);
void flask_text_read(FlaskFrame *out);
/* Presentation thread: the snapshot, while the game is still giving it; 1 when drawn. */
struct HdFont;
struct PlatOverlay;
int flask_text_draw(struct HdFont *font, const struct PlatOverlay *frame, const FlaskFrame *f, float border);
#endif
