#include <stdio.h>

int main(int argc, char *argv[]) {
    int BUF_SIZE = 1024;
    char str[BUF_SIZE];
    FILE *fh;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            fh = fopen(argv[i], "r");
            if (fh == NULL) {
                printf("wcat: cannot open file\n");
                return 1;
            }
            while (fgets(str, BUF_SIZE, fh) != NULL)
                fputs(str, stdout);
            fclose(fh);
        }
    }
    return 0;
}