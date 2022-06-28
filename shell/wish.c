#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define COMMAND_BUFSIZE 204
#define COMMAND_LINE_ARGUMENTS 128
#define CONCURRENT_CMDS 128

int path_len = 2;
const char* command_delims = " \t\r\n\f\v"; 

void print_error(void)
{
    char error_message[30] = "An error has occurred\n";
    fprintf(stderr, error_message, strlen(error_message));
}

void wish_cd(char** cmd){
    if (cmd[1] == NULL) {
        print_error();
        exit(EXIT_SUCCESS);
    } else if(cmd[2] != NULL){
        print_error();
        exit(EXIT_SUCCESS);
    } else{
        if (chdir(cmd[1]) != 0) {
            print_error();
            exit(EXIT_SUCCESS);
        }
    }
}

void free_buffer(char* buf[], int buf_len)
{
    for (int i = 0; i < buf_len; i++)
        free(buf[i]);
    free(buf);
}

bool check_for_actual_command(char *buffer)
{
    bool command_found = false;
    for (int i = 0; i < strlen(buffer); i++)
    {
        if (!isspace(buffer[i]))
        {
            command_found = true;
            break;
        }
    }
    return command_found;
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
    bool found = false;
    char* full_path;
    for(int j = 0; j < path_len && !found; j++)
    {
        full_path = (char*)malloc(strlen(paths[j]) + strlen(cmd[0]) + 1);
        strcpy(full_path, paths[j]);
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
    if (execv(full_path, cmd) == -1) {
        print_error();
        free(full_path);
        exit(EXIT_SUCCESS);
    }
    free(full_path);
}

void launch_commands(char*** cmds, char** paths, int num_cmds)
{
    pid_t *pids = malloc(num_cmds * sizeof(pid_t));
    for(int i = 0;i < num_cmds; i++){
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            launch_command((char**)cmds[i], paths);
            exit(0);
        } else if (pid < 0) {
            // Error forking
            print_error();
            exit(EXIT_SUCCESS);
        } else {
            pids[i] = pid;
        }
    }
    for (int i  = 0 ; i < num_cmds ; i++) {
            int status;
            waitpid(pids[i], &status, 0);
    }
    free(pids);
}

char** parse_command(char* command, char** paths){
    char **cmd = malloc(COMMAND_LINE_ARGUMENTS * sizeof(char*));
    char ***concurrent_cmds = malloc(CONCURRENT_CMDS * sizeof(char**));
    char *argument;
    int argument_pos = 1;
    int num_cmds = 1;
    cmd[0] = strtok_r(command, command_delims, &command);
    concurrent_cmds[num_cmds-1] = (char**)&cmd[0];
    bool redirect = false;
    char *redirect_fh = NULL;
    while((argument = strtok_r(command, command_delims, &command)) != NULL)
    {
        if(strcmp(argument, "&") == 0)
        {
            //Handle parallel commands
            cmd[argument_pos] = NULL;
            argument_pos++;
            concurrent_cmds[num_cmds] = (char**)&cmd[argument_pos];
            num_cmds++;
        }else if(strcmp(argument, ">") == 0 )
        {
            //check if redirect already found if so this is an error
            if(redirect)
            {
                print_error();
                exit(EXIT_SUCCESS);
            }
            redirect = true;
        }else if(redirect){
            if(redirect_fh != NULL)
            {
                print_error();
                exit(EXIT_SUCCESS);
            }
            redirect_fh = argument;
        }else{
            cmd[argument_pos] = argument;
            argument_pos++;
        }
    }
    cmd[argument_pos] = NULL;
    if(redirect)
    {
        if(redirect_fh == NULL)
        {
            print_error();
            exit(EXIT_SUCCESS);
        }
        int orig_stdout = dup(1);
        int orig_stderr = dup(2);
        int out = open(redirect_fh, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(out, STDOUT_FILENO);
        dup2(out, STDERR_FILENO);
        launch_commands(concurrent_cmds, paths, num_cmds);
        fflush(stdout);
        fflush(stderr);
        close(out);
        dup2(orig_stdout, STDOUT_FILENO);
        dup2(orig_stderr, STDERR_FILENO);
    }else if (strcmp(cmd[0], "exit") == 0)
    {
        // check if exit called with argument
        if(cmd[1] != NULL)
            print_error();
        exit(EXIT_SUCCESS);
    }else if(strcmp(cmd[0], "cd") == 0)
    {
        wish_cd(cmd);
    }else if(strcmp(cmd[0], "path") == 0){
        free_buffer(paths, path_len);
        paths = wish_path(&cmd[1], argument_pos - 1);
    }else{
        launch_commands(concurrent_cmds, paths, num_cmds);
    }
    free(cmd);
    free(concurrent_cmds);
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
                exit(EXIT_SUCCESS);
            }
            break;
        }
        line[strcspn(line, "\n")] = 0;
        if(check_for_actual_command(line))
            paths = parse_command(line, paths);
    }while(exit_status == 1);
    free(line);
    free_buffer(paths, path_len);
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
