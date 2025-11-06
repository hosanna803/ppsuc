/* retro_mario_c.c
   Retro-style platformer in C using SDL2 (no external assets).
   - Requires: SDL2 and SDL2_ttf
   - Compile (Linux/macOS):
       gcc retro_mario_c.c -o retro_mario_c $(sdl2-config --cflags --libs) -lSDL2_ttf
     Or Windows (MinGW) adjust include/linker flags for SDL2 and SDL2_ttf.

   Controls:
     Left/Right / A/D : move
     Z / Space / Up   : jump
     R                : restart level
     Enter            : start/proceed on menus
     Esc              : quit

   Note: This is NOT the original Nintendo game. It's an original reimplementation
   of classic platformer mechanics with simple drawn sprites.
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ---------------- CONFIG ---------------- */
const int SCREEN_W = 960;
const int SCREEN_H = 540;
const int TILE = 48;
const int FPS = 60;
const float GRAVITY = 0.60f;
const float MAX_XSPEED = 6.0f;
const float JUMP_VEL = -12.0f;

/* Colors */
SDL_Color SKY = {120,200,255,255};
SDL_Color BLOCK = {200,120,60,255};
SDL_Color GROUND = {92,56,28,255};
SDL_Color COIN_COL = {255,215,0,255};
SDL_Color PLAYER_COL = {200,30,40,255};
SDL_Color PLAYER_BIG_COL = {255,90,90,255};
SDL_Color ENEMY_COL = {120,60,20,255};
SDL_Color MUSH_COL = {180,40,40,255};
SDL_Color FLAG_COL = {50,200,100,255};
SDL_Color HUD_COL = {30,30,30,255};

/* ---------------- LEVELS (strings) ----------------
   Legend:
     'X' block, '=' ground (solid)
     't' top grass cap (visual)
     'C' coin
     'E' enemy
     'M' mushroom
     'F' flag
     ' ' empty
*/
const char *LEVELS[][8] = {
    {
    "                                                                                ",
    "                                                                                ",
    "                                                                                ",
    "                   C                                                            ",
    "        C                                                                       F",
    "    C       E                 C                                                 ",
    "XXXXXXXXXXXX      XXXX     XXXXXXXX      C                     C               ",
    "==========ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt"
    },
    {
    "                                                                                ",
    "                                                                                ",
    "        C                               C                                       ",
    "                E                 C                                              ",
    "    XXXXX        XXXXXX      XXXXXXXX              C               F           ",
    "          C               C         E                                     C    ",
    "   C    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   C                    ",
    "==========ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt"
    },
    {
    "                                                                                ",
    "                                                                                ",
    "                   C               E                                            ",
    "        M       XXXX      C      XXXXX       C                          F        ",
    "    C           X  X              X   X                E                 C      ",
    "XXXXXXXXXXXX    X  X    C    C    X   X    C    C    X   X    C    C    XXX    ",
    "==========ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt",
    "                                                                                "
    }
};
const int NUM_LEVELS = 3;

/* ---------------- STRUCTS ---------------- */
typedef struct {
    SDL_FRect r;    /* position & size as float rect */
    float vx, vy;
    int on_ground;
    int big;
    float big_timer;
    int coins;
    int score;
    int lives;
    SDL_FPoint spawn;
} Player;

typedef struct {
    SDL_Rect r;
    int active;
    int dir; /* -1 or +1 */
    float speed;
} Enemy;

typedef struct {
    SDL_Rect r;
    int active;
} Coin;

typedef struct {
    SDL_Rect r;
    int active;
} Mushroom;

/* ---------------- HELPERS ---------------- */
void draw_rect(SDL_Renderer *ren, int x, int y, int w, int h, SDL_Color col) {
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
    SDL_Rect rr = {x,y,w,h};
    SDL_RenderFillRect(ren, &rr);
}
void draw_frect(SDL_Renderer *ren, SDL_FRect fr, SDL_Color col) {
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
    SDL_Rect rr = {(int)roundf(fr.x),(int)roundf(fr.y),(int)roundf(fr.w),(int)roundf(fr.h)};
    SDL_RenderFillRect(ren, &rr);
}
int aabb_int(SDL_Rect a, SDL_Rect b) {
    return !(a.x + a.w <= b.x || a.x >= b.x + b.w || a.y + a.h <= b.y || a.y >= b.y + b.h);
}
int aabb_frect(SDL_FRect a, SDL_FRect b) {
    return !(a.x + a.w <= b.x || a.x >= b.x + b.w || a.y + a.h <= b.y || a.y >= b.y + b.h);
}

/* ---------------- LEVEL BUILD ---------------- */
#define MAX_SOLIDS 1024
SDL_Rect solids[MAX_SOLIDS];
int solids_count = 0;

#define MAX_COINS 512
Coin coins[MAX_COINS];
int coin_count = 0;

#define MAX_ENEMIES 128
Enemy enemies[MAX_ENEMIES];
int enemy_count = 0;

#define MAX_MUSH 64
Mushroom mush[MAX_MUSH];
int mush_count = 0;

SDL_Rect goal_rect;
int goal_exists = 0;
int world_width = 0;

void build_level(const char *map_lines[], int rows) {
    solids_count = coin_count = enemy_count = mush_count = 0;
    goal_exists = 0;
    world_width = (int)strlen(map_lines[0]) * TILE;
    for (int j=0;j<rows;j++){
        const char *row = map_lines[j];
        for (int i=0;i<(int)strlen(row);i++){
            char ch = row[i];
            int x = i*TILE;
            int y = j*TILE;
            if (ch=='X' || ch=='=') {
                if (solids_count < MAX_SOLIDS) {
                    solids[solids_count++] = (SDL_Rect){x,y,TILE,TILE};
                }
            } else if (ch=='t') {
                if (solids_count < MAX_SOLIDS) {
                    solids[solids_count++] = (SDL_Rect){x,y+TILE/2,TILE,TILE/2};
                }
            } else if (ch=='C') {
                if (coin_count < MAX_COINS) {
                    coins[coin_count++] = (Coin){.r = {x+TILE/4, y+TILE/4, TILE/2, TILE/2}, .active = 1};
                }
            } else if (ch=='E') {
                if (enemy_count < MAX_ENEMIES) {
                    enemies[enemy_count++] = (Enemy){.r = {x+6,y+8,TILE-12,TILE-16}, .active = 1, .dir = -1, .speed = 1.2f};
                }
            } else if (ch=='M') {
                if (mush_count < MAX_MUSH) {
                    mush[mush_count++] = (Mushroom){.r = {x+12,y+12,TILE-24,TILE-24}, .active = 1};
                }
            } else if (ch=='F') {
                goal_rect = (SDL_Rect){x+TILE/2-6,y-4*TILE,12,4*TILE};
                goal_exists = 1;
            }
        }
    }
}

/* ---------------- COLLISION helpers for player (float rect) ---------------- */
void resolve_horz_collision(Player *p) {
    SDL_FRect fr = p->r;
    fr.x += p->vx;
    /* build integer rect to test against solids */
    SDL_Rect test = {(int)roundf(fr.x),(int)roundf(fr.y),(int)roundf(fr.w),(int)roundf(fr.h)};
    for (int i=0;i<solids_count;i++){
        if (aabb_int(test, solids[i])) {
            if (p->vx > 0) {
                p->r.x = solids[i].x - p->r.w;
            } else if (p->vx < 0) {
                p->r.x = solids[i].x + solids[i].w;
            }
            p->vx = 0;
            return;
        }
    }
    p->r.x += p->vx;
}
void resolve_vert_collision(Player *p) {
    SDL_FRect fr = p->r;
    fr.y += p->vy;
    SDL_Rect test = {(int)roundf(fr.x),(int)roundf(fr.y),(int)roundf(fr.w),(int)roundf(fr.h)};
    p->on_ground = 0;
    for (int i=0;i<solids_count;i++){
        if (aabb_int(test, solids[i])) {
            if (p->vy > 0) {
                p->r.y = solids[i].y - p->r.h;
                p->on_ground = 1;
            } else if (p->vy < 0) {
                p->r.y = solids[i].y + solids[i].h;
            }
            p->vy = 0;
            return;
        }
    }
    p->r.y += p->vy;
}

/* ---------------- ENEMY movement ---------------- */
void update_enemies(float dt) {
    for (int i=0;i<enemy_count;i++){
        if (!enemies[i].active) continue;
        Enemy *e = &enemies[i];
        float oldx = e->r.x;
        e->r.x += (int)roundf(e->dir * e->speed);
        /* horizontal collision with solids */
        for (int s=0;s<solids_count;s++){
            if (aabb_int(e->r, solids[s])) {
                /* undo and flip */
                e->r.x = (int)roundf(oldx);
                e->dir *= -1;
                break;
            }
        }
        /* basic ground ahead check; if no block below ahead, flip */
        int ahead_x = e->r.x + (e->dir>0? e->r.w + 2 : -4);
        int foot_y = e->r.y + e->r.h + 2;
        SDL_Rect foot = {ahead_x, foot_y, 2, 2};
        int found=0;
        for (int s=0;s<solids_count;s++){
            if (aabb_int(foot, solids[s])) { found=1; break; }
        }
        if (!found) e->dir *= -1;
    }
}

/* ---------------- RENDER helpers ---------------- */
void draw_level(SDL_Renderer *ren, int camx) {
    /* solids */
    for (int i=0;i<solids_count;i++){
        SDL_Rect r = solids[i];
        r.x -= camx;
        draw_rect(ren, r.x, r.y, r.w, r.h, BLOCK);
        if (r.h == TILE) {
            draw_rect(ren, r.x, r.y, r.w, 6, (SDL_Color){230,150,90,255});
        }
    }
    /* grass caps (visual) - scan level rows */
    /* not drawing separate t-caps here since solids include t */
}

void draw_player(SDL_Renderer *ren, Player *p, int camx) {
    SDL_FRect pr = p->r;
    SDL_Rect rr = {(int)roundf(pr.x)-camx, (int)roundf(pr.y), (int)roundf(pr.w), (int)roundf(pr.h)};
    SDL_Color col = p->big ? PLAYER_BIG_COL : PLAYER_COL;
    draw_rect(ren, rr.x, rr.y, rr.w, rr.h, col);
    /* small hat */
    draw_rect(ren, rr.x+4, rr.y-6, rr.w-8, 6, (SDL_Color){30,30,30,255});
}

void draw_enemy(SDL_Renderer *ren, Enemy *e, int camx) {
    SDL_Rect rr = e->r;
    rr.x -= camx;
    draw_rect(ren, rr.x, rr.y, rr.w, rr.h, ENEMY_COL);
    draw_rect(ren, rr.x + rr.w/2 - 2, rr.y + 8, 4, 4, (SDL_Color){250,240,220,255});
}

void draw_coin(SDL_Renderer *ren, Coin *c, int camx) {
    SDL_Rect rr = c->r;
    rr.x -= camx;
    draw_rect(ren, rr.x, rr.y, rr.w, rr.h, COIN_COL);
}

void draw_mush(SDL_Renderer *ren, Mushroom *m, int camx) {
    SDL_Rect rr = m->r;
    rr.x -= camx;
    /* cap */
    draw_rect(ren, rr.x, rr.y, rr.w, rr.h/2, MUSH_COL);
    /* stem */
    draw_rect(ren, rr.x + rr.w/4, rr.y + rr.h/4, rr.w/2, rr.h/2, (SDL_Color){250,230,200,255});
}

void draw_flag(SDL_Renderer *ren, SDL_Rect flag, int camx) {
    SDL_Rect fr = flag;
    fr.x -= camx;
    /* pole */
    draw_rect(ren, fr.x + fr.w/2 - 2, fr.y + fr.h - 4*TILE, 4, 4*TILE, (SDL_Color){220,220,220,255});
    /* flag */
    SDL_Point pts[3] = {{fr.x + fr.w/2 + 4, fr.y + fr.h - 4*TILE + 30}, {fr.x + fr.w/2 + 38, fr.y + fr.h - 4*TILE + 18}, {fr.x + fr.w/2 + 38, fr.y + fr.h - 4*TILE + 42}};
    /* draw as filled triangle using rects (simple) */
    draw_rect(ren, pts[0].x, pts[0].y, 34, 24, FLAG_COL);
}

/* ---------------- GAME INIT / START ---------------- */
void start_level(int idx, Player *pl, int *level_time, Uint32 *t0) {
    build_level(LEVELS[idx], 8);
    /* spawn player at left safe position */
    int spawnx = 60;
    int spawny = 0;
    SDL_FRect temp = {.x = (float)spawnx, .y = (float)spawny, .w = TILE-12, .h = TILE-8};
    float vy = 0;
    /* drop until hitting ground */
    for (int iter=0; iter<2000; iter++){
        vy += 1.0f;
        temp.y += vy;
        SDL_Rect test = {(int)roundf(temp.x),(int)roundf(temp.y),(int)roundf(temp.w),(int)roundf(temp.h)};
        int hit = 0;
        for (int s=0;s<solids_count;s++){
            if (aabb_int(test, solids[s])) { hit = 1; break; }
        }
        if (hit) {
            /* snap above */
            for (int s=0;s<solids_count;s++){
                if (aabb_int(test, solids[s])) {
                    temp.y = solids[s].y - temp.h;
                    break;
                }
            }
            break;
        }
    }
    pl->r.x = temp.x; pl->r.y = temp.y; pl->r.w = temp.w; pl->r.h = temp.h;
    pl->vx = pl->vy = 0;
    pl->spawn.x = pl->r.x; pl->spawn.y = pl->r.y;
    *level_time = 300;
    *t0 = SDL_GetTicks();
}

/* ---------------- MAIN ---------------- */
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL Init error: %s\n", SDL_GetError()); return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF Init error: %s\n", TTF_GetError()); SDL_Quit(); return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Retro Platformer (C / SDL2)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    if (!win) { fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError()); TTF_Quit(); SDL_Quit(); return 1; }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "CreateRenderer failed: %s\n", SDL_GetError()); SDL_DestroyWindow(win); TTF_Quit(); SDL_Quit(); return 1; }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18);
    if (!font) {
        /* try fallback to bundled font path on some Windows setups; if missing, we will continue without text */
        font = NULL;
    }

    /* game state */
    enum {STATE_TITLE, STATE_PLAY, STATE_LEVEL_CLEAR, STATE_GAME_OVER, STATE_WIN} state = STATE_TITLE;
    int level_idx = 0;
    Player player = {0};
    player.coins = 0; player.score = 0; player.lives = 3; player.big = 0; player.big_timer = 0;
    player.r.w = TILE-12; player.r.h = TILE-8;
    int level_time = 300;
    Uint32 t0 = 0;

    start_level(level_idx, &player, &level_time, &t0);

    int running = 1;
    Uint32 last_tick = SDL_GetTicks();

    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - last_tick) / 1000.0f;
        if (dt < 1.0f / FPS) {
            SDL_Delay((Uint32)((1.0f/FPS - dt)*1000));
            now = SDL_GetTicks();
            dt = (now - last_tick) / 1000.0f;
        }
        last_tick = now;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = 0; }
            else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE) { running = 0; }
                if (state == STATE_TITLE && k == SDLK_RETURN) {
                    state = STATE_PLAY;
                    level_idx = 0;
                    player.score = player.coins = 0;
                    player.lives = 3;
                    start_level(level_idx, &player, &level_time, &t0);
                } else if (state == STATE_LEVEL_CLEAR && k == SDLK_RETURN) {
                    level_idx++;
                    if (level_idx >= NUM_LEVELS) {
                        state = STATE_WIN;
                    } else {
                        start_level(level_idx, &player, &level_time, &t0);
                        state = STATE_PLAY;
                    }
                } else if (state == STATE_GAME_OVER && k == SDLK_RETURN) {
                    /* restart */
                    state = STATE_PLAY;
                    level_idx = 0; player.score = 0; player.coins = 0; player.lives = 3;
                    start_level(level_idx, &player, &level_time, &t0);
                } else if (k == SDLK_r) {
                    start_level(level_idx, &player, &level_time, &t0);
                } else if (k == SDLK_z || k == SDLK_SPACE || k == SDLK_UP) {
                    if (state == STATE_PLAY) {
                        if (player.on_ground) player.vy = JUMP_VEL * (player.big ? 0.95f : 1.0f);
                    }
                }
            }
        }

        const Uint8 *keystate = SDL_GetKeyboardState(NULL);
        if (state == STATE_PLAY) {
            /* input horizontal */
            float ax = 0;
            if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A]) ax -= 0.9f;
            if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) ax += 0.9f;
            player.vx += ax;
            if (player.vx > MAX_XSPEED) player.vx = MAX_XSPEED;
            if (player.vx < -MAX_XSPEED) player.vx = -MAX_XSPEED;
            /* gravity */
            player.vy += GRAVITY;
            if (player.vy > 20) player.vy = 20;
            /* collisions */
            resolve_horz_collision(&player);
            resolve_vert_collision(&player);
            /* friction */
            if (player.on_ground && fabsf(player.vx) > 0.01f) { player.vx *= 0.82f; if (fabsf(player.vx) < 0.1f) player.vx = 0; }

            /* coins */
            for (int i=0;i<coin_count;i++){
                if (!coins[i].active) continue;
                SDL_Rect pr = {(int)roundf(player.r.x),(int)roundf(player.r.y),(int)roundf(player.r.w),(int)roundf(player.r.h)};
                if (aabb_int(pr, coins[i].r)) {
                    coins[i].active = 0;
                    player.coins++;
                    player.score += 100;
                }
            }
            /* mushrooms */
            for (int i=0;i<mush_count;i++){
                if (!mush[i].active) continue;
                SDL_Rect pr = {(int)roundf(player.r.x),(int)roundf(player.r.y),(int)roundf(player.r.w),(int)roundf(player.r.h)};
                if (aabb_int(pr, mush[i].r)) {
                    mush[i].active = 0;
                    if (!player.big) {
                        /* grow */
                        player.big = 1;
                        player.r.h += TILE/2;
                        player.r.y -= TILE/2;
                        player.big_timer = 12.0f;
                        player.score += 500;
                    } else {
                        /* already big -> give points */
                        player.score += 200;
                    }
                }
            }
            /* enemies update */
            update_enemies(dt);
            /* interactions with enemies */
            for (int i=0;i<enemy_count;i++){
                if (!enemies[i].active) continue;
                SDL_Rect er = enemies[i].r;
                SDL_Rect pr = {(int)roundf(player.r.x),(int)roundf(player.r.y),(int)roundf(player.r.w),(int)roundf(player.r.h)};
                if (aabb_int(pr, er)) {
                    /* stomp if falling and near top */
                    if (player.vy > 0 && (player.r.y + player.r.h) - er.y < 16.0f) {
                        enemies[i].active = 0;
                        player.vy = JUMP_VEL * 0.6f;
                        player.score += 200;
                    } else {
                        if (player.big) {
                            /* shrink */
                            player.big = 0;
                            player.r.h -= TILE/2;
                            player.r.y += TILE/2;
                        } else {
                            /* lose life and respawn */
                            player.lives--;
                            player.r.x = player.spawn.x;
                            player.r.y = player.spawn.y;
                            player.vx = player.vy = 0;
                            if (player.lives <= 0) state = STATE_GAME_OVER;
                        }
                    }
                }
            }
            /* fall off screen */
            if (player.r.y > SCREEN_H + 200) {
                player.lives--;
                player.r.x = player.spawn.x;
                player.r.y = player.spawn.y;
                player.vx = player.vy = 0;
                if (player.lives <= 0) state = STATE_GAME_OVER;
            }
            /* check flag / goal */
            if (goal_exists) {
                SDL_Rect pr = {(int)roundf(player.r.x),(int)roundf(player.r.y),(int)roundf(player.r.w),(int)roundf(player.r.h)};
                SDL_Rect gr = goal_rect;
                if (aabb_int(pr, gr)) {
                    state = STATE_LEVEL_CLEAR;
                }
            }
            /* big timer */
            if (player.big) {
                player.big_timer -= 1.0f / FPS;
                if (player.big_timer <= 0) {
                    player.big = 0;
                    player.r.h -= TILE/2;
                    player.r.y += TILE/2;
                }
            }
            /* level timer */
            int time_left = level_time - (int)((SDL_GetTicks() - t0) / 1000);
            if (time_left <= 0) {
                /* out of time: lose a life and reset level start time */
                player.lives--;
                player.r.x = player.spawn.x;
                player.r.y = player.spawn.y;
                player.vx = player.vy = 0;
                t0 = SDL_GetTicks();
                if (player.lives <= 0) state = STATE_GAME_OVER;
            }
        } /* end STATE_PLAY handling */

        /* render */
        SDL_SetRenderDrawColor(ren, SKY.r, SKY.g, SKY.b, SKY.a);
        SDL_RenderClear(ren);

        if (state == STATE_TITLE) {
            /* simple title screen */
            draw_rect(ren, 180, 100, 600, 80, (SDL_Color){255,255,255,255});
            if (font) {
                SDL_Surface *s = TTF_RenderText_Blended(font, "RETRO PLATFORMER (C / SDL2) - Press Enter to Start", HUD_COL);
                if (s) {
                    SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, s);
                    int w,h; SDL_QueryTexture(tx, NULL, NULL, &w, &h);
                    SDL_Rect dst = { (SCREEN_W-w)/2, 240, w, h };
                    SDL_RenderCopy(ren, tx, NULL, &dst);
                    SDL_DestroyTexture(tx);
                    SDL_FreeSurface(s);
                }
            }
        } else if (state == STATE_PLAY || state == STATE_LEVEL_CLEAR || state == STATE_GAME_OVER || state == STATE_WIN) {
            /* camera */
            int camx = (int)(player.r.x + player.r.w/2) - SCREEN_W/2;
            if (camx < 0) camx = 0;
            if (camx > world_width - SCREEN_W) camx = world_width - SCREEN_W;
            /* background hills */
            for (int i=-2;i<12;i++){
                int bx = i*300 - (camx/2 % 600);
                SDL_SetRenderDrawColor(ren, 70,160,90,255);
                SDL_Rect hill = {bx, SCREEN_H-80, 220, 80};
                SDL_RenderFillRect(ren, &hill);
            }
            /* draw level tiles */
            draw_level(ren, camx);
            /* draw coins */
            for (int i=0;i<coin_count;i++){
                if (coins[i].active) draw_coin(ren, &coins[i], camx);
            }
            /* mushrooms */
            for (int i=0;i<mush_count;i++){
                if (mush[i].active) draw_mush(ren, &mush[i], camx);
            }
            /* enemies */
            for (int i=0;i<enemy_count;i++){
                if (enemies[i].active) draw_enemy(ren, &enemies[i], camx);
            }
            /* flag */
            if (goal_exists) draw_flag(ren, goal_rect, camx);
            /* player */
            draw_player(ren, &player, camx);

            /* HUD */
            if (font) {
                char buf[256];
                int time_left = level_time - (int)((SDL_GetTicks() - t0) / 1000);
                snprintf(buf, sizeof(buf), "LEVEL %d    SCORE %06d    COINS %02d    LIVES %d    TIME %03d",
                         level_idx+1, player.score, player.coins, player.lives, time_left);
                SDL_Surface *s = TTF_RenderText_Blended(font, buf, HUD_COL);
                if (s) {
                    SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, s);
                    SDL_Rect dst = {12, 10, s->w, s->h};
                    SDL_RenderCopy(ren, tx, NULL, &dst);
                    SDL_DestroyTexture(tx);
                    SDL_FreeSurface(s);
                }
            }
            /* small message if big */
            if (player.big && font) {
                SDL_Surface *s = TTF_RenderText_Blended(font, "MUSHROOM: BIG!", (SDL_Color){10,10,10,255});
                if (s) {
                    SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, s);
                    SDL_Rect dst = {SCREEN_W - s->w - 12, 10, s->w, s->h};
                    SDL_RenderCopy(ren, tx, NULL, &dst);
                    SDL_DestroyTexture(tx);
                    SDL_FreeSurface(s);
                }
            }

            if (state == STATE_LEVEL_CLEAR) {
                if (font) {
                    SDL_Surface *s = TTF_RenderText_Blended(font, "COURSE CLEAR! Press Enter to continue", (SDL_Color){255,255,255,255});
                    if (s) {
                        SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, s);
                        SDL_Rect dst = {(SCREEN_W - s->w)/2, SCREEN_H/3, s->w, s->h};
                        SDL_RenderCopy(ren, tx, NULL, &dst);
                        SDL_DestroyTexture(tx);
                        SDL_FreeSurface(s);
                    }
                }
            } else if (state == STATE_GAME_OVER) {
                if (font) {
                    SDL_Surface *s = TTF_RenderText_Blended(font, "GAME OVER - Press Enter to Restart", (SDL_Color){255,255,255,255});
                    if (s) {
                        SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, s);
                        SDL_Rect dst = {(SCREEN_W - s->w)/2, SCREEN_H/3, s->w, s->h};
                        SDL_RenderCopy(ren, tx, NULL, &dst);
                        SDL_DestroyTexture(tx);
                        SDL_FreeSurface(s);
                    }
                }
            } else if (state == STATE_WIN) {
                if (font) {
                    SDL_Surface *s = TTF_RenderText_Blended(font, "YOU WIN! Thanks for playing", (SDL_Color){255,255,255,255});
                    if (s) {
                        SDL_Texture *tx = SDL_CreateTextureFromSurface(ren, s);
                        SDL_Rect dst = {(SCREEN_W - s->w)/2, SCREEN_H/3, s->w, s->h};
                        SDL_RenderCopy(ren, tx, NULL, &dst);
                        SDL_DestroyTexture(tx);
                        SDL_FreeSurface(s);
                    }
                }
            }
        }

        SDL_RenderPresent(ren);

        /* simple state advancement: go to WIN when level cleared and last level was done */
        if (state == STATE_LEVEL_CLEAR) {
            /* nothing automatic; waiting for Enter key to continue (handled in event loop) */
        }

    } /* main loop */

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
