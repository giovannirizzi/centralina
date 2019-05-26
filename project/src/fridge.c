#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "iteration_device.h"

void switch_open_action(int state);
void tick_signal(int a);

//Implementano le azioni per i segnali
void set_delay_action(int state);
void set_percentage_action(int state);
void set_temperature_action(int state);

int percentage_to_string(int percentage, char* string);
int temperature_to_string(int temperature, char* string);

timer_t timer;

int main(int argc, char *argv[]){

    Registry records[] = {
            {"temperature", "temperature", 0, &string_to_temperature, &temperature_to_string, true},
            {"percentage", "filling percentage", 0, &string_to_int, &percentage_to_string, false},
            {"delay", "delay time", 30, &string_to_int, &seconds_to_string, true},
            {"time", "time open", 0, &string_to_int, &seconds_to_string, false},
    };

    Switch switches[] = {
            {"open", &switch_open_action}
    };

    SignalBind signal_bindings[] = {
            {SIG_OPEN, &switch_open_action},
            {SIG_TICK, &tick_signal},
            {SIG_DELAY, &set_delay_action},
            {SIG_PERC, &set_percentage_action},
            {SIG_TEMP, &set_temperature_action}
    };

    DeviceData fridge = {
            FRIDGE, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            sizeof(records) / sizeof(Registry),
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch)
    };

    g_device = fridge;

    create_timer(&timer);

    init_base_device(argv, argc);

    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                NULL, 0);

    delete_timer(timer);
    clean_base_device();

    exit(EXIT_SUCCESS);
}

void tick_signal(int a){
    g_device.records[3].value++;
    if(g_device.records[3].value == g_device.records[2].value ||
        g_device.records[2].value == 0)
        switch_open_action(0);
}

void switch_open_action(int state){

    if(state == g_device.state){
        send_response(OK_NO_CHANGES);
        return;
    }

    if(state == 0){
        set_timer_tick(timer, false);
        g_device.records[3].value = 0;
    } else
        set_timer_tick(timer, true);

    g_device.state = state;
    send_response(OK_DONE);
}

void set_delay_action(const int state){
    g_device.records[2].value = state;
}

void set_percentage_action(int state){
    if(state > 0 && state < 100)
        g_device.records[1].value = state;
}

void set_temperature_action(int state){
    if(state > -20 && state < 20)
        g_device.records[0].value = state;
}

int percentage_to_string(int percentage, char* string){
    sprintf(string, "%d%%", percentage);
    return 0;
}

int temperature_to_string(int temperature, char* string){
    sprintf(string, "%d°", temperature);
    return 0;
}