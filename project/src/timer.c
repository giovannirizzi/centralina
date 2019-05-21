#include <stdlib.h>
#include <stdio.h>
#include <time.h>//serve solo per prova
#include "utils.h"
#include "control_device.h"

void switch_power_action(int state);
void tick_signal(int a);

//INIZIO PROVA end time
timer_t tim;
time_t t;
struct tm tm;
char time_now[80];
char str[80];
//FINE PROVA

int main(int argc, char *argv[]){

    Registry records[] = {
            {"begin", "starting time", 0, &string_to_time, &seconds_to_string, true},
            {"end", "ending time", 0, &string_to_time, &seconds_to_string, true},
            {"switch_controller", "switch controller", 0, &string_to_action, NULL, true}
    };

    Switch switches[] = {"power", &switch_power_action};

    SignalBind signal_bindings[] = {
            {SIG_POWER, &switch_power_action},
            {SIG_TICK, &tick_signal}
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

    //INIZIO PROVA end time
    int tempo;
    int retval = string_to_time("10:09",&tempo);
    retval = time_to_string(tempo,str);
    g_device.records[0].value = tempo;
    //FINE PROVA

    create_timer(&tim);

    init_base_device(argv, argc);
    
    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
            NULL, 0);


    clean_control_device();

    exit(EXIT_SUCCESS);
}

void tick_signal(int a){
    //INIZIO PROVA end time
    t = time(NULL);
    tm = *localtime(&t);
    sprintf(time_now,"%d:%d",tm.tm_hour, tm.tm_min);
    printf("Sto tickando! %s\n",time_now);

    if(strcmp(time_now,str) == 0){
        print_error("Orari combaciano, chiudo finestra\n");
        send_command_to_child(0,"switch open on");
    }
    //FINE PROVA
}

void switch_power_action(int state){
    g_device.state = state;
}

void set_begin_action(int state){
    g_device.records[0].value = state;
}

void set_end_action(int state){
    g_device.records[1].value = state;
}

int string_to_action(const char* string, int *id){
    
}