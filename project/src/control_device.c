#include <stdlib.h>
#include <stdio.h>
#include "control_device.h"
#include "utils.h"

const CommandBind control_device_commands[] = {"add", &add_command};

int handle_device_command(const Command *c, const CommandBind custom_commands[], const size_t n){

    if(handle_command(c, base_commands,
            sizeof(base_commands)/ sizeof(CommandBind)) == 0)
        return 0;
    if(handle_command(c, control_device_commands, 1) == 0)
        return 0;
    return handle_command(c, custom_commands, n);
}

void info_command(const char** args, const size_t n_args){

    fprintf(command_output, "info command\n");
}

void del_command(const char** args, const size_t n_args){

    fprintf(command_output, "del command\n");
}

void setconf_command(const char** args, const size_t n_args){

    fprintf(command_output, "setconf command\n");
}

void getconf_command(const char** args, const size_t n_args){

    fprintf(command_output, "getconf command\n");
}

void add_command(const char** args, const size_t n_args){

    fprintf(command_output, "add command\n");
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