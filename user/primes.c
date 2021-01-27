#include "../kernel/types.h"
#include "user.h"

int main(int argc, char *argv[]) {
    if (argc != 1) {
        printf("This program accepts 0 arguments");
        exit(1);
    }
    int left = 0, right = 1, prime = 1, has_next = 0, isChild = 0, p[128], buf[1];
    buf[0] = 2;
    pipe(p);
    pipe(p + 2);
    close(p[1]);
    for (int i = 3; i < 35; ++i) {
        write(p[3], &i, sizeof(int));
        do {
            if (buf[0] % prime == 0 && prime != 1) continue;
            if (!has_next) {
                has_next = 1;
                if (fork() == 0) {
                    close(p[left * 2]);
                    close(p[right * 2 + 1]);
                    left = right++;
                    pipe(p + right * 2);
                    isChild = 1;
                    has_next = 0;
                    prime = buf[0];
                    printf("prime %d\n", prime);
                    continue;
                } else {
                    close(p[right * 2]);
                }
            }
            if (isChild) write(p[right * 2 + 1], buf, sizeof(int));
        } while (isChild && read(p[left * 2], buf, sizeof(int)));
        
        if (isChild) {
            close(p[right * 2 + 1]);
            wait(0);
            exit(0);
        }
    }
    close(p[3]);
    wait(0);
    exit(0);
}
