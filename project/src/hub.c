#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "control_device.h"


int main(int argc, char *argv[]){

    Registry records[] = {
           // {"time", "usage time", 0, &string_to_int, &seconds_to_string, false}
    };
    Switch switches[] = {{
        //"power", &switch_power_action
    }};

    SignalBind signal_bindings[] = {
           // {SIG_POWER, &switch_power_action},
            //{SIG_TICK, &tick_signal}
    };

    DeviceData hub = {
            HUB, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            sizeof(records) / sizeof(Registry), //NUM RECORDS
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch) //NUM SWITCHES
    };

    g_device = hub;

    init_control_device(argv, argc);

    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                NULL, 0);

    clean_control_device();

    exit(EXIT_SUCCESS);
}