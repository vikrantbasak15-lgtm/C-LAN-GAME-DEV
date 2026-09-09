#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define WIDTH    40
#define HEIGHT   20
#define MAX_LEN  400

#define COL_WHITE   7
#define COL_GREEN   10
#define COL_RED     12
#define COL_YELLOW  14
#define COL_CYAN    11
#define COL_GRAY    8

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

// Snake body
int snake_x[MAX_LEN], snake_y[MAX_LEN];
int snake_len;
int dir_x, dir_y;
int food_x, food_y;
int bonus_x, bonus_y, bonus_active, bonus_timer;
int score, high_score;
int level, speed;
int game_over, paused;

void spawn_food() {
    int valid;
    do {
        valid = 1;
        food_x = 1 + rand() % (WIDTH  - 2);
        food_y = 1 + rand() % (HEIGHT - 2);
        for (int i = 0; i < snake_len; i++)
            if (snake_x[i] == food_x && snake_y[i] == food_y) { valid = 0; break; }
    } while (!valid);
}

void spawn_bonus() {
    bonus_x = 1 + rand() % (WIDTH  - 2);
    bonus_y = 1 + rand() % (HEIGHT - 2);
    bonus_active = 1;
    bonus_timer  = 30; // disappears after 30 ticks
}

void init_game() {
    snake_len = 4;
    int sx = WIDTH / 2, sy = HEIGHT / 2;
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = sx - i;
        snake_y[i] = sy;
    }
    dir_x = 1; dir_y = 0;
    score = 0; level = 1; speed = 120;
    bonus_active = 0;
    game_over = 0; paused = 0;
    spawn_food();
}

void draw() {
    // Border
    for (int x = 0; x < WIDTH; x++) {
        gotoxy(x, 0);         set_color(COL_WHITE); putchar('=');
        gotoxy(x, HEIGHT);    set_color(COL_WHITE); putchar('=');
    }
    for (int y = 1; y < HEIGHT; y++) {
        gotoxy(0, y);         set_color(COL_WHITE); putchar('|');
        gotoxy(WIDTH-1, y);   set_color(COL_WHITE); putchar('|');
    }

    // Snake
    for (int i = 0; i < snake_len; i++) {
        gotoxy(snake_x[i], snake_y[i]);
        if (i == 0) { set_color(COL_GREEN); putchar('@'); }
        else        { set_color(10);        putchar('o'); }
    }

    // Food
    gotoxy(food_x, food_y);
    set_color(COL_RED); putchar('*');

    // Bonus food
    if (bonus_active) {
        gotoxy(bonus_x, bonus_y);
        set_color(COL_YELLOW); putchar('$');
    }

    // HUD
    gotoxy(0, HEIGHT + 1);
    set_color(COL_CYAN);
    printf("Score: %d   Level: %d   Hi: %d      ", score, level, high_score);
    gotoxy(0, HEIGHT + 2);
    set_color(COL_GRAY);
    printf("Arrows=Move  P=Pause  R=Restart  Q=Quit");
}

void clear_snake_tail(int tx, int ty) {
    gotoxy(tx, ty);
    set_color(COL_WHITE); putchar(' ');
}

void update() {
    if (paused || game_over) return;

    int new_x = snake_x[0] + dir_x;
    int new_y = snake_y[0] + dir_y;

    // Wall collision
    if (new_x <= 0 || new_x >= WIDTH-1 || new_y <= 0 || new_y >= HEIGHT) {
        game_over = 1; Beep(200, 400); return;
    }

    // Self collision
    for (int i = 1; i < snake_len; i++) {
        if (snake_x[i] == new_x && snake_y[i] == new_y) {
            game_over = 1; Beep(200, 400); return;
        }
    }

    int ate = 0;
    // Food check
    if (new_x == food_x && new_y == food_y) {
        ate = 1;
        score += 10 * level;
        Beep(600, 50);
        spawn_food();
        if (score % 50 == 0 && speed > 40) { level++; speed -= 10; }
        if (rand() % 3 == 0) spawn_bonus();
    }

    // Bonus check
    if (bonus_active && new_x == bonus_x && new_y == bonus_y) {
        ate = 1;
        score += 30 * level;
        bonus_active = 0;
        Beep(900, 80);
    }

    // Move tail
    int tail_x = snake_x[snake_len-1];
    int tail_y = snake_y[snake_len-1];

    for (int i = snake_len-1; i > 0; i--) {
        snake_x[i] = snake_x[i-1];
        snake_y[i] = snake_y[i-1];
    }
    snake_x[0] = new_x;
    snake_y[0] = new_y;

    if (ate) {
        if (snake_len < MAX_LEN) snake_len++;
    } else {
        clear_snake_tail(tail_x, tail_y);
    }

    // Bonus timer
    if (bonus_active) {
        bonus_timer--;
        if (bonus_timer <= 0) bonus_active = 0;
    }

    if (score > high_score) high_score = score;
}

void show_menu() {
    system("cls");
    set_color(COL_GREEN);
    gotoxy(WIDTH/2 - 8, 5);  printf("**** SNAKE ****");
    gotoxy(WIDTH/2 - 12, 8); set_color(COL_WHITE);
    printf("Arrow Keys = Move");
    gotoxy(WIDTH/2 - 12, 9);  printf("Eat * for points");
    gotoxy(WIDTH/2 - 12, 10); set_color(COL_YELLOW); printf("Eat $ for BONUS points");
    gotoxy(WIDTH/2 - 12, 12); set_color(COL_GRAY);   printf("Press any key to start...");
    _getch();
    system("cls");
}

int main() {
    srand(time(NULL));
    hide_cursor();
    high_score = 0;

    show_menu();
    init_game();

    while (1) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 'q' || key == 'Q') break;
            if (key == 'p' || key == 'P') paused = !paused;
            if (key == 'r' || key == 'R') { system("cls"); init_game(); }
            if (key == 224 || key == 0) {
                int arrow = _getch();
                if (arrow == 72 && dir_y != 1)  { dir_x = 0;  dir_y = -1; } // Up
                if (arrow == 80 && dir_y != -1) { dir_x = 0;  dir_y =  1; } // Down
                if (arrow == 75 && dir_x != 1)  { dir_x = -1; dir_y =  0; } // Left
                if (arrow == 77 && dir_x != -1) { dir_x =  1; dir_y =  0; } // Right
            }
        }

        update();
        draw();

        if (game_over) {
            gotoxy(WIDTH/2 - 8, HEIGHT/2);
            set_color(COL_RED);
            printf("** GAME OVER! **");
            gotoxy(WIDTH/2 - 10, HEIGHT/2 + 1);
            set_color(COL_YELLOW);
            printf("Score: %d  R=Restart Q=Quit", score);
            // Wait for R or Q
            while (1) {
                if (_kbhit()) {
                    int k = _getch();
                    if (k == 'q' || k == 'Q') goto done;
                    if (k == 'r' || k == 'R') { system("cls"); init_game(); break; }
                }
            }
        }

        Sleep(speed);
    }
    done:
    system("cls");
    set_color(COL_WHITE);
    printf("Thanks for playing Snake!\nFinal Score: %d  High Score: %d\n", score, high_score);
    return 0;
}
