#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "iteration_device.h"

// Viene chiamata quando si agisce sull' interruttore power
void switch_power_action(int state);

/* Viene chiamata quando si riceve il segnale SIG_TICK inviato
 * ogni secondo se il timer è attivo */
void tick_signal(int a);

timer_t timer;

int main(int argc, char *argv[]){

    Registry records[] = {
            {"time", "usage time", 0, &string_to_int, &seconds_to_string, false}
    };

    Switch switches[] = {
            {"power", &switch_power_action}
    };

    SignalBind signal_bindings[] = {
            {SIG_POWER, &switch_power_action},
            {SIG_TICK, &tick_signal}
    };

    DeviceData bulb = {
            BULB, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            sizeof(records) / sizeof(Registry),
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch)
    };

    g_device = bulb;

    create_timer(&timer);

    init_base_device(argv, argc);
    
    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
            NULL, 0);

    delete_timer(timer);
    clean_base_device();

    exit(EXIT_SUCCESS);
}

void tick_signal(const int a){
    g_device.records[0].value++;
}

void switch_power_action(int state){

    if(state == g_device.state){
        send_response(OK_NO_CHANGES);
        return;
    }

    if(state==0){
        set_timer_tick(timer, false);
        g_device.records[0].value = 0;
    } else
        set_timer_tick(timer, true);
    g_device.state = state;
    send_response(OK_DONE);
}