#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define WIDTH      70
#define HEIGHT     22
#define PADDLE_H    4
#define WIN_SCORE  10
#define MAX_SPEED   3.5f

// Colors
#define COL_WHITE   7
#define COL_GREEN   10
#define COL_CYAN    11
#define COL_YELLOW  14
#define COL_RED     12
#define COL_GRAY    8
#define COL_MAGENTA 13
#define COL_ORANGE  6

void gotoxy(int x, int y) {
    COORD c = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void hide_cursor() {
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

void set_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void clear_screen() {
    system("cls");
}

void beep_hit()  { Beep(600, 40);  }
void beep_wall() { Beep(300, 30);  }
void beep_score(){ Beep(200, 200); }
void beep_win()  { Beep(800, 100); Sleep(80); Beep(1000, 100); Sleep(80); Beep(1200, 200); }

// Game state
float ball_x,  ball_y;
float ball_dx, ball_dy;
float speed_multiplier;
int   player_y, cpu_y;
int   player_score, cpu_score;
int   player_lives, cpu_lives;
int   hits;            // rally counter
int   paused;
int   difficulty;      // 1=Easy 2=Medium 3=Hard
char  last_msg[64];    // status message

void init_game() {
    ball_x = WIDTH / 2;
    ball_y = HEIGHT / 2;

    float base = (difficulty == 1) ? 0.8f :
                 (difficulty == 2) ? 1.1f : 1.4f;

    ball_dx = (rand() % 2 == 0) ? base : -base;
    ball_dy = ((rand() % 100) / 100.0f) * 1.2f - 0.6f;
    speed_multiplier = 1.0f;
    hits = 0;

    player_y = HEIGHT / 2 - PADDLE_H / 2;
    cpu_y    = HEIGHT / 2 - PADDLE_H / 2;
}

void draw_bar(int y_pos) {
    gotoxy(0, y_pos);
    set_color(COL_WHITE);
    for (int x = 0; x < WIDTH; x++) printf("=");
}

// Draw a number in big pixel font (5 cols wide)
void draw_big_digit(int digit, int col, int row) {
    const char* digits[10][5] = {
        {" *** "," * * "," * * "," * * "," *** "},  // 0
        {"  *  ","  *  ","  *  ","  *  ","  *  "},  // 1
        {" *** ","   * "," *** "," *   "," *** "},  // 2
        {" *** ","   * "," *** ","   * "," *** "},  // 3
        {" * * "," * * "," *** ","   * ","   * "},  // 4
        {" *** "," *   "," *** ","   * "," *** "},  // 5
        {" *** "," *   "," *** "," * * "," *** "},  // 6
        {" *** ","   * ","   * ","   * ","   * "},  // 7
        {" *** "," * * "," *** "," * * "," *** "},  // 8
        {" *** "," * * "," *** ","   * "," *** "},  // 9
    };
    for (int r = 0; r < 5; r++) {
        gotoxy(col, row + r);
        set_color(COL_CYAN);
        printf("%s", digits[digit][r]);
    }
}

void draw_scores() {
    // Big score digits
    draw_big_digit(player_score % 10, WIDTH/2 - 14, 1);
    draw_big_digit(cpu_score    % 10, WIDTH/2 +  8, 1);

    // Lives
    gotoxy(2, 0);
    set_color(COL_RED);
    printf("LIVES: ");
    for (int i = 0; i < player_lives; i++) printf("*");
    for (int i = player_lives; i < 3; i++) printf("-");

    gotoxy(WIDTH - 16, 0);
    set_color(COL_RED);
    printf("CPU: ");
    for (int i = 0; i < cpu_lives; i++) printf("*");
    for (int i = cpu_lives; i < 3; i++) printf("-");

    // Rally counter
    if (hits > 3) {
        gotoxy(WIDTH/2 - 5, 0);
        set_color(hits > 8 ? COL_RED : COL_YELLOW);
        printf("RALLY:%2d", hits);
    }

    // Speed indicator
    gotoxy(WIDTH/2 - 5, HEIGHT + 8);
    set_color(COL_GRAY);
    printf("Speed: ");
    float s = (ball_dx < 0 ? -ball_dx : ball_dx) * speed_multiplier;
    int bars = (int)(s / MAX_SPEED * 8);
    set_color(bars > 5 ? COL_RED : bars > 3 ? COL_YELLOW : COL_GREEN);
    for (int i = 0; i < bars && i < 8; i++) printf("|");
    for (int i = bars; i < 8; i++) printf(".");
}

void draw() {
    char frame[HEIGHT][WIDTH];
    memset(frame, ' ', sizeof(frame));

    // Borders
    for (int x = 0; x < WIDTH; x++) {
        frame[0][x]        = '=';
        frame[HEIGHT-1][x] = '=';
    }
    for (int y = 0; y < HEIGHT; y++) {
        frame[y][0]       = '|';
        frame[y][WIDTH-1] = '|';
    }

    // Center dashed line
    for (int y = 1; y < HEIGHT-1; y++)
        if (y % 2 == 0) frame[y][WIDTH/2] = ':';

    // Player paddle
    for (int i = 0; i < PADDLE_H; i++) {
        int row = player_y + i;
        if (row > 0 && row < HEIGHT-1) frame[row][2] = '#';
    }

    // CPU paddle — grows with difficulty
    int cpu_draw_h = PADDLE_H - difficulty + 1;
    if (cpu_draw_h < 2) cpu_draw_h = 2;
    for (int i = 0; i < cpu_draw_h; i++) {
        int row = cpu_y + i;
        if (row > 0 && row < HEIGHT-1) frame[row][WIDTH-3] = '#';
    }

    // Ball — change char based on speed
    float spd = (ball_dx < 0 ? -ball_dx : ball_dx) * speed_multiplier;
    char ball_char = spd < 1.5f ? 'O' : spd < 2.5f ? 'o' : '*';
    int bx = (int)ball_x, by = (int)ball_y;
    if (bx > 0 && bx < WIDTH-1 && by > 0 && by < HEIGHT-1)
        frame[by][bx] = ball_char;

    // Render frame rows (skip score area rows 1-6)
    for (int y = 0; y < HEIGHT; y++) {
        gotoxy(0, y + 7);
        for (int x = 0; x < WIDTH; x++) {
            char c = frame[y][x];
            if      (c == '=' || c == '|') set_color(COL_WHITE);
            else if (c == '#')             set_color(COL_GREEN);
            else if (c == 'O')             set_color(COL_CYAN);
            else if (c == 'o')             set_color(COL_YELLOW);
            else if (c == '*')             set_color(COL_RED);
            else if (c == ':')             set_color(COL_GRAY);
            else                           set_color(COL_WHITE);
            putchar(c);
        }
    }

    draw_scores();

    // Status message
    gotoxy(WIDTH/2 - (int)strlen(last_msg)/2, HEIGHT + 8);
    set_color(COL_YELLOW);
    printf("%-40s", last_msg);

    // Controls
    gotoxy(0, HEIGHT + 9);
    set_color(COL_GRAY);
    printf("  W/S=Move  P=Pause  R=Restart  Q=Quit");

    if (paused) {
        gotoxy(WIDTH/2 - 5, HEIGHT/2 + 7);
        set_color(COL_YELLOW);
        printf("** PAUSED **");
    }
}

void update() {
    if (paused) return;

    float sx = ball_dx * speed_multiplier;
    float sy = ball_dy * speed_multiplier;

    ball_x += sx;
    ball_y += sy;

    // Wall bounce
    if (ball_y <= 1) {
        ball_y  = 1;
        ball_dy = -ball_dy;
        beep_wall();
    }
    if (ball_y >= HEIGHT-2) {
        ball_y  = HEIGHT-2;
        ball_dy = -ball_dy;
        beep_wall();
    }

    // Player paddle collision (col 3)
    if (ball_x <= 3) {
        if ((int)ball_y >= player_y && (int)ball_y <= player_y + PADDLE_H) {
            ball_x  = 3;
            ball_dx = -ball_dx;
            ball_dy = ((ball_y - player_y) / (float)PADDLE_H - 0.5f) * 2.2f;
            hits++;
            // Speed up every 3 hits
            if (hits % 3 == 0 && speed_multiplier < MAX_SPEED)
                speed_multiplier += 0.2f;
            sprintf(last_msg, hits > 8 ? "INCREDIBLE RALLY! %d hits!" :
                               hits > 4 ? "Nice rally! %d hits" : "", hits);
            beep_hit();
        } else {
            // CPU scores
            cpu_score++;
            player_lives--;
            beep_score();
            strcpy(last_msg, "CPU scored! :(");
            init_game();
            return;
        }
    }

    // CPU paddle collision (col WIDTH-3)
    if (ball_x >= WIDTH-3) {
        if ((int)ball_y >= cpu_y && (int)ball_y <= cpu_y + PADDLE_H) {
            ball_x  = WIDTH-3;
            ball_dx = -ball_dx;
            ball_dy = ((ball_y - cpu_y) / (float)PADDLE_H - 0.5f) * 2.2f;
            hits++;
            if (hits % 3 == 0 && speed_multiplier < MAX_SPEED)
                speed_multiplier += 0.2f;
            beep_hit();
        } else {
            // Player scores
            player_score++;
            cpu_lives--;
            beep_score();
            strcpy(last_msg, "You scored! :)");
            init_game();
            return;
        }
    }

    // CPU AI — accuracy based on difficulty
    int cpu_center = cpu_y + PADDLE_H / 2;
    int react = (difficulty == 1) ? 3 : (difficulty == 2) ? 2 : 1;
    // Add imperfection on easy/medium
    int noise = (difficulty < 3) ? (rand() % (4 - difficulty)) - 1 : 0;
    if ((int)ball_x > WIDTH / 2) { // only move when ball coming toward CPU
        if (cpu_center + noise < (int)ball_y - react && cpu_y + PADDLE_H < HEIGHT-2) cpu_y++;
        else if (cpu_center + noise > (int)ball_y + react && cpu_y > 1)              cpu_y--;
    }
}

void show_menu() {
    clear_screen();
    set_color(COL_CYAN);
    gotoxy(WIDTH/2 - 10, 3); printf("***** PONG DELUXE *****");
    gotoxy(WIDTH/2 - 10, 5); set_color(COL_WHITE);  printf("Select Difficulty:");
    gotoxy(WIDTH/2 - 10, 7); set_color(COL_GREEN);  printf("  1. Easy");
    gotoxy(WIDTH/2 - 10, 8); set_color(COL_YELLOW); printf("  2. Medium");
    gotoxy(WIDTH/2 - 10, 9); set_color(COL_RED);    printf("  3. Hard");
    gotoxy(WIDTH/2 - 10,11); set_color(COL_GRAY);   printf("Press 1, 2, or 3...");
    set_color(COL_WHITE);

    while (1) {
        if (_kbhit()) {
            int k = _getch();
            if (k == '1') { difficulty = 1; break; }
            if (k == '2') { difficulty = 2; break; }
            if (k == '3') { difficulty = 3; break; }
        }
    }
}

int main() {
    srand(time(NULL));
    hide_cursor();

    show_menu();
    clear_screen();

    player_score = 0; cpu_score = 0;
    player_lives = 3; cpu_lives = 3;
    paused = 0;
    strcpy(last_msg, "");
    init_game();

    int delay = (difficulty == 1) ? 55 : (difficulty == 2) ? 42 : 30;

    while (1) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 'q' || key == 'Q') break;
            if (key == 'p' || key == 'P') { paused = !paused; }
            if ((key == 'r' || key == 'R')) {
                player_score = 0; cpu_score = 0;
                player_lives = 3; cpu_lives = 3;
                strcpy(last_msg, "Restarted!");
                init_game();
            }
            if (!paused) {
                if ((key == 'w' || key == 'W') && player_y > 1)                   player_y--;
                if ((key == 's' || key == 'S') && player_y + PADDLE_H < HEIGHT-2) player_y++;
            }
        }

        update();
        draw();

        // Win by score OR lives
        int you_win = (player_score >= WIN_SCORE || cpu_lives    <= 0);
        int cpu_win = (cpu_score    >= WIN_SCORE || player_lives <= 0);

        if (you_win || cpu_win) {
            beep_win();
            gotoxy(WIDTH/2 - 14, HEIGHT + 11);
            set_color(COL_YELLOW);
            if (you_win)
                printf("*** YOU WIN! Final: %d-%d  Rallies rocked! ***", player_score, cpu_score);
            else
                printf("*** CPU WINS! Final: %d-%d  Better luck!   ***", player_score, cpu_score);
            set_color(COL_WHITE);
            Sleep(4000);
            break;
        }

        Sleep(delay);
    }

    clear_screen();
    set_color(COL_WHITE);
    printf("Thanks for playing Pong Deluxe!\n");
    printf("Final Score -> You: %d | CPU: %d\n", player_score, cpu_score);
    return 0;
}
