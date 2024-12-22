#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdint.h>

int abre_mouse();
int le_mouse_valor(int fd, int *valor_x, int *valor_y);
int define_velocidade_mouse(float valor);
int get_movimento_mouse(int fd, int *velocidade);
int teste_leitura(int fd);
int teste();

#define FILTRO_MOVIMENTO_MOUSE 3

//obtem o descritor de arquivo
int abre_mouse() {
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Erro");
        return -1;
    }
    return fd;
}

//Le o valor e define a direcao do movimento 
int le_mouse_valor(int fd, int *valor_x, int *valor_y) {
    struct input_event ie; // struct que recebe as informações do mouse
    int x, y;
    while (read(fd, &ie, sizeof(struct input_event)) > 0) {
        if (ie.type == EV_REL) {
            if (ie.code == REL_X) { 
                x = ie.value;
                *valor_x = x;
                if (x > 0){
                    return 6; //direita
                } else if (x < 0){
                    return 4; //esquerda
                }
            } else if (ie.code == REL_Y) {
                y = ie.value;
                *valor_y = y;
                if (y > 0){
                    return 2; //baixo
                } else if (y < 0){
                    return 8; //cima
                }
            }
        }
    }
    close(fd);
    return 0;
}

// Define a velocidade com base na intensidade do movimento
int define_velocidade_mouse(float valor) {
   if (valor < ( FILTRO_MOVIMENTO_MOUSE * 4)) {
      //printf("velocidade 1");
      return 1; //velocidade baixa
   } else if (valor < ( FILTRO_MOVIMENTO_MOUSE * 8)) {
      //printf("velocidade 2");
      return 2; //velocidade media
   } else {
      //printf("velocidade 3");
      return 3; //velocidade alta
   }
}

// Obtem velocidade e direcao do movimento 8 cima, 2 baixo, 6 direita, 4 esquerda, 0 sem movimento
int get_movimento_mouse(int fd, int *velocidade) {
    int direcao, valor_x, valor_y;
    direcao = le_mouse_valor(fd, &valor_x, &valor_y);

    if ( (valor_x > FILTRO_MOVIMENTO_MOUSE) && (direcao == 6)) { //movimento no eixo x+
        //printf("movimento x+\n");
        *velocidade = define_velocidade_mouse(valor_x);
        return 6; // x - direcao para direita
    } else if ( (valor_y > FILTRO_MOVIMENTO_MOUSE) && (direcao == 2)) { //movimento no eixo y- 
        //printf("movimento y-\n");
        *velocidade = define_velocidade_mouse(valor_y);
        return 2; //y - direcao para baixo 
    } else if ( (valor_x < -FILTRO_MOVIMENTO_MOUSE) && (direcao == 4)) { //movimento no eixo x-
        //printf("movimento x-\n");
        *velocidade = define_velocidade_mouse(-valor_x);
        return 4; // x - direcao para esquerda

    } else if ( (valor_y < -FILTRO_MOVIMENTO_MOUSE) && (direcao == 8)) { //movimento no eixo y+
        //printf("movimento y+\n");
        *velocidade = define_velocidade_mouse(-valor_y);
        return 8; // y - direcao para cima
    }
    //printf("sem movimento\n");
    return 0; //sem movimento   
}

int teste_leitura(int fd){
        if (fd == -1) {
        perror("Erro");
        return -1;
    }
    return fd;
}

int teste() {
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Erro");
        return 1;
    }

    struct input_event ie; // struct que recebe as informações do mouse
    while (read(fd, &ie, sizeof(struct input_event)) > 0) {
        printf("Type: %d, Code: %d, Value: %d\n", ie.type, ie.code, ie.value);
    }

    close(fd);
    return 0;
}