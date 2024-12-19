#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdint.h>

int abre_mouse();
int le_mouse_orientacao(int fd);
int le_mouse_direcao(int fd, uint16_t *x, uint16_t *y);
void le_mouse(int fd, int *x_ou_y, int *direcao);


int abre_mouse() {
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Erro");
        return -1;
    }
    return fd;
}

/*
0 = X
1 = Y
*/
int le_mouse_orientacao(int fd) {
    struct input_event ie;
    
    if (read(fd, &ie, sizeof(ie)) != sizeof(ie)) { 
        perror("Erro ao ler o evento"); 
    }

    if (ie.code == REL_X) { return 0; } 
    else if (ie.code == REL_Y) { return 1; }
}

/*
0 = NEGATIVO
1 = POSITIVO
*/
int le_mouse_direcao(int fd, uint16_t *x, uint16_t *y) {
    struct input_event ie;
    
    while (1) {
        if (read(fd, &ie, sizeof(ie)) != sizeof(ie)) {
            perror("Erro ao ler o evento");
            break;
        }

        if (ie.value < 0)  { return 0; } 
        else { return 1; }        
    }
}
/*
0 = X
1 = Y

0 = NEGATIVO
1 = POSITIVO
*/
void le_mouse(int fd, int *x_ou_y, int *direcao) {
    struct input_event ie;
    
    if (read(fd, &ie, sizeof(ie)) != sizeof(ie)) { 
        perror("Erro ao ler o evento"); 
    }

    if (ie.code == REL_X) { *x_ou_y = 0; } 
    else if (ie.code == REL_Y) { *x_ou_y = 1; }

    if (ie.value < 0)  { *direcao = 0; } 
    else { *direcao = 1; }  
}

/*
        } else if (ie.code == REL_Y) {
            if (ie.value > 0)  {
                //*y+=10;
            } else {
                //*y-=10;
            }
            //*x=*x;
*/