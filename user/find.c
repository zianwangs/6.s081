#include "../kernel/types.h"
#include "user.h"
#include "../kernel/stat.h"
#include "../kernel/fs.h"

int find(char *, char *);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: search <dir> to find all the files with <name>");
        exit(1);
    }
    if(find(argv[1], argv[2])) 
        exit(1);
    exit(0);
}

int find(char * path, char * file) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return 1;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return 1;
    }

    if (st.type != T_DIR) {
        fprintf(2, "%s is not a directory\n", path);
        close(fd);
        return 1;
    }

    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0) continue;
        memmove(p, de.name, DIRSIZ);
        p[strlen(de.name)] = '\0';
        if(stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }
        if (st.type == T_FILE) {
            if (!strcmp(de.name, file))
                printf("%s\n", buf);
        } else if (st.type == T_DIR && strcmp(p,".") && strcmp(p, "..")) {
            find(buf, file);
        }
    }  
    close(fd);
    return 0;
}
