#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "iteration_device.h"

//Viene chiamata quando si agisce sull' interruttore open
void switch_open_action(int state);
void switch_close_action(int state);
void tick_signal(int a);

timer_t timer;

int main(int argc, char *argv[]){

    Registry records[] = {
            {"time", "usage time", 0, &string_to_int, &seconds_to_string, false}
    };

    Switch switches[] = {
            {"open", &switch_open_action},
            {"close", &switch_close_action}
    };

    SignalBind signal_bindings[] = {
            {SIG_OPEN, &switch_open_action},
            {SIG_CLOSE, &switch_close_action},
            {SIG_TICK, &tick_signal}
    };

    DeviceData window = {
            WINDOW, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            sizeof(records) / sizeof(Registry),
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch)
    };

    g_device = window;

    create_timer(&timer);

    init_base_device(argv, argc);

    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                NULL, 0);

    delete_timer(timer);
    clean_base_device();

    exit(EXIT_SUCCESS);
}

void tick_signal(int a){
    g_device.records[0].value++;
}

void switch_open_action(int state){

    if((state == g_device.state) || (state == 0))
        send_response(OK_NO_CHANGES);
    else{
        set_timer_tick(timer, true);
        g_device.state = state;
        send_response(OK_DONE);
    }
}

void switch_close_action(int state){

    if((state == !g_device.state) || (state == 0))
        send_response(OK_NO_CHANGES);
    else{
        set_timer_tick(timer, false);
        g_device.records[0].value = 0;
        g_device.state = 0;
        send_response(OK_DONE);
    }
}