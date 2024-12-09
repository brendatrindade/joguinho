#include "proc_grafico.h"
#include "mouse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void inicializa_fpga();
extern void fecha_dev_mem();
extern void exibe_sprite(uint8_t sp, uint16_t x, uint16_t offset, uint8_t registrador);
extern void altera_pixel_sprite(uint16_t cor, uint16_t endereco);


#define largura_sprite 20
#define altura_sprite 20


// Converte os dados da imagem de RGB para BGR 9 bits - formato da instrucao wsm
uint16_t converte_em_bgr(uint8_t rgb) {
    uint8_t r, g, b;
    uint16_t bgr;

    //Extrai cada tom (R, G e B) do formato RGB
    r = (rgb >> 5) & 0b111; // 3 bits msb do vermelho
    g = (rgb >> 2) & 0b111; // 3 bits do meio do verde
    b = rgb & 0b11; // 2 bits lsb do azul
    //ajusta o azul de 2 para 3 bits
    b = b << 1;

    //Coloca no formato BBB GGG RRR de 9 bits
    bgr = ( (b << 6) | (g << 3) | r );

    return bgr;
}

// Cria e armazena um sprite na memoria de sprites
void cria_sprite(uint16_t end_base, uint16_t dados_do_sprite[altura_sprite][largura_sprite]) {
    uint16_t cor[400]; //20x20 -> 400 pixels por sprite
    int y, x;
    int z = 0;
    for ( y = 0; y < altura_sprite; y++) {
        for ( x = 0; x < largura_sprite; x++) {
            cor[z] = dados_do_sprite[y][x]; //Extrai a cor de cada pixel em 9 bits BGR
            z++;
        }
    }
    //Escreve cada pixel da matriz 20x20 na memoria de sprites
    int i = 0;
    while (i < 400){
        altera_pixel_sprite(cor[i], end_base + i);
        i++;
        usleep(10000);
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
        int direcao = le_mouse_direcao(fd);
        int orientacao = le_mouse_orientacao(fd);
        //x, y = teste_leitura(fd);

        // apaga o sprite exibido na posicao anterior
        exibe_sprite(0, pos_xy_20b_ant, 5, 1); // sp = 0 -> desabilita sprite

        if(direcao == 0) { //horizontal
            // if(orientacao > 0){ //direita
            //     pos_x += 10;
            //     pos_x &= mascara_10bits;
            //     pos_xy_20b = (pos_x << 10 | pos_y);
            // } else if (orientacao < 0) { //esquerda
            //     pos_x -= 10;
            //     pos_x &= mascara_10bits;
            //     pos_xy_20b = (pos_x << 10 | pos_y);
            // }

        } else if (direcao == 1 ) { //vertical
            if(orientacao > 0 && pos_xy_20b < LIMITE_Y){ //baixo
                pos_xy_20b += 10;
            } else if (orientacao < 0 && pos_xy_20b > INICIO_Y) { //cima
                pos_xy_20b -= 10;
            }
        }

         // exibe o sprite na posicao atual
        exibe_sprite(1, pos_xy_20b, 5, 1); // sp = 1 -> habilita sprite
        pos_xy_20b_ant = pos_xy_20b;

        usleep(10000);
    }
}

int main() {    
    inicializa_fpga();
    move_sprite();
    fecha_dev_mem();

    return 0;
}
