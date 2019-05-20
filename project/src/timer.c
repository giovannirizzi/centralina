#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "control_device.h"

void switch_power_action(int state);

int main(int argc, char *argv[]){

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
            NULL, 0);


    clean_control_device();

    exit(EXIT_SUCCESS);
}

void switch_power_action(int state){

    g_device.state = state;
}