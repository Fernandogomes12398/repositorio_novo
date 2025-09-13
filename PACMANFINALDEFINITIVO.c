#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

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

HANDLE hConsole;

// Move o cursor para uma posição específica no console
void gotoXY(int x, int y) {
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(hConsole, coord);
}

// Muda cor do texto
void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

// Limpa tela de forma funcional
void clearScreen() {
    COORD topLeft = {0, 0};
    DWORD written, cells = WIDTH * HEIGHT;
    FillConsoleOutputCharacter(hConsole, ' ', cells, topLeft, &written);
    FillConsoleOutputAttribute(hConsole, 7, cells, topLeft, &written);
    SetConsoleCursorPosition(hConsole, topLeft);
}

// Tela de GAME OVER (mostra só ESC para sair)
void showGameOverScreen(int score) {
    clearScreen();
    setColor(12); // Vermelho
    gotoXY(2, 4);
    printf("##############################");
    gotoXY(2, 5);
    printf("#                            #");
    gotoXY(2, 6);
    printf("#        GAME OVER!          #");
    gotoXY(2, 7);
    printf("#                            #");
    gotoXY(2, 8);
    printf("#      Pontuacao: %-8d      #", score);
    gotoXY(2, 9);
    printf("#                            #");
    gotoXY(2,10);
    printf("#   Pressione ESC para sair  #");
    gotoXY(2,11);
    printf("#                            #");
    gotoXY(2,12);
    printf("##############################");
    setColor(7);
    gotoXY(0, 15);
    // Espera ESC para sair
    while (1) {
        int key = _getch();
        if (key == 27) break;
    }
}

// Desenha o mapa usando posição absoluta do cursor
void drawMap() {
    gotoXY(0, 0); // Apenas move o cursor para o início
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            char c = map[y][x];
            if (x == pacman.x && y == pacman.y) {
                setColor(14); // Amarelo
                putchar(PACMAN_CHAR);
            } else {
                int ghostHere = -1;
                for (int g = 0; g < NUM_GHOSTS; g++)
                    if (ghosts[g].x == x && ghosts[g].y == y)
                        ghostHere = g;
                if (ghostHere != -1) {
                    // Fantasma muda de cor para azul escuro no modo POWER
                    if (powerMode > 0)
                        setColor(1); // 1 = Azul escuro (Windows Console)
                    else
                        setColor(9 + ghostHere); // Cor padrão dos fantasmas
                    putchar(GHOST_CHAR);
                } else {
                    if (c == WALL_CHAR) setColor(11); // Azul claro
                    else if (c == PELLET_CHAR) setColor(15); // Branco
                    else if (c == POWER_CHAR) setColor(11); // Ciano
                    else setColor(7);
                    putchar(c);
                }
            }
        }
        putchar('\n');
    }
    setColor(7);
    gotoXY(0, HEIGHT);

    // Mostra "POWER!" apenas enquanto estiver com o poder
    if (powerMode > 0)
        printf("Pontos: %d  Vidas: %d  POWER!\n", score, lives);
    else
        printf("Pontos: %d  Vidas: %d\n", score, lives);
    // NÃO mostra mais "Pressione ESC para sair" durante o jogo!
}

// Movimentação segura
void moveEntity(Entity *e, int dx, int dy) {
    int nx = e->x + dx;
    int ny = e->y + dy;
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) return;
    if (map[ny][nx] != WALL_CHAR) {
        e->x = nx;
        e->y = ny;
    }
}

// Fantasmas não atravessam paredes/limites
void updateGhosts() {
    for (int g = 0; g < NUM_GHOSTS; g++) {
        int tryDir[4] = {0, 1, 2, 3};
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
                    showGameOverScreen(score);
                    exit(0);
                }
            }
        }
    }
    if (pelletsLeft <= 0) {
        clearScreen();
        gotoXY(0, 0);
        printf("VOCÊ VENCEU Score: %d\n", score);
        system("pause");
        exit(0);
    }
}

// Pause: tecla 'p' ou 'P' para pausar e qualquer tecla para voltar
void gameLoop() {
    drawMap();
    while (1) {
        if (_kbhit()) {
            char key = _getch();
            // Não existe mais sair do jogo com ESC!
            // Pausa
            if (key == 'p' || key == 'P') {
                gotoXY(0, HEIGHT + 2);
                setColor(12);
                printf("JOGO PAUSADO - Pressione qualquer tecla para continuar...");
                setColor(7);
                _getch(); // espera qualquer tecla
                gotoXY(0, HEIGHT + 2);
                printf("                                             "); // limpa mensagem
                drawMap(); // redesenha por segurança
                continue;
            }
            // Movimentos normais
            if (key == 'w' || key == 'W') moveEntity(&pacman, 0, -1);
            if (key == 's' || key == 'S') moveEntity(&pacman, 0, 1);
            if (key == 'a' || key == 'A') moveEntity(&pacman, -1, 0);
            if (key == 'd' || key == 'D') moveEntity(&pacman, 1, 0);
        }
        updateGhosts();
        checkCollisions();
        drawMap();
        if (powerMode > 0) powerMode--;
        Sleep(200);
    }
}

int menu() {
    int option = 0;
    while (1) {
        clearScreen();
        gotoXY(0, 0);
        printf("===== PAC-MAN =====\n");
        printf("%s Start Game\n", option == 0 ? ">" : " ");
        printf("%s Exit\n", option == 1 ? ">" : " ");

        char key = _getch();
        if ((key == 'w' || key == 'W') && option > 0) option--;
        if ((key == 's' || key == 'S') && option < 1) option++;
        if (key == '\r') return option;
        if (key == 27) exit(0); // ESC para sair também no menu
    }
}

int main() {
    srand((unsigned int)time(NULL));
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    while (1) {
        int choice = menu();
        if (choice == 1) break;
        resetGame();
        gameLoop();
    }
    return 0;
}
