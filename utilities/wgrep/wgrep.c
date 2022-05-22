#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void grep(FILE* fh, char* g_str){
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fh) != -1)
        if (strstr(line, g_str))
            fputs(line, stdout);
    free(line);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (argc == 2){
            grep(stdin, argv[1]);
        }else {
            for (int i = 2; i < argc; i++) {
                FILE* fh = fopen(argv[i], "r");
                if (fh == NULL) {
                    printf("wgrep: cannot open file\n");
                    return 1;
                }
                grep(fh, argv[1]);
                fclose(fh);
            }
        }
    }else{
        printf("wgrep: searchterm [file ...]\n");
        return 1;
    }
    return 0;
}
