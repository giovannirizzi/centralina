#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include "control_device.h"
#include "utils.h"

const const CommandBind CONTROL_DEVICE_COMMANDS[] = {"add", &add_command};

int handle_device_command(const Command *c, const CommandBind extra_commands[], const size_t n){

    if(handle_command(c, BASE_COMMANDS,
            sizeof(BASE_COMMANDS)/ sizeof(CommandBind)) == 0)
        return 0;
    if(handle_command(c, CONTROL_DEVICE_COMMANDS, 1) == 0)
        return 0;
    return handle_command(c, extra_commands, n);
}

void info_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "info command\n");
}

void del_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "del command\n");
}

void setconf_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "setconf command\n");
}

void getconf_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "getconf command\n");
}

void add_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "add command\n");

    /*
    char path[50];

    sprintf(path, "/usr/bin/xterm ./%s", device_type_to_string(device));

    printf("Adding %s...\n", device_type_to_string(device));
    printf("PID padre: %d\n", getpid());

    int fd_parent_to_child[2];
    int fd_child_to_parent[2];
    char *line_buffer = NULL;
    size_t len = 0;
    char buffer[20] = {0};

    if(pipe(fd_parent_to_child) != 0){
        perror("pipe");
        return -1;
    }
    if(pipe(fd_child_to_parent) != 0){
        perror("pipe");
        return -1;
    }
    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        return -1;
    }
    else if(pid == 0){
        dup2(fd_parent_to_child[0], STDIN_FILENO);
        dup2(fd_child_to_parent[1], STDOUT_FILENO);

        close(fd_parent_to_child[0]);
        close(fd_parent_to_child[1]);
        close(fd_child_to_parent[0]);
        close(fd_child_to_parent[1]);

        execl(path, NULL);
        perror_and_exit("[-] Error execl");
    }
    else{
        close(fd_parent_to_child[0]);
        close(fd_child_to_parent[1]);
        close(fd_parent_to_child[1]);

        FILE* file = fdopen(fd_child_to_parent[0],"r");
        if(file == NULL){
            perror("[-] Error fdopen");
        }

        printf("Ciao\n");

        ssize_t byteread = read_line(file, &line_buffer, &len);
        printf("Figlio: %s, num byte %d\n", line_buffer, (int) byteread);
        free(line_buffer);
        close(fd_child_to_parent[0]);
    }

     */
}

int add_child(ChildrenDevices* c, ChildDevice d){
    if(c->size == MAX_CHILDREN)
        return -1;
    c->children[c->size] = d;
    c->size++;
    printf("SIZE = %d\n",c->size);
    return 0;
}

int delete_child(ChildrenDevices* c, int i){
    if(c->size == 0)
        return -1;
    for ( ; i <= c->size; i++)
        c->children[i] = c->children[i + 1];
    c->size--;
    return 0;
}

void init_control_device(char *args[], size_t n_args){

	init_base_device(args, n_args);

	char* real_path = realpath(args[0], NULL);
    if(real_path){
        strcpy(BIN_PATH, dirname(real_path));
    }else{
        print_error("error, realpath\n");
    }
}