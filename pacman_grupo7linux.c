#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

#define PACMAN_CHAR 'C'
#define FANTASMA_CHAR 'G'
#define PAREDE_CHAR '#'
#define PILULA_CHAR '.'
#define PODER_CHAR 'o'
#define VAZIO_CHAR ' '
#define LARGURA 28
#define ALTURA 20
#define NUM_FANTASMAS 4
#define DURACAO_PODER 30

typedef struct {
    int x, y;
} Entidade;

Entidade pacman;
Entidade fantasmas[NUM_FANTASMAS];
int pontuacao = 0;
int vidas = 3;
int modoPoder = 0;
int pilulasRestantes = 0;
char mapa[ALTURA][LARGURA + 1];

char nivel[ALTURA][LARGURA + 1] = {
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

void irParaXY(int x, int y) {
    move(y, x);
}

void definirCor(int cor) {
    attron(COLOR_PAIR(cor));
}

void limparTela() {
    clear();
}

void mostrarTelaGameOver(int pontuacao) {
    limparTela();
    definirCor(2);
    irParaXY(2, 4);
    printw("##############################");
    irParaXY(2, 5);
    printw("#                            #");
    irParaXY(2, 6);
    printw("#        GAME OVER!          #");
    irParaXY(2, 7);
    printw("#                            #");
    irParaXY(2, 8);
    printw("#      Pontuacao: %-8d      #", pontuacao);
    irParaXY(2, 9);
    printw("#                            #");
    irParaXY(2,10);
    printw("#   Pressione ESC para sair  #");
    irParaXY(2,11);
    printw("#                            #");
    irParaXY(2,12);
    printw("##############################");
    definirCor(1);
    irParaXY(0, 15);
    refresh();
    int ch;
    while (1) {
        ch = getch();
        if (ch == 27) break;
    }
}

void desenharMapa() {
    irParaXY(0, 0); 
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            char c = mapa[y][x];
            if (x == pacman.x && y == pacman.y) {
                definirCor(4); // Amarelo
                addch(PACMAN_CHAR);
            } else {
                int fantasmaAqui = -1;
                for (int g = 0; g < NUM_FANTASMAS; g++)
                    if (fantasmas[g].x == x && fantasmas[g].y == y)
                        fantasmaAqui = g;
                if (fantasmaAqui != -1) {
                    if (modoPoder > 0)
                        definirCor(5); // Azul
                    else
                        definirCor(6 + fantasmaAqui); // Cores diferentes para fantasmas
                    addch(FANTASMA_CHAR);
                } else {
                    if (c == PAREDE_CHAR) definirCor(3); // Ciano
                    else if (c == PILULA_CHAR) definirCor(7); // Branco
                    else if (c == PODER_CHAR) definirCor(3); // Ciano
                    else definirCor(1);
                    addch(c);
                }
            }
        }
        addch('\n');
    }
    definirCor(1);
    irParaXY(0, ALTURA);

    if (modoPoder > 0)
        printw("Pontos: %-5d  Vidas: %-2d  PODER!      \n", pontuacao, vidas);
    else
        printw("Pontos: %-5d  Vidas: %-2d               \n", pontuacao, vidas);
}

void moverEntidade(Entidade *e, int dx, int dy) {
    int nx = e->x + dx;
    int ny = e->y + dy;
    if (nx < 0 || nx >= LARGURA || ny < 0 || ny >= ALTURA) return;
    if (mapa[ny][nx] != PAREDE_CHAR) {
        e->x = nx;
        e->y = ny;
    }
}

void atualizarFantasmas() {
    for (int g = 0; g < NUM_FANTASMAS; g++) {
        int tentarDir[4] = {0, 1, 2, 3};
        for (int i = 3; i > 0; i--) {
            int j = rand() % (i+1);
            int temp = tentarDir[i];
            tentarDir[i] = tentarDir[j];
            tentarDir[j] = temp;
        }
        for (int i = 0; i < 4; i++) {
            int dx = 0, dy = 0;
            if (tentarDir[i] == 0) dx = 1;
            else if (tentarDir[i] == 1) dx = -1;
            else if (tentarDir[i] == 2) dy = 1;
            else dy = -1;
            int nx = fantasmas[g].x + dx;
            int ny = fantasmas[g].y + dy;
            if (nx >= 0 && nx < LARGURA && ny >= 0 && ny < ALTURA && mapa[ny][nx] != PAREDE_CHAR) {
                fantasmas[g].x = nx;
                fantasmas[g].y = ny;
                break;
            }
        }
    }
}

void reiniciarJogo() {
    pilulasRestantes = 0;
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            mapa[y][x] = nivel[y][x];
            if (mapa[y][x] == PILULA_CHAR || mapa[y][x] == PODER_CHAR)
                pilulasRestantes++;
        }
    }
    pacman.x = 14;
    pacman.y = 17;
    fantasmas[0].x = 13; fantasmas[0].y = 10;
    fantasmas[1].x = 14; fantasmas[1].y = 10;
    fantasmas[2].x = 12; fantasmas[2].y = 10;
    fantasmas[3].x = 15; fantasmas[3].y = 10;
    pontuacao = 0;
    vidas = 3;
    modoPoder = 0;
}

void verificarColisoes() {
    char c = mapa[pacman.y][pacman.x];
    if (c == PILULA_CHAR) {
        pontuacao += 10;
        mapa[pacman.y][pacman.x] = VAZIO_CHAR;
        pilulasRestantes--;
    } else if (c == PODER_CHAR) {
        pontuacao += 50;
        mapa[pacman.y][pacman.x] = VAZIO_CHAR;
        modoPoder = DURACAO_PODER;
        pilulasRestantes--;
    }

    for (int g = 0; g < NUM_FANTASMAS; g++) {
        if (pacman.x == fantasmas[g].x && pacman.y == fantasmas[g].y) {
            if (modoPoder > 0) {
                pontuacao += 200;
                fantasmas[g].x = 14;
                fantasmas[g].y = 10;
            } else {
                vidas--;
                pacman.x = 14;
                pacman.y = 17;
                if (vidas <= 0) {
                    mostrarTelaGameOver(pontuacao);
                    endwin();
                    exit(0);
                }
            }
        }
    }
    if (pilulasRestantes <= 0) {
        limparTela();
        irParaXY(0, 0);
        printw("VOCÊ VENCEU Pontuacao: %d\n", pontuacao);
        refresh();
        getch();
        endwin();
        exit(0);
    }
}

void loopJogo() {
    desenharMapa();
    refresh();
    nodelay(stdscr, TRUE); // Não bloqueia no getch
    keypad(stdscr, TRUE);
    while (1) {
        int tecla = getch();
        if (tecla != ERR) {
            // PAUSA: tecla 'p' ou 'P'
            if (tecla == 'p' || tecla == 'P') {
                irParaXY(0, ALTURA + 2);
                definirCor(2);
                printw("JOGO PAUSADO - Pressione qualquer tecla para continuar...");
                definirCor(1);
                refresh();
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                irParaXY(0, ALTURA + 2);
                printw("                                             ");
                desenharMapa();
                refresh();
                continue;
            }
            // Movimentação
            if (tecla == 'w' || tecla == 'W' || tecla == KEY_UP) moverEntidade(&pacman, 0, -1);
            if (tecla == 's' || tecla == 'S' || tecla == KEY_DOWN) moverEntidade(&pacman, 0, 1);
            if (tecla == 'a' || tecla == 'A' || tecla == KEY_LEFT) moverEntidade(&pacman, -1, 0);
            if (tecla == 'd' || tecla == 'D' || tecla == KEY_RIGHT) moverEntidade(&pacman, 1, 0);
        }
        atualizarFantasmas();
        verificarColisoes();
        desenharMapa();
        refresh();
        if (modoPoder > 0) modoPoder--;
        usleep(200000); // 200ms
    }
}

int menu() {
    int opcao = 0;
    nodelay(stdscr, FALSE); // Espera tecla no menu
    while (1) {
        limparTela();
        irParaXY(0, 0);
        printw("===== PAC-MAN =====\n");
        printw("%s Iniciar Jogo\n", opcao == 0 ? ">" : " ");
        printw("%s Sair\n", opcao == 1 ? ">" : " ");
        refresh();
        int tecla = getch();
        if ((tecla == 'w' || tecla == 'W' || tecla == KEY_UP) && opcao > 0) opcao--;
        if ((tecla == 's' || tecla == 'S' || tecla == KEY_DOWN) && opcao < 1) opcao++;
        if (tecla == '\n' || tecla == KEY_ENTER) return opcao;
        if (tecla == 27) { endwin(); exit(0); }
    }
}

int main() {
    srand((unsigned int)time(NULL));
    initscr();
    start_color();
    curs_set(0);
    use_default_colors();
    // Definição de cores
    init_pair(1, COLOR_WHITE, -1); // Normal
    init_pair(2, COLOR_RED, -1); // Game Over / Pausa
    init_pair(3, COLOR_CYAN, -1); // Parede / Poder
    init_pair(4, COLOR_YELLOW, -1); // Pac-Man
    init_pair(5, COLOR_BLUE, -1); // Fantasma com poder
    init_pair(6, COLOR_MAGENTA, -1); // Fantasma 1
    init_pair(7, COLOR_WHITE, -1); // Pilula
    init_pair(8, COLOR_GREEN, -1); // Fantasma extra (se quiser mais cores)

    while (1) {
        int escolha = menu();
        if (escolha == 1) break;
        reiniciarJogo();
        loopJogo();
    }
    endwin();
    return 0;
}
