#include <stdio.h>
#include <stdlib.h>

//mapa

char mapa[10] [10] {
    "###########"
    "#.........#"
    "#.#######.#"
    "#.........#"
    "#####.#####"
    "#.........#"
    "#.#######.#"
    "#.........#"
    "#.........#"
    "###########"
};

//imprimir

void imprimeMapa(int px, py) {
    system ("clear");

    for (i = 0; i<10; i++){
        for (j = 0; j<10; j++){
            if (i == px && j == py ) printf("P"); //pacman
            else printf ("%c", mapa [i] [j]);
        }
        printf ("\n");
    }


}

//mover pacman

void mover(char comando, int *px , int *py){
    int novoX = *px, novoY = *py;


    if(comando == 'w') novoX--; //cima
    if(comando == 's') novoX++; //baixo
    if(comando == 'a') novoY--; //esquerda
    if(comando == 'd') novoX++; //direita

    if(mapa[novoX][novoY]  != '#') {
        *px = novoX;
        *py = novoY;

        if(mapa[*px][*py] == '.') {
            mapa[*px][*py] = ' ';
        }
    }
}
//progama principal


int main(){
    int px, py;
    char comando;

    while(1) {
        imprimeMapa(px, py);

        printf("\nUse W A S D para mover (CRTL+C para sair): ");
        scanf (" %c", &comando);

        mover (comando, &px, &py);

    }

    return 0;
}

