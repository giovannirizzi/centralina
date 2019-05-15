#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "iteration_device.h"

void switch_open_action(int state);
void switch_close_action(int state);
void tick_signal(int a);

timer_t timer;

int main(int argc, char *argv[]){

    Registry records[] = {"time", "descrizione", 0, &string_to_int};
    Switch switches[] = {
            {"open", &switch_open_action},
            {"close", &switch_close_action}
    };

    SignalBind signal_bindings[] = {
            {SIG_OPEN, &switch_open_action},
            {SIG_CLOSE, &switch_close_action},
            {SIG_TICK, &tick_signal}
    };

    DeviceBase window = {
            WINDOW, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            sizeof(records) / sizeof(Registry), //NUM RECORDS
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch) //NUM SWITCHES
    };

    g_device = window;

    create_timer(&timer);

    //Inizializzo il device in base agli argomenti passaati
    init_base_device(argv, argc);

    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                NULL, 0);

    print_error("Device %d: sto terminando\n", g_device.id);

    delete_timer(timer);

    /**
     * CLEANUP RISORSE
     */

    exit(EXIT_SUCCESS);
}

void tick_signal(int a){
    g_device.records[0].value++;
}

void switch_open_action(int state){
    if(g_curr_out_stream != NULL){
        if((state == g_device.state) || (state == 0))
            fprintf(g_curr_out_stream,"Already set\n");
        else{
            set_timer_tick(timer, true);
            g_device.state = state;
            fprintf(g_curr_out_stream,"Done\n");
        }
    }
}

void switch_close_action(int state){
    if(g_curr_out_stream != NULL){
        if((state == !g_device.state) || (state == 0))
            fprintf(g_curr_out_stream,"Already set\n");
        else{
            set_timer_tick(timer, false);
            g_device.records[0].value = 0;
            g_device.state = state;
            fprintf(g_curr_out_stream,"Done\n");
        }
    }
}