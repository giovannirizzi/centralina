#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "utils.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "utils.h"
#include "iteration_device.h"

void switch_power_action(int state);
void tick_signal(int a);

timer_t timer;

int main(int argc, char *argv[]){

    Registry records[] = {"time", "descrizione", 0, &string_to_int};
    Switch switches[] = {"power", &switch_power_action};

    SignalBind signal_bindings[] = {
            {SIG_POWER, &switch_power_action},
            {SIG_TICK, &tick_signal}
    };

    DeviceBase bulb = {
            BULB, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            1, //NUM RECORDS
            (Switch*)&switches,
            1 //NUM SWITCHES
    };

    g_device = bulb;

    create_timer(&timer);

    //Inizializzo il device in base agli argomenti passaati
    init_base_device(argv, argc);
    
    device_loop(signal_bindings, 2, NULL, 0);


    print_error("Device %d: sto terminando\n", g_device.id);

    delete_timer(&timer);

    /**
     * CLEANUP RISORSE
     */

    exit(EXIT_SUCCESS);
}

void tick_signal(const int a){
    g_device.records[0].value++;
}

void switch_power_action(int state){

    if(state==0){
        set_timer_tick(timer, false);
        g_device.records[0].value = 0;
    } else
        set_timer_tick(timer, true);
    g_device.state = state;
}