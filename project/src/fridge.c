#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "iteration_device.h"

void switch_open_action(int state);
void switch_close_action(int state);
void tick_signal(int a);
void set_delay_action();
void set_percentage_action(int state);
void set_temperature_action(int state);

timer_t timer;

int main(int argc, char *argv[]){

    Registry records[] = {
            {"time", "time open", 0, &string_to_int, &seconds_to_string, false},
            {"delay", "auto close delay", 15, &string_to_int, &seconds_to_string, true},
            {"percentage", "filling percentage", 0, &string_to_int, &seconds_to_string, false},
            {"temperature", "temperature", 0, &string_to_int, &seconds_to_string, true},
    };
    Switch switches[] = {
            {"open", &switch_open_action},
            {"close", &switch_close_action}
    };

    SignalBind signal_bindings[] = {
            {SIG_OPEN, &switch_open_action},
            {SIG_CLOSE, &switch_close_action},
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
            sizeof(records) / sizeof(Registry), //NUM RECORDS
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch) //NUM SWITCHES
    };

    g_device = fridge;

    create_timer(&timer);

    //Inizializzo il device in base agli argomenti passaati
    init_base_device(argv, argc);

    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                NULL, 0);

    print_error("Device %d: sto terminando\n", g_device.id);

    delete_timer(timer);
    clean_base_device();

    exit(EXIT_SUCCESS);
}

void tick_signal(int a){
    g_device.records[0].value++;
    if(g_device.records[0].value == g_device.records[1].value)
        switch_close_action(1);
}

void switch_open_action(int state){

    if((state == g_device.state) || (state == 0))
        send_response("ALREADY SET");
    else{
        set_timer_tick(timer, true);
        g_device.state = state;
        send_response("DONE");
    }
}

void switch_close_action(int state){

    if((state == !g_device.state) || (state == 0))
        send_response("ALREADY SET");
    else{
        set_timer_tick(timer, false);
        g_device.records[0].value = 0;
        g_device.state = 0;
        send_response("DONE");
    }
}

void set_delay_action(){

}

void set_percentage_action(int state){
    g_device.records[2].value = state;
}

void set_temperature_action(int state){
    g_device.records[3].value = state;
}