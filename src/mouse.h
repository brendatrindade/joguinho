#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>

/*
Abre o driver do mouse em modo leitura.
Retorno: O caminho do driver do mouse. Caso ocorra algum erro, retorna -1.
*/

int abre_mouse();

int le_mouse_orientacao(int fd);

int le_mouse_direcao(int fd, uint16_t *x, uint16_t *y);

void le_mouse(int fd, int *x_ou_y, int *direcao);

#endif /* MOUSE_H */