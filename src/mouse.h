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

int teste_leitura(int fd);

/*
Primeiro teste feito no LEDS.
*/
int teste();

#endif /* MOUSE_H */