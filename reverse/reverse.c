#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 2048
#define MEM_MAX BLOCK_SIZE*1024
#define NEWLINE '\n'
#define TEMP_FILE_FORMAT "/tmp/rev_%d.txt"

int reverse_block(FILE* fin, char* buf[], int* num_current_lines)
{
    long current_memory_size = 0;
    size_t len = 0;
    buf[*num_current_lines] = NULL;
    while (current_memory_size < MEM_MAX && getline(&buf[*num_current_lines], &len, fin) != -1) {
        current_memory_size += strlen(buf[*num_current_lines]);
        *num_current_lines += 1;
        buf[*num_current_lines] = NULL;
    }
    free(buf[*num_current_lines]);
    return current_memory_size < MEM_MAX;
}

void write_data_to_stream(FILE* fout, char* buf[], int num_current_lines)
{
    for (int i = num_current_lines - 1; i >= 0; i--) {
        fputs(buf[i], fout);
        if (buf[i][strlen(buf[i]) - 1] != NEWLINE)
            putc(NEWLINE, fout);
    }
}

void cat_temp_file(FILE* fout, int temp_file_seq)
{
    char str[BLOCK_SIZE];
    char file_name[150];
    sprintf(file_name, TEMP_FILE_FORMAT, temp_file_seq);
    FILE* fin = fopen(file_name, "r");
    while (fgets(str, BLOCK_SIZE, fin) != NULL)
        fputs(str, fout);
    remove(file_name);
}

void write_temp_file(char* buf[], int num_current_lines, int num_temp_files)
{
    char file_name[150];
    sprintf(file_name, TEMP_FILE_FORMAT, num_temp_files);
    FILE* fout = fopen(file_name, "w");
    if (fout == NULL) {
        fprintf(stderr,"reverse: cannot open file '%s'\n", file_name);
        exit(1);
    }
    write_data_to_stream(fout, buf, num_current_lines);
    fclose(fout);
}

void free_buffer(char* buf[], int num_current_lines)
{
    for (int i = 0; i < num_current_lines; i++)
        free(buf[i]);
    free(buf);
}

char* getFileNameFromPath(char* path)
{
    for(size_t i = strlen(path) - 1; i; i--)
    {
        if (path[i] == '/')
        {
            return &path[i+1];
        }
    }
    return path;
}

int main(int argc, char *argv[]) {
    FILE* fin = stdin;
    FILE* fout = stdout;
    if (argc > 3)
    {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }
    if(argc >= 3){
        char* file_1 = getFileNameFromPath(argv[1]);
        char* file_2 = getFileNameFromPath(argv[2]);
        if (strcmp(file_1, file_2) == 0)
        {
            fprintf(stderr, "reverse: input and output file must differ\n");
            exit(1);
        }
        fout = fopen(argv[2], "w");
        if (fout == NULL) {
            fprintf(stderr,"reverse: cannot open file '%s'\n", argv[1]);
            exit(1);
        }
    }
    if (argc >= 2) {
        fin = fopen(argv[1], "r");
        if (fin == NULL) {
            fprintf(stderr,"reverse: cannot open file '%s'\n", argv[1]);
            exit(1);
        }
    }
    int num_current_lines = 0;
    char** buf = malloc(BLOCK_SIZE * sizeof(char*));
    int done = 0;
    int num_temp_files = 0;
    do {
        done = reverse_block(fin, buf, &num_current_lines);
        if (!done)
        {
            write_temp_file(buf, num_current_lines, num_temp_files);
            free_buffer(buf, num_current_lines);
            num_temp_files += 1;
            num_current_lines = 0;
        }
    }while(!done);
    write_data_to_stream(fout, buf, num_current_lines);
    if (num_temp_files > 0){
        for(int i = num_temp_files - 1; i >= 0; i--)
            cat_temp_file(fout, i);
    }

    free_buffer(buf, num_current_lines);
    fclose(fin);
    fclose(fout);
}