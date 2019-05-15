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
    char* records[1], *registry[10], *value_str[1];
    int i, nRegistry, value;
    divide_string((char*)*args, records, 1, "$");
    sscanf(args[0], "%d|%d|%d", &g_device.id, &g_device.type, &g_device.state);
    nRegistry = divide_string(records[0], registry, 10, "&");

    divide_string(records[0], value_str, 1, "=");
    string_to_int(value_str[0], &value);
    for(i=0; i<g_device.num_records; i++)
        if(strcmp(g_device.records[i].label, records[0])==0) 
            g_device.records[i].value = value;

    for(i=0; i<nRegistry; i++){
        divide_string(registry[i], value_str, 1, "=");
        string_to_int(value_str[0], &value);
        for(i=0; i<g_device.num_records; i++)
            if(strcmp(g_device.records[i].label, registry[i])==0) 
                g_device.records[i].value = value; 
    }    

}

void getconf_command(const char** args, const size_t n_args){
    print_error("Device %d: received getconf command\n", g_device.id);
    char info_string[200], tmp[50];
    sprintf(info_string, "%d|%d|%d", g_device.id, g_device.type, g_device.state);
    int i;
    for(i=0; i<g_device.num_records; i++){
        if(i==0)
            sprintf(tmp, "$%s=%d", g_device.records[i].label, g_device.records[i].value);
        else
            sprintf(tmp, "&%s=%d", g_device.records[i].label, g_device.records[i].value);
        
        strcat(info_string, tmp);
    }

    send_response("%s", info_string);
}

void switch_command(const char** args, const size_t n_args){

    if(n_args != 2){
        send_response("INVALID ARGS");
        return;
    }

    int device_state;
    if(string_to_device_state(args[1], &device_state)==-1){
        send_response("INVALID SWITCH STATE");
        return;
    }

    int i;
    for(i=0; i<g_device.num_switches; i++)
        if(strcmp(g_device.switches[i].label, args[0]) == 0) {
            g_device.switches[i].action(device_state);
            return;
        }
    send_response("INVALID SWITCH NAME");
}