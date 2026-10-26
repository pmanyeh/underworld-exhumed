/* UW1's enhancements: changes the original game does not have, each off unless the player
   turns it on (docs/ENHANCEMENTS.md). In the order of portgame.h's ENH_ enum. */
#include "portgame.h"
#include "sys/enhance.h"

#define UH "UltimaHacks (John Glassmyer, MIT)"

const struct enhance_flag enhance_table[ENH_COUNT] = {
    { "skip-intro", "Starts at the main menu, without the title or the introduction", ENH_TIMING, UH, NULL },
    { "wrap-menu", "The start menu and the saved games wrap from the last item to the first", ENH_PRESENTATION, UH, NULL },
    { "fast-panels", "The right-hand panel slides twice as fast", ENH_TIMING, UH, NULL },
    { "free-heading", "Sliding along a wall no longer turns your view", ENH_GAMEPLAY, UH, NULL },
    { "subtitles", "Cutscenes show their text while the speech plays", ENH_PRESENTATION, UH, "UW2" },
    { "skill-messages", "Says which skill a skill point or a trainer raised, and to what", ENH_PRESENTATION, UH, "UW2" },
    { "perspective", "Floors, ceilings and walls stay straight when the view pitches", ENH_PRESENTATION, "uwpatch (SiENcE, MIT)", NULL },
    { "full-sprites", "Creatures and objects keep their size when the view pitches", ENH_PRESENTATION, UH, NULL },
    { "wide-pitch", "Look three times as far up and down, with what is behind you drawn", ENH_GAMEPLAY, UH, NULL },
    { "mouse-look", "The ` key or the middle button toggles mouse-look: the mouse turns the view, clicks act at the crosshair", ENH_TIMING, UH, NULL },
    { "invert-look", "Mouse-look's up and down inverted", ENH_PRESENTATION, UH, NULL },
    { "modern-keys", "UltimaHacks' keys: WASD, arrows to turn and look, Space attacks, Q looks and E uses at the cursor, Z the map, R and F the panels", ENH_TIMING, UH, NULL },
    { "rune-keys", "Ctrl+Alt+letter picks a rune you have, Ctrl+Alt+Backspace clears, Ctrl+Alt+Space casts", ENH_PRESENTATION, UH, NULL },
    { "tidy-objects", "A stack too heavy to take part of no longer loses an object slot, and lost slots are freed on arrival", ENH_GAMEPLAY, NULL, NULL },
    { "restore-at-rest", "A restored game starts at rest, without the motion or fall it was restored in", ENH_GAMEPLAY, NULL, NULL },
    { "flask-numbers", "The flasks show their values, 25/32, and each change floats up from them (HD text)", ENH_PRESENTATION, NULL, NULL },
    { "auto-key", "Using a locked door or chest unlocks it with the key that fits, if you carry it", ENH_GAMEPLAY, NULL, NULL },
    { "e-takes", "With modern-keys, E on something you can take puts it in your pack", ENH_GAMEPLAY, NULL, NULL },
    { "mouse-wheel", "The mouse wheel scrolls skills under the pointer, open bags, and past [MORE]", ENH_TIMING, NULL, NULL },
    { "damage-numbers", "Damage, misses and blocks float above visible enemies", ENH_PRESENTATION, NULL, NULL },
    { "enemy-health", "Health bars after creatures take damage or attack you; maximum is their highest observed health", ENH_PRESENTATION, NULL, NULL },
    { "full-map", "Show the whole dungeon map without changing explored areas", ENH_PRESENTATION, NULL, NULL },
    { "god-mode", "No health loss or death, with recovery from invalid positions", ENH_GAMEPLAY, NULL, NULL },
    { "bright-view", "Gently brighten the current light source while keeping distance shading and exploration", ENH_PRESENTATION, NULL, NULL },
    { "no-wear", "Equipped weapons, shields and armour do not lose condition through combat use", ENH_GAMEPLAY, NULL, NULL },
};
