#include <stdlib.h>
#include <stdio.h>
#include "iteration_device.h"
#include "utils.h"


int handle_device_command(const Command *c, const CommandBind extra_commands[], const size_t n){

    if(handle_command(c, BASE_COMMANDS,
            sizeof(BASE_COMMANDS)/ sizeof(CommandBind)) == 0)
        return 0;

    return handle_command(c, extra_commands, n);
}

void info_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "info command\n");
}

void del_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "del command\n");
    exit(EXIT_SUCCESS);
}

void setconf_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "setconf command\n");
}

void getconf_command(const char** args, const size_t n_args){

    fprintf(curr_out_stream, "getconf command\n");
}