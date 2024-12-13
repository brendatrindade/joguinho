#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define LARGURA 91 // Largura do labirinto (deve ser ímpar)
#define ALTURA 27  // Altura do labirinto (deve ser ímpar)
#define ESPESSURA 3 // Espessura das paredes e caminhos

char labirinto[ALTURA][LARGURA];

void inicializaLabirinto();
void imprimeLabirinto();
int validaPosicao(int x, int y);
void geraLabirinto(int x, int y);
int main();

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
    if (x < ESPESSURA || y < ESPESSURA || x >= ALTURA - ESPESSURA || y >= LARGURA - ESPESSURA) {
        return 0; // Fora dos limites
    }
    return labirinto[x][y] == '#'; // Cavar apenas se a posição for parede
}

// Função recursiva para gerar o labirinto
void geraLabirinto(int x, int y) {
    for (int i = 0; i < ESPESSURA; i++) {
        for (int j = 0; j < ESPESSURA; j++) {
            labirinto[x + i][y + j] = ' '; // Marca como espaço
        }
    }

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
        int nx = x + dx[direcoes[i]] * ESPESSURA * 2;
        int ny = y + dy[direcoes[i]] * ESPESSURA * 2;

        if (validaPosicao(nx, ny)) {
            for (int j = 1; j < ESPESSURA * 2; j++) {
                for (int k = 0; k < ESPESSURA; k++) {
                    labirinto[x + dx[direcoes[i]] * j + k][y + dy[direcoes[i]] * j + k] = ' '; // Remove parede intermediária
                }
            }
            geraLabirinto(nx, ny);
        }
    }
}

int main() {
    srand(time(NULL)); // Semente para números aleatórios

    inicializaLabirinto();

    geraLabirinto(ESPESSURA, ESPESSURA);

    for (int i = 0; i < ESPESSURA; i++) {
        labirinto[ESPESSURA][i] = ' ';
        labirinto[ALTURA - ESPESSURA - 1][LARGURA - i - 1] = ' ';
    }

    // Posiciona o p1 e p2
    labirinto[ESPESSURA][0] = '1';
    labirinto[ALTURA - ESPESSURA - 1][LARGURA - ESPESSURA + 2] = '2';
    
    imprimeLabirinto();

    return 0;
}
