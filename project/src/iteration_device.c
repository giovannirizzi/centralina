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

    char info_string[300], tmp[200], value_str[50];
    int i;

    const char* state_str = device_state_to_string(g_device.state, g_device.type);

    sprintf(info_string, "id=%d|type=%s|state=%s|", g_device.id,
            device_type_to_string(g_device.type), state_str);

    for(i=0; i<g_device.num_records; i++){
        g_device.records[i].format_value(g_device.records[i].value, value_str);
        if(i==0)
            sprintf(tmp, "%s=%s", g_device.records[i].description, value_str);
        else
            sprintf(tmp, "|%s=%s", g_device.records[i].description, value_str);

        strcat(info_string, tmp);
    }

    send_response("%s", info_string);
}

void getconf_command(const char** args, const size_t n_args){

    print_error("Device %d: received getconf command\n", g_device.id);

    char conf_str[200], records[180];
    memset(records, 0, sizeof(records) / sizeof(char));

    sprintf(conf_str, "%d|%d|%d|", g_device.id, g_device.type, g_device.state);

    int num = get_records_string(records);
    if(num > 0)
        strcat(conf_str, records);

    send_response("%s #", conf_str);
}

void switch_command(const char** args, const size_t n_args){

    if(n_args != 2){
        send_response(INV_ARGS);
        return;
    }

    int device_state;
    if(string_to_switch_state(args[1], &device_state)==-1){
        send_response(INV_SWITCH_STATE);
        return;
    }

    int i;
    for(i=0; i<g_device.num_switches; i++)
        if(strcmp(g_device.switches[i].label, args[0]) == 0) {
            g_device.switches[i].action(device_state);
            return;
        }
    send_response(INV_SWITCH);
}

void set_command(const char** args, const size_t n_args){
    if(n_args != 2){
        send_response(INV_ARGS);
        return;
    }

    int device_value;
    int i;
    for(i=0; i<g_device.num_records; i++){
        if(strcmp(g_device.records[i].label, args[0]) == 0){
            if(g_device.records[i].is_settable){
                if(g_device.records[i].convert_value(args[1], &device_value) == 0){
                    g_device.records[i].value = device_value;
                    send_response(OK_DONE);
                    return;
                } else {
                    send_response(INV_SET_VALUE);
                    return;
                }
            } else {
                send_response(ERR_REG_UNSETTABLE);
                return;
            }
        }
    }

    send_response(INV_REG);
}

void getrealtype_command(const char** args, const size_t n_args) {

    send_response("%d", g_device.type);
}

void gettree_command(const char** args, size_t n_args){

    send_response("%d|%d| #", g_device.id, g_device.type);
}