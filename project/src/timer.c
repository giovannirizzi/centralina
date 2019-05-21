#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "control_device.h"

void switch_power_action(int state);

int main(int argc, char *argv[]){

    CommandBind timer_commands[] = {
            {"canadd", &canadd_command}
    };

    Registry records[] = {
            {"begin", "starting time", 0, &string_to_int, &seconds_to_string, false},
            {"end", "ending time", 0, &string_to_int, &seconds_to_string, false}
    };
    Switch switches[] = {"power", &switch_power_action};

    SignalBind signal_bindings[] = {
            {SIG_POWER, &switch_power_action}
    };

    DeviceData timer = {
            TIMER, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            sizeof(records) / sizeof(Registry), //NUM RECORDS
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch) //NUM SWITCHES
    };

    g_device = timer;

    init_base_device(argv, argc);
    
    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                timer_commands, 1);

    clean_control_device();

    exit(EXIT_SUCCESS);
}

void switch_power_action(int state){

    g_device.state = state;
}

void canadd_command(const char** args, size_t n_args) {

    if(n_args != 1){
        send_response(INV_ARGS);
        return;
    }

    int tmp;
    if(string_to_int(args[0], &tmp)!=0) {
        send_response(INV_ARGS);
        return;
    }

    DeviceType new_device_type = device_string_to_type(device_type_to_string(tmp));

    if(new_device_type == INVALID_TYPE)
        send_response("can't link a control device that does not control devices");
    else if(children_devices.size == 0)
        send_response("yes");
    else
        send_response("timer can control only one device");

}