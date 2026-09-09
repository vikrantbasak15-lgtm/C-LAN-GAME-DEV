# C Console Arcade Games

A small collection of classic arcade games written in C for the Windows console. Each game is a standalone program with colorful terminal graphics, keyboard controls, sound effects, and score tracking for the current session.

## Games

| Game | Source file | Highlights |
| --- | --- | --- |
| **Pong Deluxe** | `pong_game.c` | Three difficulty levels, CPU opponent, rally-based speed increases, lives, and first-to-10 scoring. |
| **Snake** | `snake_game.c` | Arrow-key movement, level progression, bonus food, and a session high score. |
| **Tetris** | `tetris_game.c` | Seven tetrominoes, piece preview, ghost piece, line clearing, levels, and hard drop. |
| **Space Invaders** | `space_invaders_games.c` | Alien waves, destructible shields, UFO bonuses, enemy bombs, lives, and levels. |

## Requirements

- Windows (the code uses `windows.h` and `conio.h`)
- A C compiler that supports the Windows API, such as MinGW-w64 GCC or Microsoft Visual C++
- A terminal with a reasonably large console window for the game display

## Build and run

### With MinGW-w64 GCC

Open PowerShell in this folder and compile the game you want to play:

```powershell
gcc -std=c11 -Wall -Wextra -O2 pong_game.c -o pong_game.exe
gcc -std=c11 -Wall -Wextra -O2 snake_game.c -o snake_game.exe
gcc -std=c11 -Wall -Wextra -O2 tetris_game.c -o tetris_game.exe
gcc -std=c11 -Wall -Wextra -O2 space_invaders_games.c -o space_invaders_games.exe
```

Then run it:

```powershell
.\pong_game.exe
```

Prebuilt `.exe` files are included for convenience; rebuilding from source is recommended after making changes.

### With Microsoft Visual C++

From a **Developer PowerShell for Visual Studio**:

```powershell
cl /W4 /TC pong_game.c
```

Replace `pong_game.c` with the source file for any of the other games.

## Controls

| Game | Controls |
| --- | --- |
| **Pong Deluxe** | `W` / `S` move, `P` pause, `R` restart, `Q` quit; select `1`–`3` for difficulty at launch. |
| **Snake** | Arrow keys move, `P` pause, `R` restart, `Q` quit. Eat `*` for points and `$` for bonus points. |
| **Tetris** | `A` / `D` move, `W` rotate, `S` soft drop, `Space` hard drop, `P` pause, `Q` quit. |
| **Space Invaders** | `A` / `D` move, `Space` fire, `P` pause, `R` restart after game over, `Q` quit. |

## Project structure

```text
.
├── pong_game.c
├── snake_game.c
├── tetris_game.c
├── space_invaders_games.c
└── README.md
```

## Notes

- Scores and high scores are kept only while a game is running; they are not saved to disk.
- The games use Windows console cursor positioning, colors, keyboard polling, `Sleep`, and `Beep`, so they are not currently portable to macOS or Linux without adaptation.

## License

No license has been specified yet. Add a `LICENSE` file before publishing if you want to define how others may use, modify, and distribute this project.
