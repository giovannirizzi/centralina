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

/*void info_command(const char** args, const size_t n_args){

    fprintf(g_curr_out_stream, "info command\n");
}*/

void setconf_command(const char** args, const size_t n_args){
    sscanf(args[0], "%d-%d-%d", &g_device.id, &g_device.type, &g_device.state);
}

void getconf_command(const char** args, const size_t n_args){
    info_command(NULL, 0);
}