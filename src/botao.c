#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "proc_grafico.h"
#include "mouse.h"
#include "acelerometro.c"
#include <unistd.h>
#include <stdint.h>

extern void fecha_dev_mem();
extern void inicializa_fpga();
extern void escreve_bloco(uint16_t posicao, uint16_t cor);
extern void apaga_bloco(uint16_t posicao);
extern void altera_cor_bg(uint16_t cor, uint8_t registrador);
extern void apaga_cor_bg(uint8_t registrador);
extern void exibe_sprite(uint8_t sp, uint32_t xy, uint16_t offset, uint8_t registrador);
extern int acess_btn();

extern int abre_mouse();
extern int le_mouse_orientacao(int fd);
extern int le_mouse_direcao(int fd, uint16_t *x, uint16_t *y);
extern int teste_leitura(int fd);

#define LARGURA 80 // Largura do labirinto (deve ser ímpar)
#define ALTURA 60  // Altura do labirinto (deve ser ímpar)
#define ESPESSURA 4 // Espessura das paredes e caminhos
#define BUTTON 0

char labirinto[ALTURA][LARGURA];

void inicializaLabirinto();
void imprimeLabirinto();
int validaPosicao(int x, int y);
void geraLabirinto(int x, int y);
void imprimeLabirintoVGA();
void apagaLabirinto();
void move_sprite();
void colisao_labirinto();
int main();

// Vetores para movimentação (cima, baixo, esquerda, direita)
int dx[] = {0, 0, -1, 1};
int dy[] = {-1, 1, 0, 0};

// Função para inicializar o labirinto com paredes
void inicializaLabirinto() {
    for (int i = 0; i < ALTURA; i++) {
        for (int j = 0; j < LARGURA; j++) {
            labirinto[i][j] = '#';
        }
    }
}

// Função para imprimir o labirinto
void imprimeLabirintoTerminal() {
    for (int i = 0; i < ALTURA; i++) {
        for (int j = 0; j < LARGURA; j++) {
            printf("%c", labirinto[i][j]);
        }
        printf("\n");
    }
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

// Função para verificar se a posição é válida e se pode ser cavada
int validaPosicao(int x, int y) {
    if (x < ESPESSURA || y < ESPESSURA || x >= ALTURA - ESPESSURA || y >= LARGURA - ESPESSURA) {
        return 0; // Fora dos limites
    }
    return labirinto[x][y] == '#'; // Cavar apenas se a posição for parede
}

void imprimeLabirintoVGA() {
    altera_cor_bg(BRANCO, 0); //pintando fundo

    int i, j;
    for(i=0;i < ALTURA;i++) { //linhas
        for(j=0; j < LARGURA; j++) { //colunas
            if(labirinto[i][j] == '#'){
                escreve_bloco( j + (i * 80), AZUL_ESCURO);
            } else if(labirinto[i][j] == 'K'){
                escreve_bloco( j + (i * 80), PRETO);
            }
        }
    }
}

void apagaLabirinto() {
    apaga_cor_bg(0);
    int i, j;
    for(i=0;i < ALTURA;i++) { //linhas
        for(j=0; j < LARGURA; j++) { //colunas
            apaga_bloco( j + (i * 80));
        }
    }
}

void configuracoesGerais() {
    inicializa_fpga();
    configurar_acelerometro();
    inicializaLabirinto();
    geraLabirinto(ESPESSURA, ESPESSURA);
}

void button() {
  int btn;
  while(1) {
    btn = acess_btn();
    if(btn == 15){ //botão 0
        printf("Não esta apertando nenhum botão \n");
    } else if( btn == 14) { 
        printf("Print botão: 0 \n");
    } else if(btn == 13) { 
        printf("Print botão: 2 \n");
    } else if (btn == 11) { 
        printf("Print botão: 3 \n"); 
    } else if (btn == 7) {
        printf("Print botão: 4 \n");
    }
    }
}

void buttonTeste(){
    int btn = acess_btn();
    while(btn != 14 && btn != 13 && btn != 11 && btn != 7 ) {
        btn = acess_btn();
    }
}

int main() {
    configuracoesGerais();

    buttonTeste();
    for (int i = 0; i < 1500; i++) {
        //imprimeLabirintoVGA();
        apagaLabirinto();
    }

    desmapear_memoria();
    fecha_dev_mem();
    return 0;
}


