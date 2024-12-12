#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "proc_grafico.h"
#include "mouse.h"
//#include "acelerometro.c"
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

#define ALTURA_LAB 60   // Altura do labirinto
#define LARGURA_LAB 80  // Largura do labirinto
#define ESPESSURA 4     // Espessura das paredes e caminhos
#define mascara_10bits 0b1111111111

char labirinto[ALTURA_LAB][LARGURA_LAB];

// Vetores para movimentação (cima, baixo, esquerda, direita)
int dx[] = {0, 0, -1, 1};
int dy[] = {-1, 1, 0, 0};

int arredonda_div(float numero_float);
void inicializaLabirinto();
void imprimeLabirinto();
int validaPosicao(int x, int y);
void geraLabirinto(int x, int y);
void imprimeLabirintoVGA();
void apagaLabirinto();
void converte_sprite_para_labirinto(uint16_t pos_x, uint16_t pos_y, int *x_lab, int *y_lab);
void converte_labirinto_para_sprite(int x_lab, int y_lab, uint16_t *pos_x, uint16_t *pos_y);
void colisao_labirinto();
int main();

int arredonda_div(float numero_float) {
    int inteiro = (int)numero_float;
    float decimal = numero_float - inteiro;

    //arredonda para cima
    if (decimal > 0.49) {
        int arredondado = inteiro + 1;
        return arredondado;
    }
    //mantem a parte inteira
    return inteiro;
}

// Função para inicializar o labirinto com paredes
void inicializaLabirinto() {
    for (int i = 0; i < ALTURA_LAB; i++) {
        for (int j = 0; j < LARGURA_LAB; j++) {
            labirinto[i][j] = '#';
        }
    }
}

// Função para imprimir o labirinto
void imprimeLabirintoTerminal() {
    for (int i = 0; i < ALTURA_LAB; i++) {
        for (int j = 0; j < LARGURA_LAB; j++) {
            printf("%c", labirinto[i][j]);
        }
        printf("\n");
    }
}

// Função para verificar se a posição é válida e se pode ser cavada
int validaPosicao(int x, int y) {
    if (x < ESPESSURA || y < ESPESSURA || x >= ALTURA_LAB - ESPESSURA || y >= LARGURA_LAB - ESPESSURA) {
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
    //altera_cor_bg(BRANCO, 0); //pintando fundo
    int i, j;
    for(i=0;i < ALTURA_LAB;i++) { //linhas
        for(j=0; j < LARGURA_LAB; j++) { //colunas
            if(labirinto[i][j] == '#'){
                //escreve_bloco( j + (i * 80), AZUL_ESCURO);
            } else if(labirinto[i][j] == 'K'){
                //escreve_bloco( j + (i * 80), PRETO);
            }
        }
    }
}

void apagaLabirinto() {
    //apaga_cor_bg(0);
    int i, j;
    for(i=0; i < ALTURA_LAB; i++) { //linhas
        for(j=0; j < LARGURA_LAB; j++) { //colunas
                //apaga_bloco( j + (i * 80));
        }
    }
}

void converte_sprite_para_labirinto(uint16_t pos_x, uint16_t pos_y, int *x_lab, int *y_lab){
    // sprite pos_x < 639 <-> y_lab < 79 labirinto
    // sprite pos_y < 479 <-> x_lab < 59 labirinto

    float float_x = (pos_x / 8); 
    float float_y = (pos_y / 8);

    if (float_x > 0){
        *y_lab = arredonda_div(float_x);
        //*y_lab -= 1;
    } else {
        *y_lab = arredonda_div(float_x);
    }
    if (float_y > 0){
        *x_lab = arredonda_div(float_y);
        //*x_lab -= 1;
    } else {
        *x_lab = arredonda_div(float_y);
    }
    //printf("SPRITE - X sprite: %d Y sprite: %d \nLABIRINTO - X lab: %d Y lab: %d \n\n", pos_x, pos_y, *x_lab, *y_lab);
}

void converte_labirinto_para_sprite(int x_lab, int y_lab, uint16_t *pos_x, uint16_t *pos_y){
    // sprite pos_x < 639 <-> y_lab < 79 labirinto
    // sprite pos_y < 479 <-> x_lab < 59 labirinto

    if (x_lab == 0){
        *pos_y = x_lab * 8; 
    } else {
        float float_x = (x_lab + 1) * 8; 
        *pos_y = arredonda_div(float_x);
        *pos_y -= 1;
    }
    if (y_lab == 0){
        *pos_x = y_lab * 8;
    } else {
        float float_y = (y_lab + 1) * 8;
        *pos_x = arredonda_div(float_y);
        *pos_x -= 1;
    }
    //printf("LABIRINTO - X lab: %d Y lab: %d \nSPRITE - X sprite: %d Y sprite: %d \n\n", x_lab, y_lab, *pos_x, *pos_y);
}


//Verifica inicio e fim do sprite
int colide(int prox_pos_x, int prox_pos_y) { 
    int verifica_x[] = {prox_pos_x, prox_pos_x + 1};
    int verifica_y[] = {prox_pos_y, prox_pos_y + 1};
    int i,j,q,w;
    for( q= 0; q < 2; q++) {
        for( w = 0; w < 2; w++) {
            converte_sprite_para_labirinto(verifica_x[q], verifica_y[w], &i, &j);
            printf("Vou verificar parede - labirinto %d,%d = %c\n", i,j, labirinto[i][j]);
            if (labirinto[i][j] == '#') {
                printf("Tem parede - labirinto %d,%d = %c\n", i,j, labirinto[i][j]);
                return 1; //Tem parede
            }
        }
    }
    printf("Sem parede - labirinto %c\n", labirinto[i][j]);
    return 0; //Sem colisao 
}

/*
void colisao_labirinto2() {
    uint16_t pos_x = 0;
    uint16_t pos_y = 30; //posicao inicial p1

    pos_x &= mascara_10bits;
    pos_y &= mascara_10bits;
    
    uint32_t pos_xy_20b;
    pos_xy_20b = (pos_x << 10 | pos_y);
    
    uint32_t pos_xy_20b_ant = (pos_xy_20b); //inicia com posicao anterior igual a posicao atual

    int direcao_sprite, movimento, colidiu;
    uint16_t prox_pos_y, prox_pos_x;

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

        movimento = 5*velocidade;

        //descendo
        if ( direcao_sprite == 2 ){
            prox_pos_y = pos_y + movimento;
            colidiu = colide(pos_x, prox_pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_y = prox_pos_y;
            }
        }
        //subindo
        else if ( direcao_sprite == 8 ){
            prox_pos_y = pos_y - movimento;
            colidiu = colide(pos_x, prox_pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_y = prox_pos_y;
            }
        }
        //direita
        else if ( direcao_sprite == 6 ){
            prox_pos_x = pos_x + movimento;
            colidiu = colide(prox_pos_x, pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_x = prox_pos_x; 
            }
        }
        //esquerda
        else if ( direcao_sprite == 4 ){
            prox_pos_x = pos_x - movimento;
            colidiu = colide(prox_pos_x, pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_x = prox_pos_x; 
            }
        }
        pos_xy_20b = (pos_x << 10 | pos_y);
        usleep(10000);
    }
}


void colisao_labirinto() {
    uint16_t pos_x = 0;
    uint16_t pos_y = 39; //posicao inicial p1

    pos_x &= mascara_10bits;
    pos_y &= mascara_10bits;
    
    uint32_t pos_xy_20b;
    pos_xy_20b = (pos_x << 10 | pos_y);
    
    uint32_t pos_xy_20b_ant = (pos_xy_20b); //inicia com posicao anterior igual a posicao atual

    int direcao_sprite, i, j, movimento;
    uint16_t prox_pos_y, prox_pos_x;

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

        movimento = 5*velocidade;

        //descendo
        if ( direcao_sprite == 2 ){
            prox_pos_y = pos_y + movimento;
            converte_sprite_para_labirinto(pos_x, prox_pos_y, &i, &j);
            if( labirinto[i][j] != '#' ){ //sem parede, pode mover
                pos_y = prox_pos_y;
            }
        }
        //subindo
        else if ( direcao_sprite == 8 ){
            prox_pos_y = pos_y - movimento;
            converte_sprite_para_labirinto(pos_x, prox_pos_y, &i, &j);
            if (labirinto[i][j] != '#') { //sem parede, pode mover
                pos_y = prox_pos_y;
            }
        }
        //direita
        else if ( direcao_sprite == 6 ){
            prox_pos_x = pos_x + movimento;
            converte_sprite_para_labirinto(prox_pos_x, pos_y, &i, &j);
            //printf("Lab %c", labirinto[i][j]);
            if (labirinto[i][j] != '#') { //sem parede, pode mover
                pos_x = prox_pos_x; 
            } 
        }
        //esquerda
        else if ( direcao_sprite == 4 ){
            prox_pos_x = pos_x - movimento;
            converte_sprite_para_labirinto(prox_pos_x, pos_y, &i, &j);
            if (labirinto[i][j] != '#') { //sem parede, pode mover
                pos_x = prox_pos_x; 
            }
        }
        pos_xy_20b = (pos_x << 10 | pos_y);
        usleep(10000);
    }
}
*/
int main(){
    /*
    inicializa_fpga();
    configurar_acelerometro();
    */

    srand(time(NULL)); // Semente para números aleatórios

    inicializaLabirinto();

    geraLabirinto(ESPESSURA, ESPESSURA);

    // Coloca as saídas
    for (int i = 0; i <= ESPESSURA; i++) {
        labirinto[ESPESSURA][i] = ' ';
        labirinto[ESPESSURA + 1][i] = ' ';
        labirinto[ALTURA_LAB - ESPESSURA - 1][LARGURA_LAB - ESPESSURA - i] = ' '; 
        labirinto[ALTURA_LAB - ESPESSURA - 2][LARGURA_LAB - ESPESSURA - i] = ' '; 
    }

    // Posiciona o p1 e p2
    labirinto[ESPESSURA][0] = '1';
    labirinto[ALTURA_LAB - ESPESSURA - 2][LARGURA_LAB - ESPESSURA] = '2';

    for(int i = 0; i < ALTURA_LAB; i++) { 
        for(int j = 0; j < LARGURA_LAB; j++) {
            labirinto[i][LARGURA_LAB - 1] = 'K';
            labirinto[i][LARGURA_LAB - 2] = 'K';
            labirinto[i][LARGURA_LAB - 3] = 'K';
        }
    }

    imprimeLabirintoTerminal();

    uint16_t pos_x = 615;
    uint16_t pos_y = 439;

    pos_x &= mascara_10bits;
    pos_y &= mascara_10bits;
    uint32_t pos_xy_20b;
    pos_xy_20b = (pos_x << 10 | pos_y);

    for(int i = 0; i < ALTURA_LAB; i++) { 
        for(int j = 0; j < LARGURA_LAB; j++) {
            if(labirinto[i][j] == '1'){
                //printf("Sprite 1 - x lab: %d y lab: %d\n", i ,j);
                //exibe_sprite(1, pos_xy_20b, 1, 5);
            }
            else if(labirinto[i][j] == '2'){
                //printf("Sprite 2 - x lab: %d y lab: %d\n", i ,j);
                //exibe_sprite(1, pos_xy_20b, 2, 6);
            }
        }
    }
    /*
    for (int i = 0; i < 1500; i++) {
        imprimeLabirintoVGA();
        //apagaLabirinto();
    }
    */



    int x_lab, y_lab, colidiu;
    uint16_t prox_pos_y, prox_pos_x;
    printf("Iniciando: \n");
    converte_labirinto_para_sprite(4,0, &pos_x, &pos_y);

    printf("Posicao atual sprite: \n");
    converte_sprite_para_labirinto(pos_x,pos_y, &x_lab, &y_lab);
    printf("Labirinto: %c \n", labirinto[x_lab][y_lab]);

    while (1){
        printf("Posicao atual sprite: X %d Y %d - Labirinto: X %d Y %d \n", pos_x, pos_y, x_lab, y_lab);
        //direita
        prox_pos_x = pos_x + 15;
        colidiu = colide(prox_pos_x, pos_y);
        if( !colidiu ){ //sem parede, pode mover
            converte_sprite_para_labirinto(prox_pos_x,pos_y, &x_lab, &y_lab);
            printf("Sem parede, pode mover! Vai para labirinto: %c \n", labirinto[x_lab][y_lab]);
        } else {
            printf("Parede, nao pode mover! Labirinto: %c \n", labirinto[x_lab][y_lab]);
            break;
        }
        converte_labirinto_para_sprite(x_lab,y_lab, &pos_x, &pos_y);
    }

/*
//descendo
    prox_pos_y = pos_y + 15;
    colidiu = colide(pos_x, prox_pos_y);
    if( !colidiu ){ //sem parede, pode mover
        converte_sprite_para_labirinto(pos_x,prox_pos_y, &x_lab, &y_lab);
        printf("Sem parede, pode mover! Vai para labirinto %c", labirinto[x_lab][y_lab]);
    }
*/

    //colisao_labirinto();
    //colisao_labirinto1();

    /*
    desmapear_memoria();
    fecha_dev_mem();
    return 0;
    */
}

/*
Resolução VGA - sprites:    x 640 . 480 y
Resolução VGA - labirinto:  x  80 .  60 y

 -----------------------VGA-----------------------
| LABIRINTO:                SPRITE:               |
|   y y y y y ... y(79)      x x x x x ... x(639) | 
| x                        y                      |
| .                        .                      |
| .                        .                      |
| .                        .                      |
| x                        y                      |
| (59)                   (479)                    |
 -------------------------------------------------

X: 640/80 - 8
Y: 480/60 - 8
*/