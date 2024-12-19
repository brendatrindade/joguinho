#ifndef MOUSE_THREAD_H
#define MOUSE_THREAD_H
#include <stdint.h>

/*
Abre o driver do mouse em modo leitura.
Retorno: O caminho do driver do mouse. Caso ocorra algum erro, retorna -1.
*/

int abre_mouse();

int le_mouse_valor(int *valor_x, int *valor_y);

int define_velocidade_mouse(float valor);

int get_movimento_mouse(int fd, int *velocidade);

#endif /* MOUSE_THREAD_H */