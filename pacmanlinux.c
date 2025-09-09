
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define PACMAN_CHAR_OPEN 'C'
#define PACMAN_CHAR_CLOSED 'O'
#define GHOST_CHAR 'G'
#define WALL_CHAR '#'
#define PELLET_CHAR '.'
#define POWER_CHAR 'o'
#define EMPTY_CHAR ' '
#define WIDTH 28
#define HEIGHT 20
#define NUM_GHOSTS 4
#define POWER_DURATION 30
#define SCORE_DISPLAY_TIME 15 // frames para mostrar pontos ganhos

HANDLE hConsole;
COORD cursorHome = {0, 0};

typedef struct {
    int x, y;
} Entity;

typedef struct {
    int x, y;
    int points;
    int timer; // tempo restante para exibir pontos
} ScorePopup;

Entity pacman;
Entity ghosts[NUM_GHOSTS];
int score = 0;
int lives = 3;
int powerMode = 0;
int pelletsLeft = 0;
char map[HEIGHT][WIDTH + 1];
int pacmanMouthOpen = 1; // para animação
ScorePopup scorePopups[20]; // até 20 popups simultâneos

// ---- Mapa simples ----
char level[HEIGHT][WIDTH + 1] = {
    "############################",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#o####.#####.##.#####.####o#",
    "#.####.#####.##.#####.####.#",
    "#..........................#",
    "#.####.##.########.##.####.#",
    "#.####.##.########.##.####.#",
    "#......##....##....##......#",
    "######.##### ## #####.######",
    "     #.##### ## #####.#     ",
    "######.##          ##.######",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#o..##................##..o#",
    "###.##.##.########.##.##.###",
    "#......##....##....##......#",
    "#.##########.##.##########.#",
    "#..........................#",
    "############################"
};

void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void clearScreen() {
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X * csbi.dwSize.Y, (COORD){0,0}, &written);
    SetConsoleCursorPosition(hConsole, cursorHome);
}

void addScorePopup(int x, int y, int points) {
    for (int i = 0; i < 20; i++) {
        if (scorePopups[i].timer == 0) {
            scorePopups[i].x = x;
            scorePopups[i].y = y;
            scorePopups[i].points = points;
            scorePopups[i].timer = SCORE_DISPLAY_TIME;
            break;
        }
    }
}

void drawScorePopups() {
    for (int i = 0; i < 20; i++) {
        if (scorePopups[i].timer > 0) {
            COORD pos = { (SHORT)scorePopups[i].x, (SHORT)scorePopups[i].y };
            SetConsoleCursorPosition(hConsole, pos);
            setColor(10); // Verde para pontos ganhos
            printf("+%d", scorePopups[i].points);
            scorePopups[i].timer--;
        }
    }
}

void resetGame() {
    pelletsLeft = 0; // Zerar antes de contar
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            map[y][x] = level[y][x];
            if (map[y][x] == PELLET_CHAR || map[y][x] == POWER_CHAR)
                pelletsLeft++;
        }
    }
    pacman.x = 14;
    pacman.y = 17;
    ghosts[0].x = 13; ghosts[0].y = 10;
    ghosts[1].x = 14; ghosts[1].y = 10;
    ghosts[2].x = 12; ghosts[2].y = 10;
    ghosts[3].x = 15; ghosts[3].y = 10;
    score = 0;
    lives = 3;
    powerMode = 0;
    for (int i = 0; i < 20; i++) scorePopups[i].timer = 0;
}

void drawMap() {
    SetConsoleCursorPosition(hConsole, cursorHome);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            char c = map[y][x];
            if (x == pacman.x && y == pacman.y) {
                setColor(14); // Amarelo
                putchar(pacmanMouthOpen ? PACMAN_CHAR_OPEN : PACMAN_CHAR_CLOSED);
            } else {
                int ghostHere = 0;
                for (int g = 0; g < NUM_GHOSTS; g++) {
                    if (ghosts[g].x == x && ghosts[g].y == y) {
                        if (powerMode > 0) {
                            setColor(9); // Azul para fantasmas assustados
                        } else {
                            setColor(g + 9); // cores diferentes
                        }
                        putchar(GHOST_CHAR);
                        ghostHere = 1;
                        break;
                    }
                }
                if (!ghostHere) {
                    if (c == WALL_CHAR) { setColor(11); } // Azul claro
                    else if (c == PELLET_CHAR) { setColor(15); } // Branco
                    else if (c == POWER_CHAR) { setColor(11); } // Ciano
                    else { setColor(7); }
                    putchar(c);
                }
            }
        }
        putchar('\n');
    }
    setColor(7);
    printf("Score: %d  Lives: %d  %s\n", score, lives, powerMode ? "POWER!" : "");
    drawScorePopups();
}

// BFS para encontrar o próximo passo do fantasma em direção ao Pac-Man
int bfsNextStep(int startX, int startY, int targetX, int targetY, int *nextX, int *nextY) {
    int visited[HEIGHT][WIDTH] = {0};
    int prevX[HEIGHT][WIDTH];
    int prevY[HEIGHT][WIDTH];
    for (int i = 0; i < HEIGHT; i++)
        for (int j = 0; j < WIDTH; j++) {
            prevX[i][j] = -1;
            prevY[i][j] = -1;
        }

    typedef struct { int x, y; } Point;
    Point queue[HEIGHT * WIDTH];
    int front = 0, back = 0;

    queue[back++] = (Point){startX, startY};
    visited[startY][startX] = 1;

    int directions[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    while (front < back) {
        Point p = queue[front++];
        if (p.x == targetX && p.y == targetY) {
            // Reconstruir caminho
            int cx = p.x, cy = p.y;
            while (prevX[cy][cx] != startX || prevY[cy][cx] != startY) {
                int px = prevX[cy][cx];
                int py = prevY[cy][cx];
                cx = px;
                cy = py;
            }
            *nextX = cx;
            *nextY = cy;
            return 1;
        }
        for (int d = 0; d < 4; d++) {
            int nx = p.x + directions[d][0];
            int ny = p.y + directions[d][1];
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
                !visited[ny][nx] && map[ny][nx] != WALL_CHAR) {
                visited[ny][nx] = 1;
                prevX[ny][nx] = p.x;
                prevY[ny][nx] = p.y;
                queue[back++] = (Point){nx, ny};
            }
        }
    }
    return 0; // Sem caminho encontrado
}

void moveEntity(Entity *e, int dx, int dy) {
    int nx = e->x + dx;
    int ny = e->y + dy;
    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && map[ny][nx] != WALL_CHAR) {
        e->x = nx;
        e->y = ny;
    }
}

void playSound(int event) {
    // Sons simples para eventos
    // Frequência e duração em ms
    switch(event) {
        case 1: Beep(750, 100); break; // comer pellet
        case 2: Beep(1000, 150); break; // comer power pellet
        case 3: Beep(1500, 200); break; // comer fantasma
        case 4: Beep(400, 300); break; // perder vida
    }
}

void updateGhosts() {
    for (int g = 0; g < NUM_GHOSTS; g++) {
        int nx, ny;
        if (powerMode > 0) {
            // Fantasmas fogem do Pac-Man
            int tries = 0;
            int moved = 0;
            while (tries < 10 && !moved) {
                int dir = rand() % 4;
                int dx = 0, dy = 0;
                if (dir == 0) dx = 1;
                else if (dir == 1) dx = -1;
                else if (dir == 2) dy = 1;
                else dy = -1;
                int tx = ghosts[g].x + dx;
                int ty = ghosts[g].y + dy;
                int distNow = abs(ghosts[g].x - pacman.x) + abs(ghosts[g].y - pacman.y);
                int distNew = abs(tx - pacman.x) + abs(ty - pacman.y);
                if (tx >= 0 && tx < WIDTH && ty >= 0 && ty < HEIGHT && map[ty][tx] != WALL_CHAR && distNew > distNow) {
                    ghosts[g].x = tx;
                    ghosts[g].y = ty;
                    moved = 1;
                }
                tries++;
            }
            if (!moved) {
                // Se não achou movimento para fugir, fica parado
            }
        } else {
            // Fantasmas perseguem Pac-Man
            if (bfsNextStep(ghosts[g].x, ghosts[g].y, pacman.x, pacman.y, &nx, &ny)) {
                ghosts[g].x = nx;
                ghosts[g].y = ny;
            } else {
                // Se não achou caminho, move aleatoriamente
                int tries = 0;
                while (tries < 4) {
                    int dir = rand() % 4;
                    int dx = 0, dy = 0;
                    if (dir == 0) dx = 1;
                    else if (dir == 1) dx = -1;
                    else if (dir == 2) dy = 1;
                    else dy = -1;
                    int tx = ghosts[g].x + dx;
                    int ty = ghosts[g].y + dy;
                    if (tx >= 0 && tx < WIDTH && ty >= 0 && ty < HEIGHT && map[ty][tx] != WALL_CHAR) {
                        ghosts[g].x = tx;
                        ghosts[g].y = ty;
                        break;
                    }
                    tries++;
                }
            }
        }
    }
}

void checkCollisions() {
    char c = map[pacman.y][pacman.x];
    if (c == PELLET_CHAR) {
        score += 10;
        addScorePopup(pacman.x, pacman.y, 10);
        playSound(1);
        map[pacman.y][pacman.x] = EMPTY_CHAR;
        pelletsLeft--;
    } else if (c == POWER_CHAR) {
        score += 50;
        addScorePopup(pacman.x, pacman.y, 50);
        playSound(2);
        map[pacman.y][pacman.x] = EMPTY_CHAR;
        powerMode = POWER_DURATION;
        pelletsLeft--;
    }

    for (int g = 0; g < NUM_GHOSTS; g++) {
        if (pacman.x == ghosts[g].x && pacman.y == ghosts[g].y) {
            if (powerMode > 0) {
                score += 200;
                addScorePopup(ghosts[g].x, ghosts[g].y, 200);
                playSound(3);
                ghosts[g].x = 14;
                ghosts[g].y = 10;
            } else {
                lives--;
                playSound(4);
                pacman.x = 14;
                pacman.y = 17;
                if (lives <= 0) {
                    clearScreen();
                    printf("GAME OVER! Score: %d\n", score);
                    system("pause");
                    exit(0);
                }
            }
        }
    }
    if (pelletsLeft <= 0) {
        clearScreen();
        printf("YOU WIN! Score: %d\n", score);
        system("pause");
        exit(0);
    }
}

void gameLoop() {
    int frameCount = 0;
    while (1) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 0 || key == 224) { // Códigos especiais (setas)
                key = _getch();
                if (key == 72) moveEntity(&pacman, 0, -1); // seta cima
                else if (key == 80) moveEntity(&pacman, 0, 1); // seta baixo
                else if (key == 75) moveEntity(&pacman, -1, 0); // seta esquerda
                else if (key == 77) moveEntity(&pacman, 1, 0); // seta direita
            } else {
                // Também aceita WASD
                if (key == 'w' || key == 'W') moveEntity(&pacman, 0, -1);
                else if (key == 's' || key == 'S') moveEntity(&pacman, 0, 1);
                else if (key == 'a' || key == 'A') moveEntity(&pacman, -1, 0);
                else if (key == 'd' || key == 'D') moveEntity(&pacman, 1, 0);
            }
        }
        updateGhosts();
        checkCollisions();

        // Alterna a boca do Pac-Man a cada 5 frames para animação
        if (frameCount % 5 == 0) {
            pacmanMouthOpen = !pacmanMouthOpen;
        }

        drawMap();
        if (powerMode > 0) powerMode--;
        frameCount++;
        Sleep(100); // Frame rate mais suave
    }
}

int menu() {
    int option = 0;
    while (1) {
        clearScreen();
        printf("===== PAC-MAN =====\n");
        printf("%s Start Game\n", option == 0 ? ">" : " ");
        printf("%s Exit\n", option == 1 ? ">" : " ");

        char key = _getch();
        if ((key == 'w' || key == 'W' || key == 72) && option > 0) option--;
        if ((key == 's' || key == 'S' || key == 80) && option < 1) option++;
        if (key == '\r') return option;
    }
}

int main() {
    srand((unsigned int)time(NULL));
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    while (1) {
        int choice = menu();
        if (choice == 1) break;
        resetGame();
        clearScreen();
        gameLoop();
    }
    return 0;
}
