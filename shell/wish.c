#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define COMMAND_BUFSIZE 204
#define COMMAND_LINE_ARGUMENTS 128

int path_len = 2;

void print_error(void)
{
    char error_message[30] = "An error has occurred\n";
    fprintf(stderr, error_message, strlen(error_message));
}

void wish_cd(char** cmd){
    if (cmd[1] == NULL) {
        print_error();
    } else if(cmd[2] != NULL){
        print_error();
    } else{
        if (chdir(cmd[1]) != 0) {
            print_error();
        }
    }
}

void free_buffer(char* buf[], int buf_len)
{
    for (int i = 0; i < buf_len; i++)
        free(buf[i]);
    free(buf);
}

char** wish_path(char** new_paths, int arg_len){
    char** paths = malloc(arg_len * sizeof(char*));
    for(int i = 0; i < arg_len; i++)
    {   
        int s_len = strlen(new_paths[i]);
        if(new_paths[i][s_len - 1] != '/')
            s_len++;
        paths[i] = malloc((s_len+1) * sizeof(char));
        strcpy(paths[i], new_paths[i]);
        if(paths[i][s_len - 1] != '/')
        {
          paths[i][s_len - 1] = '/';
          paths[i][s_len] = '\0';
        }
    }
    path_len = arg_len;
    return paths;
}

void launch_command(char** cmd, char** paths)
{
    pid_t pid;
    int status;
    bool found = false;
    char* full_path;
    
    for(int i = 0; i < path_len && !found; i++)
    {
        full_path = (char*)malloc(strlen(paths[i]) + strlen(cmd[0]) + 1);
        strcpy(full_path, paths[i]);
        strcat(full_path, cmd[0]);
        if(access(full_path, X_OK) == 0)
        {
            found = true;
        }else{
            free(full_path);
        }
    }
    if(!found){
        print_error();
        exit(EXIT_SUCCESS);
    }
    pid = fork();
    if (pid == 0) {
        // Child process
        if (execv(full_path, cmd) == -1) {
            perror("wish");
        }
        free(full_path);
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        print_error();
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    free(full_path);
}

char** parse_command(char* command, char** paths){
    char **cmd = malloc(COMMAND_LINE_ARGUMENTS * sizeof(char*));
    char *argument;
    int argument_pos = 1;
    cmd[0] = strsep(&command, " ");
    while((argument = strsep(&command, " ")) != NULL)
    {
        cmd[argument_pos] = argument;
        argument_pos++;
    }
    cmd[argument_pos] = NULL;
    if (strcmp(cmd[0], "exit") == 0)
    {
        exit(EXIT_SUCCESS);
    }else if(strcmp(cmd[0], "cd") == 0)
    {
        wish_cd(cmd);
    }else if(strcmp(cmd[0], "path") == 0){
        free_buffer(paths, path_len);
        paths = wish_path(&cmd[1], argument_pos - 1);
    }else{
        launch_command(cmd, paths);
    }
    free(cmd);
    return paths;
}

void wish_loop(FILE* fp, bool batch_mode){
    char *line = NULL;
    size_t len = 0;
    int exit_status = 1;
    char** paths = malloc(2 * sizeof(char*));
    paths[0] = malloc(sizeof("/bin/") * sizeof(char));
    strcpy(paths[0], "/bin/");
    paths[1] = malloc(sizeof("/usr/bin/") * sizeof(char));
    strcpy(paths[1], "/usr/bin/");
    do{
        if (!batch_mode)
            printf("wish>");
        int lr = getline(&line, &len, fp);
        if (lr == -1){
            if (!feof(fp)){
            print_error();
            }
            break;
        }
        line[strcspn(line, "\n")] = 0;
        paths = parse_command(line, paths);
    }while(exit_status == 1);
    free(line);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        FILE* fp = fopen(argv[1], "r");
        if (fp == NULL) {
            print_error();
        }else{
            wish_loop(fp, true);
            fclose(fp);
        }
    }else{
        wish_loop(stdin, false);
    }
    return EXIT_SUCCESS;
}
