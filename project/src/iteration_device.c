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


void getinfo_command(const char **args, size_t n_args){

    print_error("Device %d: received getinfo command\n", g_device.id);

    char info_string[300];
    char tmp[100];
    int i;

    const char* state_str = device_state_to_string(g_device.state, g_device.type);

    sprintf(info_string, "id=%d|type=%s|state=%s|", g_device.id,
            device_type_to_string(g_device.type), state_str);

    for(i=0; i<g_device.num_records; i++){
        if(i==0)
            sprintf(tmp, "%s=%s", g_device.records[i].description, "valore");
        else
            sprintf(tmp, "|%s=%s", g_device.records[i].description, "valore");

        strcat(info_string, tmp);
    }

    send_response("%s", info_string);
}

void setconf_command(const char** args, const size_t n_args){

    print_error("Device %d: received setconf command\n", g_device.id);

    char *records[1];
    int value;

    int num = divide_string((char*)args[0], records, 1, "|");
    if(num == 1){
        string_to_int(args[0], &value);
        g_device.state = value;
        set_records_from_string(records[0]);
    }
}

void getconf_command(const char** args, const size_t n_args){

    print_error("Device %d: received getconf command\n", g_device.id);

    char conf_str[200], records[180];
    memset(records, 0, sizeof(conf_str) / sizeof(char));

    sprintf(conf_str, "%d|%d|%d|", g_device.id, g_device.type, g_device.state);

    int num = get_records_string(records);
    if(num > 0)
        strcat(conf_str, records);

    send_response("%s", conf_str);
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