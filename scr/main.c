
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

int main() {
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Erro");
        return 1;
    }

    struct input_event ie;
    while (read(fd, &ie, sizeof(struct input_event)) > 0) {
        printf("Type: %d, Code: %d, Value: %d\n", ie.type, ie.code, ie.value);
    }

    close(fd);
    return 0;
}
