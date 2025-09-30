#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

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

HANDLE hConsole;


void irParaXY(int x, int y) {
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(hConsole, coord);
}


void definirCor(int cor) {
    SetConsoleTextAttribute(hConsole, cor);
}


void limparTela() {
    COORD topoEsquerdo = {0, 0};
    DWORD escrito, celulas = LARGURA * ALTURA;
    FillConsoleOutputCharacter(hConsole, ' ', celulas, topoEsquerdo, &escrito);
    FillConsoleOutputAttribute(hConsole, 7, celulas, topoEsquerdo, &escrito);
    SetConsoleCursorPosition(hConsole, topoEsquerdo);
}


void mostrarTelaGameOver(int pontuacao) {
    limparTela();
    definirCor(12);
    irParaXY(2, 4);
    printf("##############################");
    irParaXY(2, 5);
    printf("#                            #");
    irParaXY(2, 6);
    printf("#        GAME OVER!          #");
    irParaXY(2, 7);
    printf("#                            #");
    irParaXY(2, 8);
    printf("#      Pontuacao: %-8d      #", pontuacao);
    irParaXY(2, 9);
    printf("#                            #");
    irParaXY(2,10);
    printf("#   Pressione ESC para sair  #");
    irParaXY(2,11);
    printf("#                            #");
    irParaXY(2,12);
    printf("##############################");
    definirCor(7);
    irParaXY(0, 15);
    
    while (1) {
        int tecla = _getch();
        if (tecla == 27) break;
    }
}


void desenharMapa() {
    irParaXY(0, 0); 
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            char c = mapa[y][x];
            if (x == pacman.x && y == pacman.y) {
                definirCor(14); 
                putchar(PACMAN_CHAR);
            } else {
                int fantasmaAqui = -1;
                for (int g = 0; g < NUM_FANTASMAS; g++)
                    if (fantasmas[g].x == x && fantasmas[g].y == y)
                        fantasmaAqui = g;
                if (fantasmaAqui != -1) {
                    
                    if (modoPoder > 0)
                        definirCor(1); 
                    else
                        definirCor(9 + fantasmaAqui); 
                    putchar(FANTASMA_CHAR);
                } else {
                    if (c == PAREDE_CHAR) definirCor(11); 
                    else if (c == PILULA_CHAR) definirCor(15); 
                    else if (c == PODER_CHAR) definirCor(11); 
                    else definirCor(7);
                    putchar(c);
                }
            }
        }
        putchar('\n');
    }
    definirCor(7);
    irParaXY(0, ALTURA);

    
    if (modoPoder > 0)
        printf("Pontos: %-5d  Vidas: %-2d  PODER!      \n", pontuacao, vidas);
    else
        printf("Pontos: %-5d  Vidas: %-2d               \n", pontuacao, vidas);
    
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
                    exit(0);
                }
            }
        }
    }
    if (pilulasRestantes <= 0) {
        limparTela();
        irParaXY(0, 0);
        printf("VOCÊ VENCEU Pontuacao: %d\n", pontuacao);
        system("pause");
        exit(0);
    }
}


void loopJogo() {
    desenharMapa();
    while (1) {
        if (_kbhit()) {
            char tecla = _getch();
            
           
            if (tecla == 'p' || tecla == 'P') {
                irParaXY(0, ALTURA + 2);
                definirCor(12);
                printf("JOGO PAUSADO - Pressione qualquer tecla para continuar...");
                definirCor(7);
                _getch(); 
                irParaXY(0, ALTURA + 2);
                printf("                                             "); 
                desenharMapa(); 
                continue;
            }
            
            if (tecla == 'w' || tecla == 'W') moverEntidade(&pacman, 0, -1);
            if (tecla == 's' || tecla == 'S') moverEntidade(&pacman, 0, 1);
            if (tecla == 'a' || tecla == 'A') moverEntidade(&pacman, -1, 0);
            if (tecla == 'd' || tecla == 'D') moverEntidade(&pacman, 1, 0);
        }
        atualizarFantasmas();
        verificarColisoes();
        desenharMapa();
        if (modoPoder > 0) modoPoder--;
        Sleep(200);
    }
}

int menu() {
    int opcao = 0;
    while (1) {
        limparTela();
        irParaXY(0, 0);
        printf("===== PAC-MAN =====\n");
        printf("%s Iniciar Jogo\n", opcao == 0 ? ">" : " ");
        printf("%s Sair\n", opcao == 1 ? ">" : " ");

        char tecla = _getch();
        if ((tecla == 'w' || tecla == 'W') && opcao > 0) opcao--;
        if ((tecla == 's' || tecla == 'S') && opcao < 1) opcao++;
        if (tecla == '\r') return opcao;
        if (tecla == 27) exit(0); 
    }
}

int main() {
    srand((unsigned int)time(NULL));
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    while (1) {
        int escolha = menu();
        if (escolha == 1) break;
        reiniciarJogo();
        loopJogo();
    }
    return 0;
}