#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define WIDTH       60
#define HEIGHT      26
#define ROWS         5
#define COLS        11
#define MAX_BULLETS  3
#define MAX_BOMBS    5

void gotoxy(int x, int y) {
    COORD c = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}
void hide_cursor() {
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}
void set_color(int c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}
void cls() { system("cls"); }

// ─── Alien grid ───────────────────────────────────────────────────────────────
// Each alien: alive, x, y, type (row 0=top hardest, row 4=bottom easiest)
typedef struct { int alive, x, y, type; } Alien;
Alien grid[ROWS][COLS];

int alien_dir;          // +1 right, -1 left
int alien_tick;         // counter for movement speed
int alien_speed;        // ticks per step (lower = faster)
int aliens_alive;

// ─── Player ───────────────────────────────────────────────────────────────────
int px;                 // player x (centre of ship)
int lives, score, high_score, level;

// ─── Bullets (player) ─────────────────────────────────────────────────────────
int bx[MAX_BULLETS], by[MAX_BULLETS], bactive[MAX_BULLETS];
int shoot_cooldown;

// ─── Bombs (alien) ────────────────────────────────────────────────────────────
int dx[MAX_BOMBS], dy[MAX_BOMBS], dactive[MAX_BOMBS];
int bomb_tick, bomb_speed;

// ─── Shields (4 bunkers, each 4×3 block of cells) ────────────────────────────
// shield_hp[s][row][col]: 2=solid, 1=damaged, 0=gone
int shield_hp[4][3][4];
int shield_ox[4]; // x origin of each shield

// ─── UFO ──────────────────────────────────────────────────────────────────────
int ufo_x, ufo_alive, ufo_dir, ufo_tick;

// ─── State ────────────────────────────────────────────────────────────────────
int paused, game_over, wave_clear;

// ══════════════════════════════════════════════════════════════════════════════
void erase(int x, int y, int len) {
    gotoxy(x, y);
    set_color(0);
    for (int i = 0; i < len; i++) putchar(' ');
}

void init_shields() {
    // 4 shields evenly spaced
    int positions[4] = {4, 17, 30, 43};
    for (int s = 0; s < 4; s++) {
        shield_ox[s] = positions[s];
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 4; c++)
                shield_hp[s][r][c] = 2;
    }
}

void init_aliens() {
    // Start grid top-left at x=4, y=3
    int start_x = 4, start_y = 3;
    aliens_alive = 0;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            grid[r][c].alive = 1;
            grid[r][c].x     = start_x + c * 5;
            grid[r][c].y     = start_y + r * 2;
            grid[r][c].type  = r; // 0=top, 4=bottom
            aliens_alive++;
        }
    alien_dir   = 1;
    alien_tick  = 0;
    alien_speed = 20;
}

void init_game() {
    px = WIDTH / 2;
    lives = 3; score = 0; level = 1;
    game_over = 0; paused = 0; wave_clear = 0;
    shoot_cooldown = 0;
    bomb_tick = 0; bomb_speed = 8;
    ufo_alive = 0; ufo_tick = 0;
    memset(bactive, 0, sizeof(bactive));
    memset(dactive, 0, sizeof(dactive));
    init_shields();
    init_aliens();
}

// ── Draw one alien ────────────────────────────────────────────────────────────
// type 0,1 = top 2 rows  \/\/  (worth 30)
// type 2,3 = mid 2 rows  -oo-  (worth 20)
// type 4   = bottom row  (^^)  (worth 10)
void draw_alien(int r, int c) {
    Alien *a = &grid[r][c];
    if (!a->alive) return;
    gotoxy(a->x, a->y);
    switch (a->type) {
        case 0: case 1: set_color(13); printf("/Y\\"); break; // magenta
        case 2: case 3: set_color(11); printf("oOo"); break; // cyan
        case 4:         set_color(10); printf("(V)"); break; // green
    }
}

void erase_alien(int r, int c) {
    erase(grid[r][c].x, grid[r][c].y, 3);
}

void draw_shields() {
    for (int s = 0; s < 4; s++) {
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 4; c++) {
                int hp = shield_hp[s][r][c];
                gotoxy(shield_ox[s] + c, HEIGHT - 5 + r);
                if      (hp == 2) { set_color(10); putchar('#'); }
                else if (hp == 1) { set_color(14); putchar('+'); }
                else              { set_color(0);  putchar(' '); }
            }
        }
    }
}

void draw_player() {
    gotoxy(px - 2, HEIGHT - 2);
    set_color(10);
    printf(" /A\\");
    // erase sides to prevent ghost when moving
    gotoxy(px - 3, HEIGHT - 2); set_color(0); putchar(' ');
    gotoxy(px + 2, HEIGHT - 2); set_color(0); putchar(' ');
}

void draw_hud() {
    gotoxy(0, 0);
    set_color(14); printf("Score:%-5d", score);
    gotoxy(WIDTH/2 - 7, 0);
    set_color(11); printf("SPACE INVADERS");
    gotoxy(WIDTH - 20, 0);
    set_color(12); printf("Lives:");
    for (int i = 0; i < lives; i++) { set_color(10); printf("A"); }
    for (int i = lives; i < 3; i++) { set_color(8);  printf("-"); }
    gotoxy(WIDTH - 7, 0);
    set_color(7); printf("Lv:%d", level);
    // ground line
    gotoxy(0, HEIGHT - 1);
    set_color(7);
    for (int x = 0; x < WIDTH; x++) putchar('_');
    // high score
    gotoxy(0, HEIGHT + 1);
    set_color(8); printf("Best: %d   A/D=Move  Space=Fire  P=Pause  Q=Quit", high_score);
}

void draw_ufo() {
    if (!ufo_alive) return;
    gotoxy(ufo_x, 1);
    set_color(12); printf("<***>");
}

void draw_bullets() {
    for (int i = 0; i < MAX_BULLETS; i++)
        if (bactive[i]) {
            gotoxy(bx[i], by[i]);
            set_color(14); putchar('|');
        }
}

void draw_bombs() {
    for (int i = 0; i < MAX_BOMBS; i++)
        if (dactive[i]) {
            gotoxy(dx[i], dy[i]);
            set_color(12); putchar('!');
        }
}

// ── Shoot ─────────────────────────────────────────────────────────────────────
void player_shoot() {
    if (shoot_cooldown > 0) return;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bactive[i]) {
            bx[i] = px; by[i] = HEIGHT - 3;
            bactive[i] = 1;
            shoot_cooldown = 10;
            Beep(700, 25);
            return;
        }
    }
}

// ── Check shield hit ──────────────────────────────────────────────────────────
// returns 1 if hit, erases shield cell
int check_shield(int x, int y) {
    for (int s = 0; s < 4; s++)
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 4; c++)
                if (shield_hp[s][r][c] > 0 &&
                    shield_ox[s]+c == x &&
                    HEIGHT-5+r    == y) {
                    shield_hp[s][r][c]--;
                    return 1;
                }
    return 0;
}

// ── Update ────────────────────────────────────────────────────────────────────
void update() {
    if (paused) return;
    if (shoot_cooldown > 0) shoot_cooldown--;

    // ── Move player bullets ──
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bactive[i]) continue;
        erase(bx[i], by[i], 1);
        by[i]--;
        if (by[i] < 2) { bactive[i] = 0; continue; }

        // hit UFO
        if (ufo_alive && by[i] == 1 && bx[i] >= ufo_x && bx[i] <= ufo_x + 4) {
            erase(ufo_x, 1, 5);
            ufo_alive = 0; bactive[i] = 0;
            score += 200;  // bonus!
            Beep(900,60); Beep(1100,60); Beep(1300,80);
            score += 200;
            continue;
        }

        // hit alien
        int hit = 0;
        for (int r = 0; r < ROWS && !hit; r++)
            for (int c = 0; c < COLS && !hit; c++) {
                Alien *a = &grid[r][c];
                if (!a->alive) continue;
                if (by[i] == a->y && bx[i] >= a->x && bx[i] <= a->x+2) {
                    erase_alien(r, c);
                    a->alive = 0;
                    bactive[i] = 0;
                    aliens_alive--;
                    int pts = (a->type<=1)?30:(a->type<=3)?20:10;
                    score += pts * level;
                    Beep(500, 35);
                    hit = 1;
                    if (aliens_alive == 0) wave_clear = 1;
                }
            }

        if (!hit && check_shield(bx[i], by[i]))
            bactive[i] = 0;
    }

    // ── Move aliens ──
    alien_tick++;
    if (alien_tick >= alien_speed) {
        alien_tick = 0;

        // Find bounds
        int min_x = WIDTH, max_x = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                if (!grid[r][c].alive) continue;
                if (grid[r][c].x     < min_x) min_x = grid[r][c].x;
                if (grid[r][c].x + 2 > max_x) max_x = grid[r][c].x + 2;
            }

        // Hit wall → drop 1 row and reverse
        int drop = 0;
        if (alien_dir == 1 && max_x >= WIDTH - 2) { alien_dir = -1; drop = 1; }
        if (alien_dir ==-1 && min_x <= 1)         { alien_dir =  1; drop = 1; }

        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                if (!grid[r][c].alive) continue;
                erase_alien(r, c);
                if (drop) grid[r][c].y++;
                else      grid[r][c].x += alien_dir;
                // reached ground → game over
                if (grid[r][c].y >= HEIGHT - 2) { game_over = 1; }
            }

        // Speed up as fewer aliens remain
        alien_speed = (aliens_alive > 30) ? 18 :
                      (aliens_alive > 20) ? 14 :
                      (aliens_alive > 10) ?  9 :
                      (aliens_alive >  5) ?  5 : 2;
        alien_speed -= (level - 1);
        if (alien_speed < 1) alien_speed = 1;

        // Bottom alien in random column drops bomb
        if (rand() % 3 == 0) {
            int col = rand() % COLS;
            int lowest = -1;
            for (int r = ROWS-1; r >= 0; r--)
                if (grid[r][col].alive) { lowest = r; break; }
            if (lowest >= 0) {
                for (int b = 0; b < MAX_BOMBS; b++) {
                    if (!dactive[b]) {
                        dx[b] = grid[lowest][col].x + 1;
                        dy[b] = grid[lowest][col].y + 1;
                        dactive[b] = 1;
                        break;
                    }
                }
            }
        }
    }

    // ── Move bombs ──
    bomb_tick++;
    if (bomb_tick >= bomb_speed) {
        bomb_tick = 0;
        for (int i = 0; i < MAX_BOMBS; i++) {
            if (!dactive[i]) continue;
            erase(dx[i], dy[i], 1);
            dy[i]++;
            if (dy[i] >= HEIGHT - 1) { dactive[i] = 0; continue; }

            // hit shield
            if (check_shield(dx[i], dy[i])) { dactive[i] = 0; continue; }

            // hit player
            if (dy[i] == HEIGHT - 2 && abs(dx[i] - px) <= 2) {
                dactive[i] = 0;
                lives--;
                Beep(200, 350);
                // flash red
                gotoxy(px-2, HEIGHT-2);
                set_color(12); printf(" *** ");
                Sleep(350);
                erase(px-3, HEIGHT-2, 7);
                if (lives <= 0) { game_over = 1; Beep(150,600); }
            }
        }
    }

    // ── UFO ──
    ufo_tick++;
    if (!ufo_alive && ufo_tick >= 150 + rand()%80) {
        ufo_tick  = 0;
        ufo_alive = 1;
        ufo_dir   = (rand()%2==0) ? 1 : -1;
        ufo_x     = (ufo_dir == 1) ? 1 : WIDTH - 6;
        Beep(300,20);
    }
    if (ufo_alive) {
        erase(ufo_x, 1, 5);
        ufo_x += ufo_dir;
        if (ufo_x < 0 || ufo_x > WIDTH-5) ufo_alive = 0;
    }
}

// ══════════════════════════════════════════════════════════════════════════════
int main() {
    srand((unsigned)time(NULL));
    hide_cursor();
    high_score = 0;

    // ── Welcome screen ──
    cls();
    gotoxy(16, 3);  set_color(11); printf("**** SPACE INVADERS ****");
    gotoxy(12, 6);  set_color(13); printf("/Y\\  = 30 pts");
    gotoxy(12, 7);  set_color(11); printf("oOo  = 20 pts");
    gotoxy(12, 8);  set_color(10); printf("(V)  = 10 pts");
    gotoxy(12, 9);  set_color(12); printf("<***>= 200 pts  (UFO!)");
    gotoxy(12,12);  set_color(7);  printf("A / D    = Move left / right");
    gotoxy(12,13);               printf("SPACE    = Shoot");
    gotoxy(12,14);               printf("P        = Pause");
    gotoxy(12,15);               printf("Q        = Quit");
    gotoxy(12,18);  set_color(8);  printf("Press any key to start...");
    _getch();
    cls();

    init_game();

    while (1) {
        // ── Input ──
        if (_kbhit()) {
            int k = _getch();
            if (k=='q'||k=='Q') break;
            if (k=='p'||k=='P') paused = !paused;
            if (!paused && !game_over) {
                if ((k=='a'||k=='A') && px > 3)          { erase(px-3,HEIGHT-2,7); px--; }
                if ((k=='d'||k=='D') && px < WIDTH-3)    { erase(px-3,HEIGHT-2,7); px++; }
                if (k==' ')                               player_shoot();
            }
            if (game_over && (k=='r'||k=='R')) {
                cls(); level=1; init_game();
            }
        }

        if (!game_over) {
            update();
            draw_hud();
            draw_ufo();
            draw_shields();
            draw_player();
            draw_bullets();
            draw_bombs();
            // Draw all living aliens
            for (int r=0;r<ROWS;r++)
                for (int c=0;c<COLS;c++)
                    draw_alien(r,c);
        }

        // ── Wave clear ──
        if (wave_clear) {
            wave_clear = 0;
            level++;
            gotoxy(WIDTH/2-10, HEIGHT/2);
            set_color(14); printf("** WAVE CLEAR! LEVEL %d **", level);
            Sleep(1800);
            cls();
            memset(bactive,0,sizeof(bactive));
            memset(dactive,0,sizeof(dactive));
            init_shields();
            init_aliens();
        }

        // ── Game over ──
        if (game_over) {
            if (score > high_score) high_score = score;
            gotoxy(WIDTH/2-9, HEIGHT/2-1);
            set_color(12); printf("**** GAME  OVER ****");
            gotoxy(WIDTH/2-9, HEIGHT/2+1);
            set_color(14); printf("Score: %d   Level: %d", score, level);
            gotoxy(WIDTH/2-9, HEIGHT/2+3);
            set_color(7);  printf("R = Restart    Q = Quit");
        }

        if (score > high_score) high_score = score;
        Sleep(30);
    }

    cls();
    set_color(7);
    printf("Thanks for playing Space Invaders!\n");
    printf("Score: %d   Level: %d   Best: %d\n", score, level, high_score);
    return 0;
}
