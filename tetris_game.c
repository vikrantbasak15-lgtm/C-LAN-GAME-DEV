#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define BOARD_W   10
#define BOARD_H   20
#define OFFSET_X   4
#define OFFSET_Y   2

void gotoxy(int x, int y) {
    COORD c = {x*2, y};  // x*2 for double-wide cells
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}
void gotoxy_raw(int x, int y) {
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

// 7 tetrominoes, 4 rotations each (4x4 grid)
const int PIECES[7][4][4][4] = {
    // I
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
     {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
    // O
    {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
    // T
    {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // S
    {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
     {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
     {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // Z
    {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}},
     {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}},
    // J
    {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
    // L
    {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
     {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
};

// Colors per piece
const int COLORS[7] = {11, 14, 13, 10, 12, 9, 6};

int board[BOARD_H][BOARD_W];   // 0=empty, else color
int board_color[BOARD_H][BOARD_W];

int cur_piece, cur_rot, cur_x, cur_y;
int next_piece;
int score, lines, level, high_score;
int game_over, paused;
int drop_timer, drop_speed;

void new_piece() {
    cur_piece = next_piece;
    cur_rot   = 0;
    cur_x     = BOARD_W / 2 - 2;
    cur_y     = 0;
    next_piece = rand() % 7;
}

int valid(int piece, int rot, int px, int py) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (PIECES[piece][rot][r][c]) {
                int bx = px + c, by = py + r;
                if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return 0;
                if (by >= 0 && board[by][bx]) return 0;
            }
    return 1;
}

void lock_piece() {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (PIECES[cur_piece][cur_rot][r][c]) {
                int bx = cur_x + c, by = cur_y + r;
                if (by >= 0) {
                    board[by][bx]       = 1;
                    board_color[by][bx] = COLORS[cur_piece];
                }
            }
}

int clear_lines() {
    int cleared = 0;
    for (int r = BOARD_H-1; r >= 0; r--) {
        int full = 1;
        for (int c = 0; c < BOARD_W; c++)
            if (!board[r][c]) { full = 0; break; }
        if (full) {
            cleared++;
            for (int rr = r; rr > 0; rr--) {
                memcpy(board[rr],       board[rr-1],       sizeof(board[0]));
                memcpy(board_color[rr], board_color[rr-1], sizeof(board_color[0]));
            }
            memset(board[0], 0, sizeof(board[0]));
            r++;
        }
    }
    return cleared;
}

void draw_cell(int x, int y, int color, int filled) {
    gotoxy(OFFSET_X + x, OFFSET_Y + y);
    set_color(color);
    printf(filled ? "[]" : "  ");
}

void draw_board() {
    // Border
    for (int y = 0; y < BOARD_H; y++) {
        gotoxy_raw((OFFSET_X)*2 - 2, OFFSET_Y + y);
        set_color(7); printf("|");
        gotoxy_raw((OFFSET_X + BOARD_W)*2, OFFSET_Y + y);
        printf("|");
    }
    gotoxy_raw((OFFSET_X)*2 - 2, OFFSET_Y + BOARD_H);
    set_color(7);
    for (int x = 0; x < BOARD_W*2 + 2; x++) printf("=");

    // Board cells
    for (int r = 0; r < BOARD_H; r++)
        for (int c = 0; c < BOARD_W; c++) {
            if (board[r][c]) draw_cell(c, r, board_color[r][c], 1);
            else             draw_cell(c, r, 8, 0);
        }

    // Ghost piece (where it will land)
    int gy = cur_y;
    while (valid(cur_piece, cur_rot, cur_x, gy+1)) gy++;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (PIECES[cur_piece][cur_rot][r][c] && gy + r != cur_y + r) {
                gotoxy(OFFSET_X + cur_x + c, OFFSET_Y + gy + r);
                set_color(8); printf("--");
            }

    // Current piece
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (PIECES[cur_piece][cur_rot][r][c] && cur_y + r >= 0)
                draw_cell(cur_x + c, cur_y + r, COLORS[cur_piece], 1);

    // HUD
    int hx = (OFFSET_X + BOARD_W + 2) * 2;
    gotoxy_raw(hx, OFFSET_Y);     set_color(11); printf("TETRIS");
    gotoxy_raw(hx, OFFSET_Y+2);   set_color(7);  printf("Score: %d  ", score);
    gotoxy_raw(hx, OFFSET_Y+3);   set_color(7);  printf("Lines: %d  ", lines);
    gotoxy_raw(hx, OFFSET_Y+4);   set_color(7);  printf("Level: %d  ", level);
    gotoxy_raw(hx, OFFSET_Y+5);   set_color(14); printf("Best:  %d  ", high_score);

    gotoxy_raw(hx, OFFSET_Y+8);   set_color(11); printf("NEXT:");
    // Draw next piece preview
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            gotoxy_raw(hx + c*2, OFFSET_Y + 9 + r);
            if (PIECES[next_piece][0][r][c]) {
                set_color(COLORS[next_piece]); printf("[]");
            } else {
                set_color(8); printf("  ");
            }
        }

    gotoxy_raw(hx, OFFSET_Y+15); set_color(8);
    printf("A/D=Move");
    gotoxy_raw(hx, OFFSET_Y+16); printf("W=Rotate");
    gotoxy_raw(hx, OFFSET_Y+17); printf("S=Drop");
    gotoxy_raw(hx, OFFSET_Y+18); printf("Space=Hard");
    gotoxy_raw(hx, OFFSET_Y+19); printf("P=Pause");
}

void init_game() {
    memset(board,       0, sizeof(board));
    memset(board_color, 0, sizeof(board_color));
    score = 0; lines = 0; level = 1;
    drop_speed = 30; drop_timer = 0;
    game_over = 0; paused = 0;
    next_piece = rand() % 7;
    new_piece();
}

int main() {
    srand(time(NULL));
    hide_cursor();
    high_score = 0;
    system("cls");

    // Welcome
    gotoxy_raw(10, 5);  set_color(11); printf("**** TETRIS ****");
    gotoxy_raw(10, 7);  set_color(7);  printf("A/D = Move Left/Right");
    gotoxy_raw(10, 8);               printf("W   = Rotate");
    gotoxy_raw(10, 9);               printf("S   = Soft Drop");
    gotoxy_raw(10, 10);              printf("SPC = Hard Drop");
    gotoxy_raw(10, 11);              printf("P   = Pause  Q = Quit");
    gotoxy_raw(10, 13); set_color(8); printf("Press any key...");
    _getch(); system("cls");

    init_game();

    while (!game_over) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 'q' || key == 'Q') break;
            if (key == 'p' || key == 'P') { paused = !paused; }
            if (!paused) {
                if ((key=='a'||key=='A') && valid(cur_piece,cur_rot,cur_x-1,cur_y)) cur_x--;
                if ((key=='d'||key=='D') && valid(cur_piece,cur_rot,cur_x+1,cur_y)) cur_x++;
                if ((key=='s'||key=='S') && valid(cur_piece,cur_rot,cur_x,cur_y+1)) { cur_y++; score++; }
                if (key=='w'||key=='W') {
                    int nr = (cur_rot+1)%4;
                    if (valid(cur_piece,nr,cur_x,cur_y)) cur_rot = nr;
                }
                if (key==' ') { // Hard drop
                    while (valid(cur_piece,cur_rot,cur_x,cur_y+1)) { cur_y++; score+=2; }
                    lock_piece();
                    int cl = clear_lines();
                    if (cl) {
                        int pts[] = {0,100,300,500,800};
                        score += pts[cl] * level;
                        lines += cl;
                        Beep(800,50);
                    }
                    level = lines/10 + 1;
                    drop_speed = 30 - level*2;
                    if (drop_speed < 5) drop_speed = 5;
                    new_piece();
                    if (!valid(cur_piece,cur_rot,cur_x,cur_y)) game_over = 1;
                }
            }
        }

        if (!paused) {
            drop_timer++;
            if (drop_timer >= drop_speed) {
                drop_timer = 0;
                if (valid(cur_piece, cur_rot, cur_x, cur_y+1)) {
                    cur_y++;
                } else {
                    lock_piece();
                    Beep(400, 30);
                    int cl = clear_lines();
                    if (cl) {
                        int pts[] = {0,100,300,500,800};
                        score += pts[cl] * level;
                        lines += cl;
                        Beep(800,80);
                    }
                    level = lines/10 + 1;
                    drop_speed = 30 - level*2;
                    if (drop_speed < 5) drop_speed = 5;
                    new_piece();
                    if (!valid(cur_piece,cur_rot,cur_x,cur_y)) game_over = 1;
                }
            }
        }

        if (score > high_score) high_score = score;
        draw_board();
        Sleep(33); // ~30fps
    }

    // Game over screen
    gotoxy_raw(OFFSET_X*2, OFFSET_Y + BOARD_H/2);
    set_color(12); printf("  GAME OVER!  ");
    gotoxy_raw(OFFSET_X*2, OFFSET_Y + BOARD_H/2 + 1);
    set_color(14); printf("  Score: %d   ", score);
    Sleep(3000);

    system("cls");
    set_color(7);
    printf("Thanks for playing Tetris!\nScore: %d  Lines: %d  Level: %d\n", score, lines, level);
    return 0;
}
