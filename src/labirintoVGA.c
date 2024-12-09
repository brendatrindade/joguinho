#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "proc_grafico.h"
#include "mouse.h"
#include "acelerometro.c"
#include <math.h>

extern void fecha_dev_mem();
extern void inicializa_fpga();
extern void escreve_bloco(uint16_t posicao, uint16_t cor);
extern void apaga_bloco(uint16_t posicao);
extern void altera_cor_bg(uint16_t cor, uint8_t registrador);
extern void apaga_cor_bg(uint8_t registrador);
extern void exibe_sprite(uint8_t sp, uint32_t xy, uint16_t offset, uint8_t registrador);

extern int abre_mouse();
extern int le_mouse_orientacao(int fd);
extern int le_mouse_direcao(int fd, uint16_t *x, uint16_t *y);
extern int teste_leitura(int fd);

#define LARGURA 80 // Largura do labirinto (deve ser ímpar)
#define ALTURA 60  // Altura do labirinto (deve ser ímpar)
#define ESPESSURA 4 // Espessura das paredes e caminhos

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
void imprimeLabirintoTerminal() {
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

// Exibe e move um sprite armazenado na memoria de sprites pela tela
void move_sprite() {
    #define INICIO_Y 358410
    #define LIMITE_Y 358850
    int x = 0, y = 0;

    #define mascara_10bits 0b1111111111
    uint16_t pos_x = 350;
    uint16_t pos_y = 240;

    pos_x &= mascara_10bits;
    pos_y &= mascara_10bits;
    
    uint32_t pos_xy_20b;
    pos_xy_20b = (pos_x << 10 | pos_y);
    uint32_t pos_xy_20b_ant = (pos_xy_20b); //inicia com posicao anterior igual a posicao atual

    int fd = abre_mouse();

    while (1) {
        int direcao = le_mouse_direcao(fd, &pos_x, &pos_y);
        int orientacao = le_mouse_orientacao(fd);

        // apaga o sprite exibido na posicao anterior
        exibe_sprite(0, pos_xy_20b_ant, 5, 1); // sp = 0 -> desabilita sprite

       
        pos_x &= mascara_10bits;
        pos_y &= mascara_10bits;
        pos_xy_20b = (pos_x << 10 | pos_y);
        
         // exibe o sprite na posicao atual
        exibe_sprite(1, pos_xy_20b, 5, 1); // sp = 1 -> habilita sprite
        pos_xy_20b_ant = pos_xy_20b;

    }
}

void colisao() {
    // tela 640 x 480 
    // labirinto 60 x 80
    // i -> 10,66 j -> 6
    #define mascara_10bits 0b1111111111
    #define limite_superior_eixoY 33  //esta para o y 5 do labirinto 
    #define limite_inferior_eixoY 428 //esta para o y 75 do labirinto 
    #define limite_esquerdo_eixoX 35 //esta para o x 5 do labirinto 
    #define limite_direito_eixoX 550 //esta para o x 59 do labirinto

    uint16_t pos_x = 0;
    uint16_t pos_y = 30;

    pos_x &= mascara_10bits;
    pos_y &= mascara_10bits;
    
    uint32_t pos_xy_20b;
    pos_xy_20b = (pos_x << 10 | pos_y);
    
    uint32_t pos_xy_20b_ant = (pos_xy_20b); //inicia com posicao anterior igual a posicao atual

    int direcao_sprite;
    int velocidade = 1;    

    while (1) {
        pos_y = (pos_xy_20b & mascara_10bits);
        pos_x = ((pos_xy_20b >> 10) & mascara_10bits);

        direcao_sprite = get_movimento(&velocidade); //8 cima, 2 baixo, 6 direita, 4 esquerda, 0 sem movimento

        //apaga o sprite exibido na posicao anterior
        exibe_sprite(0, pos_xy_20b_ant, 1, 1);//sp = 0 - desabilita sprite
        pos_xy_20b_ant = pos_xy_20b;
    
        //exibe o sprite na posicao atual
        exibe_sprite(1, pos_xy_20b, 1, 1);//sp = 1 - habilita sprite

        //descendo
        if ( direcao_sprite == 2 ){
            if ( (pos_y += (5*velocidade)) < limite_inferior_eixoY ) {
                pos_y += (5*velocidade); //posicao atual + 10 * (1,2 ou 3)
            } else {
                pos_y == limite_inferior_eixoY; //fica no limite da tela
            }
        }
        //subindo
        else if ( direcao_sprite == 8 ){
            if ( (pos_y -= (5*velocidade)) > limite_superior_eixoY) {
                pos_y -= (5*velocidade); //posicao atual + 10 * (1,2 ou 3)
            } else {
                pos_y == limite_superior_eixoY; //fica no limite da tela
            }
        }
        //direita
        else if ( direcao_sprite == 6 ){
            if ( (pos_x += (5*velocidade)) < limite_direito_eixoX) { // 
                pos_x += (5*velocidade); //posicao atual + 10 * (1,2 ou 3)
            } else {
                pos_x == limite_direito_eixoX; //fica no limite da tela
            }
        }
        //esquerda
        else if ( direcao_sprite == 4 ){
            if ( (pos_x -= (5*velocidade)) > limite_esquerdo_eixoX) {                             
                pos_x -= (5*velocidade); //posicao atual + 10 * (1,2 ou 3)
            } else {
                pos_x == limite_direito_eixoX; //fica no limite da tela
            }
        }

        float divisor_i = 10.66;
        int divisor_j = 6;
        float i_float = pos_x / divisor_i;
        int j = (pos_y / divisor_j) - 1;
        int i = (round(i_float)) - 1;
        
        printf("x = %d; y = %d\n", i, j);

        pos_xy_20b = (pos_x << 10 | pos_y);
        usleep(10000);
    }
}

int main() {
    inicializa_fpga();
    configurar_acelerometro();

    srand(time(NULL)); // Semente para números aleatórios

    inicializaLabirinto();

    geraLabirinto(ESPESSURA, ESPESSURA);

    // Coloca as saídas
    for (int i = 0; i <= ESPESSURA; i++) {
        labirinto[ESPESSURA][i] = ' ';
        labirinto[ESPESSURA + 1][i] = ' ';
        labirinto[ALTURA - ESPESSURA - 1][LARGURA - ESPESSURA - i] = ' '; 
        labirinto[ALTURA - ESPESSURA - 2][LARGURA - ESPESSURA - i] = ' '; 
    }

    // Posiciona o p1 e p2
    labirinto[ESPESSURA][0] = '1';
    labirinto[ALTURA - ESPESSURA - 2][LARGURA - ESPESSURA] = '2';

    
    for(int i = 0; i < ALTURA; i++) { 
        for(int j = 0; j < LARGURA; j++) {
            labirinto[i][LARGURA - 1] = 'K';
            labirinto[i][LARGURA - 2] = 'K';
            labirinto[i][LARGURA - 3] = 'K';
        }
    }

    #define mascara_10bits 0b1111111111
    uint16_t pos_x = 590;
    uint16_t pos_y = 430;
    pos_x &= mascara_10bits;
    pos_y &= mascara_10bits;
    uint32_t pos_xy_20b;
    pos_xy_20b = (pos_x << 10 | pos_y);

    for(int i = 0; i < ALTURA; i++) { 
        for(int j = 0; j < LARGURA; j++) {
            if(labirinto[i][j] == '1'){
                exibe_sprite(1, pos_xy_20b, 1, 1);
            }
            else if(labirinto[i][j] == '2'){
                exibe_sprite(1, pos_xy_20b, 5, 2);
            }
        }
    }
    
    imprimeLabirintoTerminal();
    
    for (int i = 0; i < 1500; i++) {
        //imprimeLabirintoVGA();
        //apagaLabirinto();
    }
    
    //move_sprite();
    //colisao();

    desmapear_memoria();
    fecha_dev_mem();
    return 0;
}