#include <stdio.h>


void output_characters(char character, int num_characters)
{
    for (int i = 0; i < num_characters; i++)
        putchar(character);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            FILE *fh = fopen(argv[i], "rb");
            if (fh == NULL) {
                printf("wunzip: cannot open file\n");
                return 1;
            }
            char character;
            int num_characters;
            while (fread(&num_characters, sizeof(int), 1, fh) == 1) {
                fread(&character, sizeof(char), 1, fh);
                output_characters(character, num_characters);
            }
            fclose(fh);
        }
    }else{
        printf("wunzip: file1 [file2 ...]\n");
        return 1;
    }
    return 0;
}
