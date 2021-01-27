#include "../kernel/types.h"
#include "user.h"
#include "../kernel/param.h"

int main(int argc, char * argv[]) {
    if (argc <= 1) {
        fprintf(2, "Usage: xargs <program> <Args...>\n");
        exit(1);
    }
    char buf[128], c[1], *new_argv[MAXARG];
    int idx = 0, cur = 0;
    while (read(0, c, 1) > 0) {
        if (*c == '\n') {
            idx = 0;
            buf[cur] = 0;
            for (int i = 1; i < argc; ++i) {
                new_argv[idx++] = argv[i];
            }
            new_argv[idx++] = buf;
            new_argv[idx] = 0;
            cur = 0;
            if (fork() == 0) {
                exec(new_argv[0], new_argv);
                exit(1);
            } else {
                wait(0);
            }
        } else buf[cur++] = *c;
    }
    exit(0);
}
