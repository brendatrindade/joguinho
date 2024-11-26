#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>


int abre_mouse();
void le_mouse(int fd);
int teste();


int abre_mouse() {
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Erro");
        return -1;
    }
    return fd;
}

void le_mouse(int fd) {
    struct input_event ie;
    int x = 0, y = 0;
    
    while (1) {
        if (read(fd, &ie, sizeof(ie)) != sizeof(ie)) {
            perror("Erro ao ler o evento");
            break;
        }

        if (ie.type == EV_REL) {
            if (ie.code == REL_X) {
                x = ie.value;
                printf("Movimento horizontal: %d\n", x);
            } else if (ie.code == REL_Y) {
                y = ie.value;
                printf("Movimento vertical: %d\n", y);
            }
        }
    }
}

void le_mouse1(int fd) {
    struct input_event ie;
    int x = 0, y = 0;
    
    while (1) {
        if (read(fd, &ie, sizeof(ie)) != sizeof(ie)) {
            perror("Erro ao ler o evento");
            break;
        }

        if (ie.type == EV_REL) {
            if (ie.code == REL_X) {
                x = ie.value;
                //printf("Movimento horizontal: %d\n", x);
            } else if (ie.code == REL_Y) {
                y = ie.value;
                //printf("Movimento vertical: %d\n", y);
            }
        }
    }
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

int main() {
    int fd = abre_mouse();
    if (fd != -1) {
        le_mouse(fd);
        close(fd);
    }
    
    return 0;
}

/*
struct input_event {
struct timeval time;
__u16 type;
__u16 code;
__s32 value;
};
*/