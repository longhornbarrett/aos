#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    int current_cnt = 0;
    char current_char;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            FILE *fh = fopen(argv[i], "r");
            if (fh == NULL) {
                printf("wzip: cannot open file\n");
                return 1;
            }
            while ((nread = getline(&line, &len, fh)) != -1) {
                for (int i = 0; i < nread; i++) {
                    if (line[i] == current_char)
                        current_cnt += 1;
                    else {
                        if (current_cnt > 0) {
                            fwrite(&current_cnt, sizeof(current_cnt), 1, stdout);
                            fwrite(&current_char, sizeof(current_char), 1, stdout);
                        }
                        current_char = line[i];
                        current_cnt = 1;
                    }
                }
            }
            fclose(fh);
        }
        fwrite(&current_cnt, sizeof(current_cnt), 1, stdout);
        fwrite(&current_char, sizeof(current_char), 1, stdout);
        free(line);
    }else{
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }
    return 0;
}
