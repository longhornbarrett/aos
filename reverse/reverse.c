#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
    FILE* fin = stdin;
    FILE* fout = stdout;
    char newline = '\n';
    if (argc > 3)
    {
        printf("usage: reverse <input> <output>\n");
        return 1;
    }
    if(argc >= 3){
        int compare_args = strcmp(argv[1], argv[2]);
        if (compare_args == 0)
        {
            printf("Input and output file must differ\n");
            return 1;
        }
        fout = fopen(argv[2], "w");
        if (fout == NULL) {
            printf("reverse: cannot open file '%s'\n", argv[1]);
            return 1;
        }
    }
    if (argc >= 2) {
        fin = fopen(argv[1], "r");
        if (fin == NULL) {
            printf("reverse: cannot open file '%s'\n", argv[1]);
            return 1;
        }
    }
    int num_current_lines = 0;
    char* buf[20];
    //char *line = NULL;
    size_t len = 0;
    while (getline(&buf[num_current_lines], &len, fin) != -1) {
        //buf[num_current_lines] = line;
        num_current_lines += 1;
    }

    for(int i = num_current_lines-1; i >= 0; i--) {
        fputs(buf[i], fout);
        if (buf[i][strlen(buf[i]) - 1] != newline)
            putc(newline, fout);
    }
    fclose(fin);
    fclose(fout);
}