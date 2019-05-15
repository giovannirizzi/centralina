#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

void switch_command(const char** args, const size_t n_args){

    if(n_args != 2){
        fprintf(g_curr_out_stream, "invalid args\n");
        return;
    }

    int device_state;
    if(string_to_device_state(args[1], &device_state)==-1){
        fprintf(g_curr_out_stream, "invalid state\n");
        return;
    }

    int i;

    for(i=0; i<g_device.num_switches; i++){

        if(strcmp(g_device.switches[i].label, args[0]) == 0)
            g_device.switches[i].action(device_state);
        else
            fprintf(g_curr_out_stream, "invalid siwtch\n");
    }
}