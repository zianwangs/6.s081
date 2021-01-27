#include "../kernel/types.h"
#include "user.h"

int main(int argc, char *argv[]) {
    if (argc != 1) {
        printf("This program accepts 0 arguments\n");
        exit(1);
    }
    int p[2];
    pipe(p);
    if (fork() == 0) {
        char buf[2];
        read(p[0], buf, 1);
        printf("%d: received ping\n", getpid());
        write(p[1], buf, 1);
        exit(0);
    } else {
        char buf[2];
        int status;
        write(p[1], "a", 1);
        wait(&status);
        read(p[0], buf, 1);
        printf("%d: received pong\n", getpid());
        exit(0);
    }

}
