#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LARGURA  11 // Largura do labirinto (deve ser ímpar)
#define ALTURA 7 // Altura do labirinto (deve ser ímpar)

char labirinto[ALTURA][LARGURA];

void inicializaLabirinto();
void imprimeLabirinto();
int validaPosicao(int x, int y);
void geraLabirinto(int x, int y);


// Função para inicializar o labirinto com paredes
void inicializaLabirinto() {
    for (int i = 0; i < ALTURA; i++) {
        for (int j = 0; j < LARGURA; j++) {
            labirinto[i][j] = '#';
        }
    }
}

// Função para imprimir o labirinto
void imprimeLabirinto() {
    for (int i = 0; i < ALTURA; i++) {
        for (int j = 0; j < LARGURA; j++) {
            printf("%c", labirinto[i][j]);
        }
        printf("\n");
    }
}

// Vetores para movimentação (cima, baixo, esquerda, direita)
int dx[] = {0, 0, -1, 1};
int dy[] = {-1, 1, 0, 0};

// Função para verificar se a posição é válida e se pode ser cavada
int validaPosicao(int x, int y) {
    if (x <= 0 || y <= 0 || x >= ALTURA - 1 || y >= LARGURA - 1) {
        return 0; // Fora dos limites
    }
    int paredes = 0;
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (labirinto[nx][ny] == '#') paredes++;
    }
    return paredes >= 3; // Cavar apenas se a maioria ao redor for parede
}

// Função recursiva para gerar o labirinto
void geraLabirinto(int x, int y) {
    labirinto[x][y] = ' '; // Marca como espaço

    // Embaralhar as direções
    int direcoes[] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        int r = rand() % 4;
        int temp = direcoes[i];
        direcoes[i] = direcoes[r];
        direcoes[r] = temp;
    }

    // Tentar cada direção
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[direcoes[i]] * 2;
        int ny = y + dy[direcoes[i]] * 2;

        if (validaPosicao(nx, ny)) {
            labirinto[x + dx[direcoes[i]]][y + dy[direcoes[i]]] = ' '; // Remove parede intermediária
            geraLabirinto(nx, ny);
        }
    }
}

int main() {
    srand(time(NULL)); // Semente para números aleatórios

    inicializaLabirinto();

    geraLabirinto(1, 1);

    // Posiciona o p1 e p2
    labirinto[1][0] = '1';
    labirinto[ALTURA - 2][LARGURA - 1] = '2';

    imprimeLabirinto();

    return 0;
}
