#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>//serve solo per prova
#include "utils.h"
#include "iteration_device.h"

void switch_open_action(int state);
void switch_close_action(int state);
void tick_signal(int a);

timer_t timer;
//INIZIO PROVA end time
time_t t;
struct tm tm;
char time_now[80];
char str[80];
//FINE PROVA

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
            sizeof(records) / sizeof(Registry), //NUM RECORDS
            (Switch*)&switches,
            sizeof(switches) / sizeof(Switch) //NUM SWITCHES
    };

    g_device = window;

    //INIZIO PROVA end time
    int tempo;
    int retval = string_to_time("11:54",&tempo);
    retval = time_to_string(tempo,str);
    g_device.records[1].value = tempo;
    //FINE PROVA 

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
    //INIZIO PROVA end time
    t = time(NULL);
    tm = *localtime(&t);
    sprintf(time_now,"%d:%d",tm.tm_hour, tm.tm_min);
    if(strcmp(time_now,str) == 0){
        print_error("Orari combaciano, chiudo finestra\n");
        switch_close_action(1);
    }
    //FINE PROVA
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