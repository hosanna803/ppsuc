"""
retro_mario_clone.py
A single-file retro-style platformer inspired by old Super Mario Bros.
Requires: pygame (pip install pygame)
Controls:
    Left/Right / A/D : move
    Z / Space / Up  : jump
    R               : restart level (or restart from title on game over)
    Enter / Return  : start / proceed on title/win/gameover
    Esc             : quit

Features:
- Title screen, 3 levels, level progression
- Coins, enemies, mushroom power-up (grow), flag goal
- HUD: score, coins, lives, level timer
- No external assets or sounds — sprites are drawn with code
"""
import pygame, sys, random
from pygame import Rect

# ---------- CONFIG ----------
WIDTH, HEIGHT = 960, 540
FPS = 60
TILE = 48
GRAVITY = 0.6
MAX_XSPEED = 6.0
JUMP_VEL = -12.0
FONT_NAME = None  # default system font

# Colors (retro palette)
SKY = (120, 200, 255)
GROUND = (92, 56, 28)
BLOCK = (200, 120, 60)
COIN_COLOR = (255, 215, 0)
PLAYER_COLOR = (200, 30, 40)
PLAYER_BIG_COLOR = (255, 90, 90)
ENEMY_COLOR = (120, 60, 20)
MUSHROOM_COLOR = (180, 40, 40)
FLAG_COLOR = (50, 200, 100)
HUD_COLOR = (30, 30, 30)

# ---------- LEVELS (3 levels) ----------
# Legend:
# 'X' block, '=' ground (solid)
# 't' top grass cap (visual)
# 'C' coin
# 'E' enemy
# 'M' mushroom power-up
# 'F' flag (goal)
# ' ' empty
LEVELS = [
    [
    "                                                                                ",
    "                                                                                ",
    "                                                                                ",
    "                   C                                                            ",
    "        C                                                                       F",
    "    C       E                 C                                                 ",
    "XXXXXXXXXXXX      XXXX     XXXXXXXX      C                     C               ",
    "==========ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt",
    ],
    [ # level 2
    "                                                                                ",
    "                                                                                ",
    "        C                               C                                       ",
    "                E                 C                                              ",
    "    XXXXX        XXXXXX      XXXXXXXX              C               F           ",
    "          C               C         E                                     C    ",
    "   C    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   C                    ",
    "==========ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt",
    ],
    [ # level 3 - trickier
    "                                                                                ",
    "                                                                                ",
    "                   C               E                                            ",
    "        M       XXXX      C      XXXXX       C                          F        ",
    "    C           X  X              X   X                E                 C      ",
    "XXXXXXXXXXXX    X  X    C    C    X   X    C    C    X   X    C    C    XXX    ",
    "==========ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt",
    ],
]

# ---------- ENTITIES ----------
class Entity:
    def __init__(self, x, y, w, h):
        self.rect = Rect(x, y, w, h)
        self.vx = 0.0
        self.vy = 0.0
        self.on_ground = False
        self.dead = False

    def aabb_collide(self, solids):
        # Horizontal
        self.rect.x += int(self.vx)
        hits = [b for b in solids if self.rect.colliderect(b)]
        for b in hits:
            if self.vx > 0:
                self.rect.right = b.left
            elif self.vx < 0:
                self.rect.left = b.right
            self.vx = 0
        # Vertical
        self.rect.y += int(self.vy)
        hits = [b for b in solids if self.rect.colliderect(b)]
        self.on_ground = False
        for b in hits:
            if self.vy > 0:
                self.rect.bottom = b.top
                self.on_ground = True
            elif self.vy < 0:
                self.rect.top = b.bottom
            self.vy = 0

class Player(Entity):
    def __init__(self, x, y):
        super().__init__(x, y, TILE-12, TILE-8)
        self.spawn = (x, y)
        self.coins = 0
        self.score = 0
        self.lives = 3
        self.big = False
        self.big_timer = 0.0

    def update(self, keys, solids):
        ax = 0.0
        if keys[pygame.K_LEFT] or keys[pygame.K_a]:
            ax -= 0.9
        if keys[pygame.K_RIGHT] or keys[pygame.K_d]:
            ax += 0.9
        # apply horizontal accel and clamp
        self.vx = self.vx + ax
        if self.vx > MAX_XSPEED: self.vx = MAX_XSPEED
        if self.vx < -MAX_XSPEED: self.vx = -MAX_XSPEED
        # gravity
        self.vy += GRAVITY
        if self.vy > 20: self.vy = 20
        # collide
        self.aabb_collide(solids)
        # friction when grounded
        if self.on_ground and abs(self.vx) > 0:
            self.vx *= 0.82
            if abs(self.vx) < 0.1: self.vx = 0
        # big timer
        if self.big:
            self.big_timer -= 1.0 / FPS
            if self.big_timer <= 0:
                self.shrink()

    def jump(self):
        if self.on_ground:
            self.vy = JUMP_VEL if not self.big else JUMP_VEL * 0.95

    def grow(self):
        if not self.big:
            # grow: increase height and adjust rect so player stands at same base
            self.big = True
            self.rect.inflate_ip(0, TILE//2)  # grow taller
            self.rect.y -= TILE//2
            self.big_timer = 12.0  # seconds of big before shrinking (can be extended)
            # add score
            self.score += 500

    def shrink(self):
        if self.big:
            self.big = False
            self.rect.inflate_ip(0, -TILE//2)
            self.rect.y += TILE//2

    def kill(self):
        self.lives -= 1
        self.rect.topleft = self.spawn
        self.vx = self.vy = 0
        self.big = False
        self.big_timer = 0

class Enemy(Entity):
    def __init__(self, x, y):
        super().__init__(x, y, TILE-14, TILE-16)
        self.dir = -1
        self.speed = 1.2

    def update(self, solids):
        self.vx = self.dir * self.speed
        self.vy += GRAVITY
        if self.vy > 20: self.vy = 20
        self.aabb_collide(solids)
        # ground/ledge check: if blocked or no ground ahead, turn
        ahead = self.rect.move(self.dir*4, 0)
        ground_check = ahead.move(0, 2)
        on_ground = any(ground_check.move(0, TILE//2).colliderect(s) for s in solids)
        if self.vx == 0 or not on_ground:
            self.dir *= -1

# ---------- LEVEL BUILD ----------
def build_level(level_map):
    solids = []
    coins = []
    enemies = []
    mushrooms = []
    goal = None
    for j, row in enumerate(level_map):
        for i, ch in enumerate(row):
            x, y = i*TILE, j*TILE
            if ch in ('X', '='):
                solids.append(Rect(x, y, TILE, TILE))
            elif ch == 't':
                # visual top grass strip (we'll also add a thin block to prevent falling through)
                solids.append(Rect(x, y+TILE//2, TILE, TILE//2))
            elif ch == 'C':
                coins.append(Rect(x+TILE//4, y+TILE//4, TILE//2, TILE//2))
            elif ch == 'E':
                enemies.append(Enemy(x+6, y+8))
            elif ch == 'M':
                mushrooms.append(Rect(x+12, y+12, TILE-24, TILE-24))
            elif ch == 'F':
                # tall flagpole region (player touching it triggers level clear)
                goal = Rect(x+TILE//2-6, y-4*TILE, 12, 4*TILE)
    return solids, coins, enemies, mushrooms, goal

# ---------- DRAW HELPERS ----------
def draw_clouds(surf, camx):
    for i in range(-1, 12):
        cx = i*260 - int(camx*0.25) % 2600
        pygame.draw.ellipse(surf, (255,255,255), (cx, 60, 120, 50))
        pygame.draw.ellipse(surf, (255,255,255), (cx+30, 40, 140, 70))

def draw_tile(surf, r):
    # draw a block with bevel
    pygame.draw.rect(surf, BLOCK, r)
    pygame.draw.rect(surf, (230,150,90), (r.x, r.y, r.w, 6))

def draw_level(surf, solids, level_map, camx):
    for s in solids:
        rr = s.move(-camx, 0)
        draw_tile(surf, rr)
    # draw grass caps where 't'
    for j, row in enumerate(level_map):
        for i, ch in enumerate(row):
            if ch == 't':
                x, y = i*TILE-camx, j*TILE
                pygame.draw.rect(surf, (70, 180, 70), (x, y, TILE, 10), border_radius=3)

def draw_player(surf, player, camx):
    pr = player.rect.move(-camx, 0)
    # pixel / blocky silhouette
    color = PLAYER_BIG_COLOR if player.big else PLAYER_COLOR
    # small shadow
    pygame.draw.rect(surf, (0,0,0,40), (pr.x+2, pr.bottom-4, pr.w, 4))
    # body
    pygame.draw.rect(surf, color, pr, border_radius=4)
    # simple hat / head
    pygame.draw.rect(surf, (30,30,30), (pr.x+4, pr.y-10, pr.w-8, 6), border_radius=3)

def draw_enemy(surf, enemy, camx):
    er = enemy.rect.move(-camx, 0)
    pygame.draw.rect(surf, ENEMY_COLOR, er, border_radius=6)
    # eye
    pygame.draw.rect(surf, (250,240,220), (er.centerx-6, er.y+8, 4, 4))

def draw_coin(surf, c, camx):
    cr = c.move(-camx, 0)
    pygame.draw.ellipse(surf, COIN_COLOR, cr)
    pygame.draw.ellipse(surf, (255,255,255), (cr.x+4, cr.y+4, cr.w//3, cr.h//3))

def draw_mushroom(surf, m, camx):
    mr = m.move(-camx, 0)
    # cap
    pygame.draw.ellipse(surf, MUSHROOM_COLOR, (mr.x, mr.y, mr.w, mr.h//2))
    # stem
    pygame.draw.rect(surf, (250,230,200), (mr.x+mr.w//4, mr.y+mr.h//4, mr.w//2, mr.h//2))

def draw_flag(surf, flag, camx):
    if not flag: return
    f = flag.move(-camx, 0)
    pole = Rect(f.centerx-2, f.bottom-4*TILE, 4, 4*TILE)
    pygame.draw.rect(surf, (220,220,220), pole)
    pygame.draw.polygon(surf, FLAG_COLOR, [(pole.right, pole.y+30), (pole.right+34, pole.y+18), (pole.right+34, pole.y+42)])

def draw_hud(surf, font, player, coins_collected, level_idx, time_left):
    text = f"LEVEL {level_idx+1}    SCORE {player.score:06d}    COINS {coins_collected:02d}    LIVES {player.lives}    TIME {int(time_left):03d}"
    img = font.render(text, True, HUD_COLOR)
    surf.blit(img, (12, 10))

# ---------- MAIN GAME ----------
def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Retro Platformer — Retro Mario-like")
    clock = pygame.time.Clock()
    font_small = pygame.font.SysFont(FONT_NAME, 20, bold=True)
    font_big = pygame.font.SysFont(FONT_NAME, 44, bold=True)

    # preload levels
    built = [build_level(l) for l in LEVELS]

    # game state
    game_state = "TITLE"  # TITLE, PLAYING, LEVEL_CLEAR, GAME_OVER, WIN
    level_idx = 0
    solids, coins, enemies, mushrooms, goal = built[level_idx]
    world_width = len(LEVELS[level_idx][0]) * TILE

    # find spawn: leftmost open spot above ground
    spawn_x = 60
    spawn_y = 0
    # simulate drop to find ground below spawn_x
    temp = Rect(spawn_x, spawn_y, TILE-12, TILE-8)
    vy = 0
    while True:
        vy += 1
        temp.y += vy
        if any(temp.colliderect(s) for s in solids):
            temp.bottom = min(s.top for s in solids if temp.colliderect(s))
            break
        if temp.y > HEIGHT:
            break

    player = Player(spawn_x, temp.bottom - (TILE-8))
    coins_collected = 0
    level_time = 300  # seconds
    t0 = 0

    won_all = False

    def start_level(idx):
        nonlocal solids, coins, enemies, mushrooms, goal, world_width, player, coins_collected, t0
        solids, coins, enemies, mushrooms, goal = built[idx]
        world_width = len(LEVELS[idx][0]) * TILE
        # respawn player near left
        spawn_x = 60
        spawn_y = 0
        temp = Rect(spawn_x, spawn_y, TILE-12, TILE-8)
        vy = 0
        while True:
            vy += 1
            temp.y += vy
            if any(temp.colliderect(s) for s in solids):
                temp.bottom = min(s.top for s in solids if temp.colliderect(s))
                break
            if temp.y > HEIGHT:
                break
        player.rect.topleft = (spawn_x, temp.bottom - (TILE-8))
        player.vx = player.vy = 0
        player.spawn = (player.rect.x, player.rect.y)
        coins_collected = 0
        # reset coins/enemies/mushrooms from built copy
        # (built already used as fresh objects)
        t0 = pygame.time.get_ticks()

    # Title screen loop until Enter pressed
    while True:
        dt = clock.tick(FPS) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit(); sys.exit()
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_RETURN or event.key == pygame.K_KP_ENTER:
                    # start game
                    game_state = "PLAYING"
                    level_idx = 0
                    player = Player(60, 0)
                    start_level(level_idx)
                if event.key == pygame.K_ESCAPE:
                    pygame.quit(); sys.exit()

        # Render title
        screen.fill(SKY)
        draw_clouds(screen, 0)
        title = font_big.render("RETRO PLATFORMER", True, HUD_COLOR)
        screen.blit(title, (WIDTH//2 - title.get_width()//2, 100))
        sub = font_small.render("A small Mario-like retro platformer (no sounds). Press Enter to start.", True, HUD_COLOR)
        screen.blit(sub, (WIDTH//2 - sub.get_width()//2, 200))
        info = font_small.render("Controls: ← → (A D), Z/Space/Up to jump, R restart, Esc quit", True, HUD_COLOR)
        screen.blit(info, (WIDTH//2 - info.get_width()//2, 260))
        hint = font_small.render("Collect coins, get mushrooms to grow, stomp enemies, reach the flag.", True, HUD_COLOR)
        screen.blit(hint, (WIDTH//2 - hint.get_width()//2, 300))
        pygame.display.flip()

        if game_state != "TITLE":
            break

    # Main game loop
    running = True
    won_all = False
    while running:
        dt = clock.tick(FPS) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key in (pygame.K_z, pygame.K_SPACE, pygame.K_UP):
                    if game_state == "PLAYING":
                        player.jump()
                if event.key == pygame.K_r:
                    # restart current level
                    start_level(level_idx)
                    player.lives = max(1, player.lives)
                if event.key == pygame.K_ESCAPE:
                    running = False
                if event.key == pygame.K_RETURN and game_state in ("LEVEL_CLEAR", "GAME_OVER", "WIN"):
                    if game_state == "LEVEL_CLEAR":
                        # next level or win
                        level_idx += 1
                        if level_idx >= len(LEVELS):
                            game_state = "WIN"
                        else:
                            start_level(level_idx)
                            game_state = "PLAYING"
                    elif game_state == "WIN":
                        # go to title
                        pygame.quit(); sys.exit()
                    elif game_state == "GAME_OVER":
                        # restart whole game
                        game_state = "PLAYING"
                        level_idx = 0
                        player = Player(60, 0)
                        start_level(level_idx)
                        player.lives = 3

        if game_state == "PLAYING":
            keys = pygame.key.get_pressed()
            player.update(keys, solids)

            # collect coins
            got = [c for c in coins if player.rect.colliderect(c)]
            for c in got:
                coins.remove(c)
                player.coins += 1
                coins_collected += 1
                player.score += 100

            # collect mushrooms
            gotm = [m for m in mushrooms if player.rect.colliderect(m)]
            for m in gotm:
                mushrooms.remove(m)
                player.grow()

            # enemies update
            for e in enemies:
                e.update(solids)
            # interactions with enemies
            for e in enemies[:]:
                if player.rect.colliderect(e.rect):
                    # if falling and near top of enemy => stomp
                    if player.vy > 0 and player.rect.bottom - e.rect.top < 16:
                        enemies.remove(e)
                        player.vy = JUMP_VEL * 0.6
                        player.score += 200
                    else:
                        # hit enemy: if big shrink, else lose a life
                        if player.big:
                            player.shrink()
                        else:
                            player.kill()
                            if player.lives <= 0:
                                game_state = "GAME_OVER"

            # fall off screen
            if player.rect.top > HEIGHT + 200:
                player.kill()
                if player.lives <= 0:
                    game_state = "GAME_OVER"

            # win condition
            time_left = level_time - (pygame.time.get_ticks() - t0) / 1000.0
            if time_left <= 0:
                # out of time => kill
                player.kill()
                t0 = pygame.time.get_ticks()
                if player.lives <= 0:
                    game_state = "GAME_OVER"
                time_left = level_time

            if goal and player.rect.colliderect(goal):
                # level clear
                game_state = "LEVEL_CLEAR"

            # camera
            camx = max(0, min(player.rect.centerx - WIDTH//2, world_width - WIDTH))

            # DRAW playing
            screen.fill(SKY)
            draw_clouds(screen, camx)
            # hills background
            for i in range(-2, 12):
                bx = i*300 - int(camx*0.5) % 6000
                pygame.draw.ellipse(screen, (70,160,90), (bx, HEIGHT-80, 220, 80))
            draw_level(screen, solids, LEVELS[level_idx], camx)

            for c in coins:
                draw_coin(screen, c, camx)
            for m in mushrooms:
                draw_mushroom(screen, m, camx)
            for e in enemies:
                draw_enemy(screen, e, camx)
            draw_flag(screen, goal, camx)
            draw_player(screen, player, camx)
            draw_hud(screen, font_small, player, coins_collected, level_idx, time_left)

            if player.big:
                btxt = font_small.render("MUSHROOM: BIG!", True, (10,10,10))
                screen.blit(btxt, (WIDTH-180, 12))

            pygame.display.flip()

        elif game_state == "LEVEL_CLEAR":
            # simple clear screen, show message and wait for Enter
            screen.fill(SKY)
            draw_clouds(screen, 0)
            # show "Course Clear" and stats
            msg = font_big.render("COURSE CLEAR!", True, HUD_COLOR)
            screen.blit(msg, (WIDTH//2 - msg.get_width()//2, HEIGHT//3))
            sub = font_small.render(f"Score: {player.score:06d}    Coins: {player.coins:02d}    Lives: {player.lives}", True, HUD_COLOR)
            screen.blit(sub, (WIDTH//2 - sub.get_width()//2, HEIGHT//3 + 80))
            tip = font_small.render("Press Enter to continue", True, HUD_COLOR)
            screen.blit(tip, (WIDTH//2 - tip.get_width()//2, HEIGHT//3 + 140))
            pygame.display.flip()

        elif game_state == "GAME_OVER":
            screen.fill(SKY)
            msg = font_big.render("GAME OVER", True, HUD_COLOR)
            screen.blit(msg, (WIDTH//2 - msg.get_width()//2, HEIGHT//3))
            sub = font_small.render(f"Final Score: {player.score:06d}", True, HUD_COLOR)
            screen.blit(sub, (WIDTH//2 - sub.get_width()//2, HEIGHT//3 + 80))
            tip = font_small.render("Press Enter to restart (or Esc to quit)", True, HUD_COLOR)
            screen.blit(tip, (WIDTH//2 - tip.get_width()//2, HEIGHT//3 + 140))
            pygame.display.flip()

        elif game_state == "WIN":
            screen.fill(SKY)
            msg = font_big.render("YOU WIN!", True, HUD_COLOR)
            screen.blit(msg, (WIDTH//2 - msg.get_width()//2, HEIGHT//3))
            sub = font_small.render(f"Final Score: {player.score:06d}    Coins: {player.coins:02d}", True, HUD_COLOR)
            screen.blit(sub, (WIDTH//2 - sub.get_width()//2, HEIGHT//3 + 80))
            tip = font_small.render("Thanks for playing! Press Enter to exit.", True, HUD_COLOR)
            screen.blit(tip, (WIDTH//2 - tip.get_width()//2, HEIGHT//3 + 140))
            pygame.display.flip()

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()

