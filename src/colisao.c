#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

int arredonda_div(float numero_float);
void converte_sprite_para_labirinto(uint16_t pos_x, uint16_t pos_y, int *x_lab, int *y_lab);
void converte_labirinto_para_sprite(int x_lab, int y_lab, uint16_t *pos_x, uint16_t *pos_y);

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

void converte_sprite_para_labirinto(uint16_t pos_x, uint16_t pos_y, int *x_lab, int *y_lab){
    // sprite pos_x < 639 <-> y_lab < 79 labirinto
    // sprite pos_y < 479 <-> x_lab < 59 labirinto

    float float_x = (pos_x / 8); 
    float float_y = (pos_y / 8);

    *y_lab = arredonda_div(float_x);
    *x_lab = arredonda_div(float_y);
}

void converte_labirinto_para_sprite(int x_lab, int y_lab, uint16_t *pos_x, uint16_t *pos_y){
    // sprite pos_x < 639 <-> y_lab < 79 labirinto
    // sprite pos_y < 479 <-> x_lab < 59 labirinto

    if (x_lab == 0){
        *pos_y = x_lab * 8; 
    } else {
        int x = (x_lab + 1) * 8; 
        *pos_y = x;
        *pos_y -= 1;
    }
    if (y_lab == 0){
        *pos_x = y_lab * 8;
    } else {
        int y = (y_lab + 1) * 8;
        *pos_x = y;
        *pos_x -= 1;
    }
}