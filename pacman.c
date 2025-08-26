#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define PACMAN_CHAR 'C'
#define GHOST_CHAR 'G'
#define WALL_CHAR '#'
#define PELLET_CHAR '.'
#define POWER_CHAR 'o'
#define EMPTY_CHAR ' '
#define WIDTH 28
#define HEIGHT 20
#define NUM_GHOSTS 4
#define POWER_DURATION 30

typedef struct {
    int x, y;
} Entity;

Entity pacman;
Entity ghosts[NUM_GHOSTS];
int score = 0;
int lives = 3;
int powerMode = 0;
int pelletsLeft = 0;
char map[HEIGHT][WIDTH + 1];

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
    switch (color) {
        case 14: printf("\033[1;33m"); break; // yellow
        case 11: printf("\033[1;36m"); break; // cyan
        case 15: printf("\033[1;37m"); break; // white
        case 7: printf("\033[0;37m"); break;  // gray
        case 9: printf("\033[1;31m"); break;  // ghost 1 - red
        case 10: printf("\033[1;32m"); break; // ghost 2 - green
        case 12: printf("\033[1;35m"); break; // ghost 4 - magenta
        default: printf("\033[0m"); break;
    }
}

void resetColor() {
    printf("\033[0m");
}

char getch() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);           // Salva estado atual
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);         // Modo raw
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restaura estado
    return ch;
}

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

// Corrige a inicialização e repetição do mapa e pellets
void resetGame() {
    pelletsLeft = 0;
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
}

// Corrige o desenhar do mapa para não sobrescrever fantasmas repetidos e não duplicar o mapa
void drawMap() {
    printf("\033[H\033[J"); // Limpa tela
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int isPacman = (x == pacman.x && y == pacman.y);
            int ghostIndex = -1;
            for (int g = 0; g < NUM_GHOSTS; g++)
                if (ghosts[g].x == x && ghosts[g].y == y)
                    ghostIndex = g;
            if (isPacman) {
                setColor(14);
                printf("%c", PACMAN_CHAR);
                resetColor();
            } else if (ghostIndex >= 0) {
                setColor(9 + ghostIndex);
                printf("%c", GHOST_CHAR);
                resetColor();
            } else {
                char c = map[y][x];
                if (c == WALL_CHAR) setColor(11);
                else if (c == PELLET_CHAR) setColor(15);
                else if (c == POWER_CHAR) setColor(11);
                else setColor(7);
                printf("%c", c);
                resetColor();
            }
        }
        printf("\n");
    }
    printf("Score: %d  Lives: %d  %s\n", score, lives, powerMode ? "POWER!" : "");
}

// Corrige movimentação para não sair dos limites do mapa
void moveEntity(Entity *e, int dx, int dy) {
    int nx = e->x + dx;
    int ny = e->y + dy;
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) return;
    if (map[ny][nx] != WALL_CHAR) {
        e->x = nx;
        e->y = ny;
    }
}

// Corrige movimentação dos fantasmas para não ficar presos em paredes ou sair do mapa
void updateGhosts() {
    for (int g = 0; g < NUM_GHOSTS; g++) {
        int tryDir[4] = {0, 1, 2, 3};
        // Shuffle directions to try random order
        for (int i = 3; i > 0; i--) {
            int j = rand() % (i+1);
            int temp = tryDir[i];
            tryDir[i] = tryDir[j];
            tryDir[j] = temp;
        }
        for (int i = 0; i < 4; i++) {
            int dx = 0, dy = 0;
            if (tryDir[i] == 0) dx = 1;
            else if (tryDir[i] == 1) dx = -1;
            else if (tryDir[i] == 2) dy = 1;
            else dy = -1;
            int nx = ghosts[g].x + dx;
            int ny = ghosts[g].y + dy;
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && map[ny][nx] != WALL_CHAR) {
                ghosts[g].x = nx;
                ghosts[g].y = ny;
                break;
            }
        }
    }
}

// Corrige fim de jogo e fim de fase
void checkCollisions() {
    char c = map[pacman.y][pacman.x];
    if (c == PELLET_CHAR) {
        score += 10;
        map[pacman.y][pacman.x] = EMPTY_CHAR;
        pelletsLeft--;
    } else if (c == POWER_CHAR) {
        score += 50;
        map[pacman.y][pacman.x] = EMPTY_CHAR;
        powerMode = POWER_DURATION;
        pelletsLeft--;
    }

    for (int g = 0; g < NUM_GHOSTS; g++) {
        if (pacman.x == ghosts[g].x && pacman.y == ghosts[g].y) {
            if (powerMode > 0) {
                score += 200;
                ghosts[g].x = 14;
                ghosts[g].y = 10;
            } else {
                lives--;
                pacman.x = 14;
                pacman.y = 17;
                if (lives <= 0) {
                    printf("\033[H\033[J");
                    printf("GAME OVER! Score: %d\n", score);
                    printf("Pressione Enter para sair...");
                    getchar();
                    exit(0);
                }
            }
        }
    }
    if (pelletsLeft <= 0) {
        printf("\033[H\033[J");
        printf("YOU WIN! Score: %d\n", score);
        printf("Pressione Enter para sair...");
        getchar();
        exit(0);
    }
}

void gameLoop() {
    while (1) {
        if (kbhit()) {
            char key = getch();
            if (key == 'w' || key == 'W') moveEntity(&pacman, 0, -1);
            if (key == 's' || key == 'S') moveEntity(&pacman, 0, 1);
            if (key == 'a' || key == 'A') moveEntity(&pacman, -1, 0);
            if (key == 'd' || key == 'D') moveEntity(&pacman, 1, 0);
        }
        updateGhosts();
        checkCollisions();
        drawMap();
        if (powerMode > 0) powerMode--;
        usleep(200000); // 200ms
    }
}

int menu() {
    int option = 0;
    while (1) {
        printf("\033[H\033[J");
        printf("===== PAC-MAN =====\n");
        printf("%s Start Game\n", option == 0 ? ">" : " ");
        printf("%s Exit\n", option == 1 ? ">" : " ");

        char key = getch();
        if ((key == 'w' || key == 'W') && option > 0) option--;
        if ((key == 's' || key == 'S') && option < 1) option++;
        if (key == '\n' || key == '\r') return option;
    }
}

int main() {
    srand(time(NULL));
    printf("\033[?25l"); // Hide cursor

    while (1) {
        int choice = menu();
        if (choice == 1) break;
        resetGame();
        gameLoop();
    }

    printf("\033[?25h"); // Show cursor
    resetColor();
    return 0;
}
