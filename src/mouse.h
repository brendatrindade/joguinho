#ifndef MOUSE_H
#define MOUSE_H

/*
Abre o driver do mouse em modo leitura.
Retorno: O caminho do driver do mouse. Caso ocorra algum erro, retorna -1.
*/
int abre_mouse();

void le_mouse(int fd);

void le_mouse1(int fd);

/*
Primeiro teste feito no LEDS.
*/
int teste();

#endif /* MOUSE_H */