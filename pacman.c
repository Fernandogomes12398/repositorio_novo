#include <stdio.h>
#include <stdlib.h>

// mapa
char mapa[10][11] = {
    "###########",
    "#.........#",
    "#.#######.#",
    "#.........#",
    "#####.#####",
    "#.........#",
    "#.#######.#",
    "#.........#",
    "#.........#",
    "###########"
};

// imprimir
void imprimeMapa(int px, int py) {
    system("clear");
    int i, j;
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 11; j++) {
            if (i == px && j == py)
                printf("P"); // pacman
            else
                printf("%c", mapa[i][j]);
        }
        printf("\n");
    }
}

// mover pacman
void mover(char comando, int *px, int *py) {
    int novoX = *px, novoY = *py;

    if (comando == 'w') novoX--; // cima
    if (comando == 's') novoX++; // baixo
    if (comando == 'a') novoY--; // esquerda
    if (comando == 'd') novoY++; // direita

    // checa limites
    if (novoX >= 0 && novoX < 10 && novoY >= 0 && novoY < 11 && mapa[novoX][novoY] != '#') {
        *px = novoX;
        *py = novoY;

        if (mapa[*px][*py] == '.') {
            mapa[*px][*py] = ' ';
        }
    }
}

// programa principal
int main() {
    int px = 1, py = 1; // posição inicial válida
    char comando;

    while (1) {
        imprimeMapa(px, py);

        printf("\nUse W A S D para mover (CTRL+C para sair): ");
        scanf(" %c", &comando);

        mover(comando, &px, &py);
    }

    return 0;
}