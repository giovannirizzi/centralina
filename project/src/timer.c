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
struct tm tn;
char time_now[80];
char str[80];
//FINE PROVA

int main(int argc, char *argv[]){

    CommandBind timer_commands[] = {
            {"canadd", &canadd_command}
    };

    Registry records[] = {
            {"begin", "starting time", 0, &string_to_time, &seconds_to_string, true},
            {"end", "ending time", 0, &string_to_time, &seconds_to_string, true}
            //{"switch_controller", "switch controller", 0, &string_to_action, &int_to_string, true}
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
    int retval = string_to_time("11:24",&tempo);
    retval = time_to_string(tempo,str);
    g_device.records[0].value = tempo;
    retval = string_to_time("16:40",&tempo);
    retval = time_to_string(tempo,str);
    g_device.records[1].value = tempo;
    create_timer(&tim);
    set_timer_tick(tim, true);
    //FINE PROVA

    init_base_device(argv, argc);
    
    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                timer_commands, 1);

    clean_control_device();

    exit(EXIT_SUCCESS);
}

void tick_signal(int a){
    //INIZIO PROVA end time
    time_t t_now,t_begin,t_end;
    char temp[80];
    LineBuffer line_buffer = {NULL, 0};
    t = time(NULL);
    tn = *localtime(&t);
    strftime(temp,80,"%H:%M", &tn);
    //printf("Time now: %s\n",temp);
    strptime(temp, "%H:%M", &t_now);
    time_to_string(g_device.records[0].value,temp);
    //printf("Time begin: %s\n",temp);
    strptime(temp, "%H:%M", &t_begin);
    time_to_string(g_device.records[1].value,temp);
    //printf("Time end: %s\n",temp);
    strptime(temp, "%H:%M", &t_end);

    double diff_begin = difftime(t_now,t_begin);
    double diff_end = difftime(t_now,t_end);

    /*if(diff_begin>=0 && diff_end<=0){
        switch(g_device.records[2].value){
            case 0:
                send_command_to_child(0,"switch power on");
                read_child_response(0, &line_buffer);
                send_response("%s", line_buffer.buffer);
                break;
            case 1:
                send_command_to_child(0,"switch power off");
                read_child_response(0, &line_buffer);
                send_response("%s", line_buffer.buffer);
                break;
            case 2:
                send_command_to_child(0,"switch open on");
                read_child_response(0, &line_buffer);
                send_response("%s", line_buffer.buffer);
                break;
            case 3:
                send_command_to_child(0,"switch close on");
                read_child_response(0, &line_buffer);
                send_response("%s", line_buffer.buffer);
                break;
            default:
                printf("Non so\n");
        }
    }*/

    if(line_buffer.length > 0) free(line_buffer.buffer);
    //FINE PROVA
}

void switch_power_action(int state){
    g_device.state = state;
}

void canadd_command(const char** args, size_t n_args) {

    if(n_args != 1){
        send_response(INV_ARGS);
        return;
    }

    int tmp;
    if(string_to_int(args[0], &tmp)!=0) {
        send_response(INV_ARGS);
        return;
    }

    DeviceType new_device_type = device_string_to_type(device_type_to_string(tmp));

    if(new_device_type == INVALID_TYPE)
        send_response("can't link a control device that does not control devices");
    else if(children_devices.size == 0)
        send_response("yes");
    else
        send_response("timer can control only one device");

}

void set_begin_action(int state){
    g_device.records[0].value = state;
}

void set_end_action(int state){
    g_device.records[1].value = state;
}