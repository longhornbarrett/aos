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
#define NO_EXIT 1

int path_len = 2;
const char* command_delims = " \t\r\n\f\v"; 
char concurrent_char = '&';
char redirect_char = '>';

//This is my struct that returns a path
struct PathRet{
    char** path;
    int ret_code;
};

void print_error(void)
{
    char error_message[30] = "An error has occurred\n";
    fprintf(stderr, error_message, strlen(error_message));
}

// method that changes directory
int wish_cd(char** cmd){
    if (cmd[1] == NULL) {
        print_error();
        return EXIT_SUCCESS;
    } else if(cmd[2] != NULL){
        print_error();
        return EXIT_SUCCESS;
    } else{
        if (chdir(cmd[1]) != 0) {
            print_error();
            return EXIT_SUCCESS;
        }
    }
    return NO_EXIT;
}

//This is a method that frees a memory buffer
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

int launch_command(char** cmd, char** paths)
{
    if(!check_for_actual_command(cmd[0]))
        return NO_EXIT;
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
        return EXIT_SUCCESS;
    }

    bool redirect = false;
    char* redirect_fh = NULL;
    for(int i = 0; cmd[i] != NULL; i++)
    {
        if(strcmp(cmd[i], ">") == 0)
        {
            if(redirect || cmd[i+1] == NULL || cmd[i+2] != NULL)
            {
                print_error();
                free(full_path);
                return EXIT_SUCCESS;
            }
            redirect = true;
            redirect_fh = cmd[i+1];
            cmd[i] = NULL;
        }
    }
    if(redirect)
    {
        int orig_stdout = dup(1);
        int orig_stderr = dup(2);
        int out = open(redirect_fh, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(out, STDOUT_FILENO);
        dup2(out, STDERR_FILENO);
        int ret_code = execv(full_path, cmd);
        fflush(stdout);
        fflush(stderr);
        close(out);
        dup2(orig_stdout, STDOUT_FILENO);
        dup2(orig_stderr, STDERR_FILENO);
        if(ret_code == -1){
            print_error();
            free(full_path);
            return EXIT_SUCCESS;
        }
    }else if (execv(full_path, cmd) == -1) {
        print_error();
        free(full_path);
        return EXIT_SUCCESS;
    }
    free(full_path);
    return NO_EXIT;
}

int launch_commands(char*** cmds, char** paths, int num_cmds)
{
    pid_t *pids = malloc(num_cmds * sizeof(pid_t));
    for(int i = 0;i < num_cmds; i++){
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            return launch_command((char**)cmds[i], paths);
        } else if (pid < 0) {
            // Error forking
            print_error();
            return EXIT_SUCCESS;
        } else {
            pids[i] = pid;
        }
    }
    for (int i  = 0 ; i < num_cmds ; i++) {
            int status;
            waitpid(pids[i], &status, 0);
    }
    free(pids);
    return NO_EXIT;
}

struct PathRet parse_command(char* command, char** paths){
    char **cmd = malloc(COMMAND_LINE_ARGUMENTS * sizeof(char*));
    char ***concurrent_cmds = malloc(CONCURRENT_CMDS * sizeof(char**));
    char *argument;
    int argument_pos = 0;
    int num_cmds = 1;
    bool a_command = false;
    struct PathRet ret_struct;
    ret_struct.ret_code = NO_EXIT;
    ret_struct.path = paths;

    concurrent_cmds[num_cmds-1] = (char**)&cmd[0];

    while((argument = strtok_r(command, command_delims, &command)) != NULL && ret_struct.ret_code == NO_EXIT)
    {
        if(strcmp(argument, "&") == 0)
        {
            if(a_command){
                //Handle parallel commands
                cmd[argument_pos] = NULL;
                argument_pos++;
                concurrent_cmds[num_cmds] = (char**)&cmd[argument_pos];
                num_cmds++;
                a_command = false;
            }
        }else{
            //at_least_one_command = true;
            cmd[argument_pos] = argument;
            argument_pos++;
            a_command = true;
        }
    }
    cmd[argument_pos] = NULL;
    if(ret_struct.ret_code == NO_EXIT && cmd[0] != NULL){
        if (strcmp(cmd[0], "exit") == 0)
        {
            // check if exit called with argument
            if(cmd[1] != NULL)
            {
                print_error();               
            }
            ret_struct.ret_code = EXIT_SUCCESS;
        }else if(strcmp(cmd[0], "cd") == 0)
        {
            ret_struct.ret_code = wish_cd(cmd);
        }else if(strcmp(cmd[0], "path") == 0){
            free_buffer(paths, path_len);
            paths = wish_path(&cmd[1], argument_pos - 1);
            ret_struct.path = paths;
        }else{
            ret_struct.ret_code = launch_commands(concurrent_cmds, paths, num_cmds);
        }
    }
    free(cmd);
    free(concurrent_cmds);
    return ret_struct;
}

char * replace(
    char const * const original, 
    char const * const pattern, 
    char const * const replacement
) {
  size_t const replen = strlen(replacement);
  size_t const patlen = strlen(pattern);
  size_t const orilen = strlen(original);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
  {
    patcnt++;
  }

  {
    // allocate memory for the new string
    size_t const retlen = orilen + patcnt * (replen - patlen);
    char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

    if (returned != NULL)
    {
      // copy the original string, 
      // replacing all the instances of the pattern
      char * retptr = returned;
      for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
      {
        size_t const skplen = patloc - oriptr;
        // copy the section until the occurence of the pattern
        strncpy(retptr, oriptr, skplen);
        retptr += skplen;
        // copy the replacement 
        strncpy(retptr, replacement, replen);
        retptr += replen;
      }
      // copy the rest of the string.
      strcpy(retptr, oriptr);
    }
    return returned;
  }
}

void wish_loop(FILE* fp, bool batch_mode){
    char *line = NULL;
    size_t len = 0;
    int exit_status = NO_EXIT;
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
            free(line);
            break;
        }
        line[strcspn(line, "\n")] = 0;
        if(check_for_actual_command(line))
        {
            char* cleaned_concurrent = replace(line, "&", " & ");
            free(line);
            line = replace(cleaned_concurrent, ">", " > ");
            free(cleaned_concurrent);
            struct PathRet ret_struct = parse_command(line, paths);
            paths = ret_struct.path;
            exit_status = ret_struct.ret_code;
        }
        free(line);
        line = NULL;
    }while(exit_status == NO_EXIT);
    free_buffer(paths, path_len);
}


int main(int argc, char *argv[]) {
    if (argc == 2) {
        FILE* fp = fopen(argv[1], "r");
        if (fp == NULL) {
            print_error();
            return EXIT_FAILURE;
        }else{
            wish_loop(fp, true);
            fclose(fp);
        }
    }else if (argc > 2){
        print_error();
        return(EXIT_FAILURE);
    }else{
        wish_loop(stdin, false);
    }
    return EXIT_SUCCESS;
}
