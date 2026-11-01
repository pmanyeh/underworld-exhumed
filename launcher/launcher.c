/* launcher.c: the release's front page (Underworld.exe), which starts Ultima Underworld or Ultima
   Underworld II. Replaces nothing in either game.

   The release is one folder: this program, uw1port and uw2port with the libraries they share,
   fonts/, and a home folder for each game (UW1/, UW2/: the saves, the settings, the Chinese
   catalog in i18n/zh-TW). Each game is started with that folder as its home and the release's
   folder as its current one, so its fonts and catalog are found as a playtest folder's are.

   Where each game is comes from the games themselves: uw1port and uw2port --find-game search as
   they do on a player's start (sys/gamedir.c: the remembered folder, GOG's registry entries and
   install folders, a GOG CD image), remember what they find in their home's settings file and
   exit 0. A game not found is started all the same, and asks for its folder in its own dialog.
   The search runs on a thread, so the page shows at once.

   The pictures are the games' own title screens, read from the player's copy when it is found:
   UW1's DATA/OPSCR.BYT and UW2's block 5 of DATA/BYT.ARK (LZSS, as UW2's ARC.C reads it), with
   palette 2 of DATA/PALS.DAT. None is shipped.

   The page is drawn at CANVAS_W by CANVAS_H into an RGBA buffer, the text with the games' HD
   font (presentation/hd_font.c), and scaled to the window. */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "presentation/hd_font.h"

#ifndef LAUNCHER_VERSION
#define LAUNCHER_VERSION "dev"
#endif
#define ISSUES_URL "https://github.com/pmanyeh/underworld-exhumed/issues"
#define FONT_FILE "fonts/jf-openhuninn-2.1.ttf"

enum { CANVAS_W = 1440, CANVAS_H = 860 };
enum { CARD_Y = 150, CARD_W = 640, CARD_H = 400, CARD_GAP = 40 };
enum { BUTTON_Y = 740, BUTTON_W = 220, BUTTON_H = 60 };
enum { NOT_ASKED, SEARCHING, FOUND, NOT_FOUND };
enum { B_HELP, B_ISSUES, B_QUIT, BUTTONS };

typedef struct Game {
    const char *exe;            /* the program, in the release's folder */
    const char *home;           /* its home folder there */
    const char *cfg;            /* its settings file in the home folder */
    const char *name, *subtitle;
    SDL_AtomicInt state;
    char data[1024];            /* where the search found it (state FOUND), set before state */
    uint8_t picture[320 * 200 * 4];
    int have_picture;
} Game;

static Game games[2] = {
    { .exe = "uw1port", .home = "UW1", .cfg = "uw1port.cfg", .name = "地下創世紀：冥河深淵",
      .subtitle = "Ultima Underworld: The Stygian Abyss" },
    { .exe = "uw2port", .home = "UW2", .cfg = "uw2port.cfg", .name = "地下創世紀 II：萬界迷宮",
      .subtitle = "Ultima Underworld II: Labyrinth of Worlds" },
};
static const char *const button_text[BUTTONS] = { "說明", "回報問題", "結束" };

static char base[1024];         /* the release's folder, with its separator */
static char programs[1024];     /* where uw1port and uw2port are: base, or on macOS the app's
                                   Contents/MacOS, the three programs being one app */
static uint8_t canvas[CANVAS_W * CANVAS_H * 4];
static HdFont *font;
static int selected = 0, hover_button = -1;
static SDL_AtomicInt playing;   /* a game is running: the page is hidden */
static int returning;           /* the page comes back when the game started ends */
static Uint32 event_done;       /* a search or a game ended */

/* ---------------------------------------------------------------- files */

static void path_in(char *out, size_t n, const char *dir, const char *name)
{
    size_t l = strlen(dir);
    snprintf(out, n, "%s%s%s", dir, l && (dir[l - 1] == '/' || dir[l - 1] == '\\') ? "" : "/", name);
}

struct find_ci { const char *want; char found[256]; };
static SDL_EnumerationResult SDLCALL find_ci_entry(void *ud, const char *dir, const char *name)
{
    struct find_ci *f = ud;
    (void)dir;
    if (SDL_strcasecmp(name, f->want) == 0) {
        SDL_strlcpy(f->found, name, sizeof f->found);
        return SDL_ENUM_SUCCESS;
    }
    return SDL_ENUM_CONTINUE;
}

/* dir/sub/name with the case the files have (a Mac or Linux copy of a DOS game's files) */
static int find_file(const char *dir, const char *sub, const char *name, char *out, size_t n)
{
    char d[1200];
    struct find_ci f;
    f.want = sub; f.found[0] = 0;
    SDL_EnumerateDirectory(dir, find_ci_entry, &f);
    if (!f.found[0]) return -1;
    path_in(d, sizeof d, dir, f.found);
    f.want = name; f.found[0] = 0;
    SDL_EnumerateDirectory(d, find_ci_entry, &f);
    if (!f.found[0]) return -1;
    path_in(out, n, d, f.found);
    return 0;
}

static void *read_all(const char *path, size_t *len)
{
    return SDL_LoadFile(path, len);
}

/* the value of key in a game's settings file, the last line naming it */
static int config_get(const Game *g, const char *key, char *out, size_t n)
{
    char p[1200];
    size_t len, k = strlen(key);
    char *text, *line, *next;
    int found = -1;
    path_in(p, sizeof p, base, g->home);
    path_in(p, sizeof p, p, g->cfg);
    if (!(text = read_all(p, &len))) return -1;
    for (line = text; line && *line; line = next) {
        char *end;
        next = strchr(line, '\n');
        if (next) *next++ = 0;
        end = line + strlen(line);
        while (end > line && (end[-1] == '\r' || end[-1] == ' ')) *--end = 0;
        if (!strncmp(line, key, k) && line[k] == '=') { SDL_strlcpy(out, line + k + 1, n); found = 0; }
    }
    SDL_free(text);
    return found;
}

/* ---------------------------------------------------------------- the title screens */

static void palette(const char *data, uint8_t rgb[768])
{
    char p[1200];
    size_t len = 0;
    uint8_t *pals;
    int i;
    memset(rgb, 0, 768);
    if (find_file(data, "DATA", "PALS.DAT", p, sizeof p)) return;
    if ((pals = read_all(p, &len)) != NULL && len >= 3 * 768)
        for (i = 0; i < 768; i++) rgb[i] = (uint8_t)((pals[2 * 768 + i] & 63) * 255 / 63);
    SDL_free(pals);
}

/* UW2's .ark block decompression: Okumura's LZSS (UW2's LZSS.C), a 4096-byte ring filled with
   spaces, matches of 3 to 18 bytes */
static size_t lzss(const uint8_t *src, size_t n, uint8_t *dst, size_t max)
{
    uint8_t ring[4096];
    size_t i = 0, o = 0;
    unsigned flags = 0, r = 4096 - 18;
    memset(ring, ' ', sizeof ring);
    while (i < n && o < max) {
        flags >>= 1;
        if (!(flags & 0x100)) { flags = src[i++] | 0xFF00; if (i >= n) break; }
        if (flags & 1) {
            dst[o++] = ring[r] = src[i++];
            r = (r + 1) & 4095;
        } else {
            unsigned p, len, k;
            if (i + 1 >= n) break;
            p = src[i] | (src[i + 1] & 0xF0) << 4;
            len = (src[i + 1] & 0x0F) + 3;
            i += 2;
            for (k = 0; k < len && o < max; k++) {
                dst[o++] = ring[r] = ring[(p + k) & 4095];
                r = (r + 1) & 4095;
            }
        }
    }
    return o;
}

static uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

static int title_uw1(const char *data, uint8_t pix[64000])
{
    char p[1200];
    size_t len = 0;
    uint8_t *b;
    if (find_file(data, "DATA", "OPSCR.BYT", p, sizeof p) || !(b = read_all(p, &len))) return -1;
    if (len >= 64000) memcpy(pix, b, 64000);
    SDL_free(b);
    return len >= 64000 ? 0 : -1;
}

static int title_uw2(const char *data, uint8_t pix[64000])
{
    char p[1200];
    size_t len = 0;
    uint8_t *ark;
    unsigned count, blk = 5;
    uint32_t off, flags, size;
    int ok = -1;
    if (find_file(data, "DATA", "BYT.ARK", p, sizeof p) || !(ark = read_all(p, &len))) return -1;
    count = len >= 2 ? ark[0] | ark[1] << 8 : 0;
    if (blk < count && 6 + 12 * (size_t)count <= len) {
        off = le32(ark + 6 + 4 * blk);
        flags = le32(ark + 6 + 4 * count + 4 * blk);
        size = le32(ark + 6 + 8 * count + 4 * blk);
        if (off && off + (size_t)size <= len && size > 4) {
            if (flags & 2) ok = lzss(ark + off + 4, size - 4, pix, 64000) == 64000 ? 0 : -1;
            else if (size >= 64000) { memcpy(pix, ark + off, 64000); ok = 0; }
        }
    }
    SDL_free(ark);
    return ok;
}

static void load_picture(Game *g)
{
    static uint8_t pix[64000];
    uint8_t rgb[768];
    int i;
    g->have_picture = 0;
    if ((g == &games[0] ? title_uw1 : title_uw2)(g->data, pix)) return;
    palette(g->data, rgb);
    for (i = 0; i < 64000; i++) {
        g->picture[i * 4] = rgb[pix[i] * 3];
        g->picture[i * 4 + 1] = rgb[pix[i] * 3 + 1];
        g->picture[i * 4 + 2] = rgb[pix[i] * 3 + 2];
        g->picture[i * 4 + 3] = 255;
    }
    g->have_picture = 1;
}

/* ---------------------------------------------------------------- the games */

static void exe_path(const Game *g, char *out, size_t n)
{
#ifdef _WIN32
    char name[64];
    snprintf(name, sizeof name, "%s.exe", g->exe);
    path_in(out, n, programs, name);
#else
    path_in(out, n, programs, g->exe);
#endif
}

/* a test's (--test-play): the game quits by itself, without sound or a recording */
static const char *const test_args[] = { "--exit-after", "4000", "--sound", "0,0", "--no-recording", NULL };
static int testing;

static SDL_Process *start(const Game *g, int find)
{
    char exe[1200], home[1200];
    const char *args[12];
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_Process *p;
    int n = 0, k;
    exe_path(g, exe, sizeof exe);
    path_in(home, sizeof home, base, g->home);
    args[n++] = exe; args[n++] = "--home"; args[n++] = home;
    if (find) args[n++] = "--find-game";
    else if (testing)
        for (k = 0; test_args[k]; k++) args[n++] = test_args[k];
    args[n] = NULL;
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)args);
    SDL_SetStringProperty(props, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, base);
    if (find) {
        SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_NULL);
        SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_NULL);
    }
    p = SDL_CreateProcessWithProperties(props);
    SDL_DestroyProperties(props);
    return p;
}

static void post_done(void)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = event_done;
    SDL_PushEvent(&e);
}

static int SDLCALL search_thread(void *unused)
{
    int i;
    (void)unused;
    for (i = 0; i < 2; i++) {
        Game *g = &games[i];
        SDL_Process *p = start(g, 1);
        int status = 1;
        if (p) {
            SDL_WaitProcess(p, true, &status);
            SDL_DestroyProcess(p);
        }
        if (status == 0 && config_get(g, "data", g->data, sizeof g->data) == 0) SDL_SetAtomicInt(&g->state, FOUND);
        else SDL_SetAtomicInt(&g->state, NOT_FOUND);
        post_done();
    }
    return 0;
}

static void search(void)
{
    SDL_Thread *t;
    SDL_SetAtomicInt(&games[0].state, SEARCHING);
    SDL_SetAtomicInt(&games[1].state, SEARCHING);
    if ((t = SDL_CreateThread(search_thread, "search", NULL)) != NULL) SDL_DetachThread(t);
    else { SDL_SetAtomicInt(&games[0].state, NOT_FOUND); SDL_SetAtomicInt(&games[1].state, NOT_FOUND); }
}

static int SDLCALL play_thread(void *process)
{
    int status = 0;
    SDL_WaitProcess(process, true, &status);
    SDL_DestroyProcess(process);
    SDL_SetAtomicInt(&playing, 0);
    post_done();
    return 0;
}

static void play(SDL_Window *window, int i)
{
    SDL_Process *p;
    SDL_Thread *t;
    char exe[1200], msg[1400];
    if (SDL_GetAtomicInt(&playing)) return;
    if (!(p = start(&games[i], 0))) {
        exe_path(&games[i], exe, sizeof exe);
        snprintf(msg, sizeof msg, "無法啟動 %s。\n\n%s", exe, SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, games[i].name, msg, window);
        return;
    }
    SDL_SetAtomicInt(&playing, 1);
    returning = 1;
    SDL_HideWindow(window);
    if ((t = SDL_CreateThread(play_thread, "play", p)) != NULL) SDL_DetachThread(t);
}

/* ---------------------------------------------------------------- drawing */

static PlatOverlay frame(void)
{
    PlatOverlay f;
    memset(&f, 0, sizeof f);
    f.rgba = canvas; f.width = CANVAS_W; f.height = CANVAS_H; f.pitch = CANVAS_W * 4;
    f.window_width = CANVAS_W; f.window_height = CANVAS_H;
    f.logical_width = CANVAS_W; f.logical_height = CANVAS_H;
    f.viewport_w = CANVAS_W; f.viewport_h = CANVAS_H;
    return f;
}

static void fill(int x, int y, int w, int h, uint32_t rgb)
{
    int i, j;
    for (j = y < 0 ? 0 : y; j < y + h && j < CANVAS_H; j++)
        for (i = x < 0 ? 0 : x; i < x + w && i < CANVAS_W; i++) {
            uint8_t *p = canvas + (j * CANVAS_W + i) * 4;
            p[0] = (uint8_t)(rgb >> 16); p[1] = (uint8_t)(rgb >> 8); p[2] = (uint8_t)rgb; p[3] = 255;
        }
}

static void frame_rect(int x, int y, int w, int h, int t, uint32_t rgb)
{
    fill(x - t, y - t, w + 2 * t, t, rgb);
    fill(x - t, y + h, w + 2 * t, t, rgb);
    fill(x - t, y, t, h, rgb);
    fill(x + w, y, t, h, rgb);
}

static float text_width(const char *s, float em)
{
    HdTextMetrics m;
    PlatOverlay f = frame();
    if (!font || hd_text_measure_logical(font, &f, s, strlen(s), em, 0, &m)) return 0;
    return m.width;
}

static void text(const char *s, float x, float y, float em, uint32_t rgb)
{
    PlatOverlay f = frame();
    HdColor c = { (uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb, 255 };
    if (font) hd_text_draw(font, &f, s, strlen(s), x, y, em, 0, c);
}

static void text_centered(const char *s, float cx, float y, float em, uint32_t rgb)
{
    text(s, cx - text_width(s, em) / 2, y, em, rgb);
}

/* s fitted to width, its start dropped for "…" (a path's end says the most) */
static void text_fitted(const char *s, float cx, float y, float em, float width, uint32_t rgb)
{
    char buf[1300];
    const char *p = s;
    while (*p && text_width(p, em) > width - text_width("…", em)) {
        p++;
        while ((*p & 0xC0) == 0x80) p++;     /* whole UTF-8 characters */
    }
    if (p == s) { text_centered(s, cx, y, em, rgb); return; }
    snprintf(buf, sizeof buf, "…%s", p);
    text_centered(buf, cx, y, em, rgb);
}

static int card_x(int i) { return (CANVAS_W - 2 * CARD_W - CARD_GAP) / 2 + i * (CARD_W + CARD_GAP); }
static int button_x(int b) { return (CANVAS_W - BUTTONS * BUTTON_W - (BUTTONS - 1) * 30) / 2 + b * (BUTTON_W + 30); }

enum { BACKGROUND = 0x15110D, GOLD = 0xE2BE6A, DIM_GOLD = 0x6E5A3A, TEXT = 0xE8E0D0, GREY = 0x9A9184 };

static void draw(void)
{
    int i, b, x, y;
    char line[1300];
    fill(0, 0, CANVAS_W, CANVAS_H, BACKGROUND);
    text_centered("地下創世紀 繁體中文版", CANVAS_W / 2.0f, 34, 46, GOLD);
    snprintf(line, sizeof line, "Ultima Underworld 1＋2　　版本 %s　　選一款遊戲開始", LAUNCHER_VERSION);
    text_centered(line, CANVAS_W / 2.0f, 96, 22, GREY);
    for (i = 0; i < 2; i++) {
        Game *g = &games[i];
        int state = SDL_GetAtomicInt(&g->state), sel = i == selected;
        x = card_x(i);
        if (g->have_picture)
            for (y = 0; y < CARD_H; y++) {
                const uint8_t *src = g->picture + (y / 2) * 320 * 4;
                uint8_t *dst = canvas + ((CARD_Y + y) * CANVAS_W + x) * 4;
                int k;
                for (k = 0; k < CARD_W; k++) memcpy(dst + k * 4, src + (k / 2) * 4, 4);
            }
        else {
            fill(x, CARD_Y, CARD_W, CARD_H, 0x2A241C);
            text_centered(g->subtitle, x + CARD_W / 2.0f, CARD_Y + CARD_H / 2.0f - 20, 30, sel ? GOLD : DIM_GOLD);
        }
        frame_rect(x, CARD_Y, CARD_W, CARD_H, sel ? 6 : 2, sel ? GOLD : DIM_GOLD);
        text_centered(g->name, x + CARD_W / 2.0f, CARD_Y + CARD_H + 16, 30, sel ? GOLD : TEXT);
        text_centered(g->subtitle, x + CARD_W / 2.0f, CARD_Y + CARD_H + 56, 22, sel ? TEXT : GREY);
        if (state == FOUND) {
            char where[1100], *c;
            if (strstr(g->data, "gog-cd"))     /* copied out of GOG's CD image into the home */
                SDL_strlcpy(where, "已從 GOG 安裝取出", sizeof where);
            else {
                /* "/": the HD text reads a backslash as the start of a colour code */
                SDL_strlcpy(where, g->data, sizeof where);
                for (c = where; *c; c++) if (*c == '\\') *c = '/';
            }
            snprintf(line, sizeof line, "遊戲位置：%s", where);
            text_fitted(line, x + CARD_W / 2.0f, CARD_Y + CARD_H + 92, 18, CARD_W, GREY);
        } else
            text_centered(state == NOT_FOUND ? "還沒找到這款遊戲：按下開始後選擇遊戲資料夾" : "正在尋找遊戲……",
                          x + CARD_W / 2.0f, CARD_Y + CARD_H + 92, 18, state == NOT_FOUND ? 0xD08A6A : GREY);
    }
    text_centered("滑鼠點選或用 ← → 選擇，Enter 開始，Esc 結束", CANVAS_W / 2.0f, 690, 20, GREY);
    for (b = 0; b < BUTTONS; b++) {
        x = button_x(b);
        fill(x, BUTTON_Y, BUTTON_W, BUTTON_H, b == hover_button ? 0x3A3024 : 0x262019);
        frame_rect(x, BUTTON_Y, BUTTON_W, BUTTON_H, 2, b == hover_button ? GOLD : DIM_GOLD);
        text_centered(button_text[b], x + BUTTON_W / 2.0f, BUTTON_Y + 14, 26, TEXT);
    }
}

/* ---------------------------------------------------------------- input */

static int card_at(float x, float y)
{
    int i;
    for (i = 0; i < 2; i++)
        if (x >= card_x(i) && x < card_x(i) + CARD_W && y >= CARD_Y && y < CARD_Y + CARD_H + 100) return i;
    return -1;
}

static int button_at(float x, float y)
{
    int b;
    for (b = 0; b < BUTTONS; b++)
        if (x >= button_x(b) && x < button_x(b) + BUTTON_W && y >= BUTTON_Y && y < BUTTON_Y + BUTTON_H) return b;
    return -1;
}

static int button(SDL_Window *window, int b)
{
    char p[1200], url[1400];
    if (b == B_QUIT) return 1;
    if (b == B_ISSUES) SDL_OpenURL(ISSUES_URL);
    if (b == B_HELP) {
        /* a file URL, its path percent-encoded (macOS refuses one with spaces or Chinese in it) */
        static const char hex[] = "0123456789ABCDEF";
        const unsigned char *c;
        size_t n = 0;
        path_in(p, sizeof p, base, "說明.txt");
        n = (size_t)snprintf(url, sizeof url, "file://%s", p[0] == '/' ? "" : "/");
        for (c = (const unsigned char *)p; *c && n + 4 < sizeof url; c++)
            if (*c == '\\') url[n++] = '/';
            else if (*c >= 0x80 || *c == ' ' || *c == '%' || *c == '#' || *c == '?') {
                url[n++] = '%'; url[n++] = hex[*c >> 4]; url[n++] = hex[*c & 15];
            } else url[n++] = (char)*c;
        url[n] = 0;
        if (!SDL_OpenURL(url))
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "說明", "說明.txt 在這個程式的資料夾裡。", window);
    }
    return 0;
}

/* ---------------------------------------------------------------- main */

/* The release's folder: given (--base, the macOS start script's), else the program's own; on macOS,
   inside an app, the programs are its Contents/MacOS and the release's folder the one holding the
   app. The current folder becomes the release's, where the games find fonts/. */
static void enter_base(const char *given)
{
    const char *b = SDL_GetBasePath();
    char *r;
    SDL_strlcpy(base, b ? b : "./", sizeof base);
    SDL_strlcpy(programs, base, sizeof programs);
#ifdef __APPLE__
    if ((r = strstr(programs, ".app/Contents/Resources/")) != NULL) {
        strcpy(r + strlen(".app/Contents/"), "MacOS/");
        SDL_strlcpy(base, programs, sizeof base);
        r = strstr(base, ".app/Contents/");
        while (r > base && r[-1] != '/') r--;   /* the app's own folder name */
        *r = 0;
    }
#else
    (void)r;
#endif
    if (given && *given) {
        SDL_strlcpy(base, given, sizeof base);
        if (base[strlen(base) - 1] != '/' && base[strlen(base) - 1] != '\\') SDL_strlcat(base, "/", sizeof base);
    }
#ifdef _WIN32
    {
        wchar_t w[1024];
        if (MultiByteToWideChar(CP_UTF8, 0, base, -1, w, 1024)) SetCurrentDirectoryW(w);
    }
#else
    if (chdir(base)) {}
#endif
}

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    const SDL_DisplayMode *mode;
    int quit = 0, dirty = 1, w = 1152, h = 688, i, shot = 0, searched, test_play = -1, launched = 0;
    const char *shot_path = NULL, *base_arg = NULL;

    /* a test's: --screenshot FILE writes the page once both games are searched for and quits;
       --test-play N first starts game N (0 or 1) and waits for it, the game quitting by itself */
    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) shot_path = argv[++i];
        else if (!strcmp(argv[i], "--test-play") && i + 1 < argc) { test_play = atoi(argv[++i]) & 1; testing = 1; }
        else if (!strcmp(argv[i], "--base") && i + 1 < argc) base_arg = argv[++i];
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "underworld: %s\n", SDL_GetError());
        return 1;
    }
    enter_base(base_arg);
    event_done = SDL_RegisterEvents(1);
    if (hd_font_init(&font, FONT_FILE)) {
        font = NULL;
        fprintf(stderr, "underworld: no font %s\n", FONT_FILE);
    }
    if ((mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay())) != NULL && mode->w > 0) {
        w = mode->w * 3 / 4;
        if (w > CANVAS_W) w = CANVAS_W;
        h = w * CANVAS_H / CANVAS_W;
    }
    if (!SDL_CreateWindowAndRenderer("地下創世紀 繁體中文版", w, h,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (shot_path ? SDL_WINDOW_HIDDEN : 0),
                                     &window, &renderer)) {
        fprintf(stderr, "underworld: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderLogicalPresentation(renderer, CANVAS_W, CANVAS_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, CANVAS_W, CANVAS_H);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    search();

    while (!quit) {
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, 200)) do {
            SDL_ConvertEventToRenderCoordinates(renderer, &e);
            if (e.type == SDL_EVENT_QUIT) quit = 1;
            else if (e.type == event_done) {
                if (returning && !SDL_GetAtomicInt(&playing)) {
                    returning = 0;      /* the game ended: the page again */
                    if (!shot_path) { SDL_ShowWindow(window); SDL_RaiseWindow(window); }
                    games[0].have_picture = games[1].have_picture = 0;
                    search();           /* a game may have just been given its folder */
                }
                for (i = 0; i < 2; i++)
                    if (SDL_GetAtomicInt(&games[i].state) == FOUND && !games[i].have_picture) load_picture(&games[i]);
                dirty = 1;
            } else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                int c = card_at(e.motion.x, e.motion.y), b = button_at(e.motion.x, e.motion.y);
                if (c >= 0 && c != selected) { selected = c; dirty = 1; }
                if (b != hover_button) { hover_button = b; dirty = 1; }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
                int c = card_at(e.button.x, e.button.y), b = button_at(e.button.x, e.button.y);
                if (c >= 0) play(window, c);
                else if (b >= 0) quit = button(window, b);
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                case SDLK_LEFT: case SDLK_1: selected = 0; dirty = 1; break;
                case SDLK_RIGHT: case SDLK_2: selected = 1; dirty = 1; break;
                case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE: play(window, selected); break;
                case SDLK_ESCAPE: quit = 1; break;
                case SDLK_F1: button(window, B_HELP); break;
                default: break;
                }
            } else if (e.type == SDL_EVENT_WINDOW_EXPOSED || e.type == SDL_EVENT_WINDOW_RESIZED
                       || e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) dirty = 1;
        } while (SDL_PollEvent(&e));
        if (dirty && !SDL_GetAtomicInt(&playing)) {
            draw();
            SDL_UpdateTexture(texture, NULL, canvas, CANVAS_W * 4);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            dirty = 0;
        }
        searched = SDL_GetAtomicInt(&games[0].state) >= FOUND && SDL_GetAtomicInt(&games[1].state) >= FOUND;
        if (test_play >= 0 && !launched && searched) {
            launched = 1;
            play(window, test_play);
            continue;
        }
        if (shot_path && searched && (test_play < 0 || (launched && !returning)) && !shot++) {
            SDL_Surface *s = SDL_CreateSurfaceFrom(CANVAS_W, CANVAS_H, SDL_PIXELFORMAT_ABGR8888, canvas, CANVAS_W * 4);
            draw();
            if (s) { SDL_SaveBMP(s, shot_path); SDL_DestroySurface(s); }
            quit = 1;
        }
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    hd_font_shutdown(font);
    SDL_Quit();
    return 0;
}
