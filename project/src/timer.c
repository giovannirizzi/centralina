#include <stdlib.h>
#include <stdio.h>
#include <time.h>//serve solo per prova
#include "utils.h"
#include "control_device.h"

void switch_power_action(int state);
void tick_signal(int a);
int string_to_action(const char* string, int *id);
int action_to_string(int action, char* string);

//INIZIO PROVA end time
timer_t tim;
time_t t;
struct tm tm;
char time_now[80];
char str[80];
//FINE PROVA

int main(int argc, char *argv[]){

    CommandBind timer_commands[] = {
            {"canadd", &canadd_command}
    };

    Registry records[] = {
            {"begin", "starting time", 0, &string_to_time, &seconds_to_string, true},
            {"end", "ending time", 0, &string_to_time, &seconds_to_string, true},
            {"switch_controller", "switch controller", 2, &string_to_action, &action_to_string, true}
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
    string_to_time("14:58",&tempo);
    //time_to_string(tempo,str);
    g_device.records[0].value = tempo;
    string_to_time("15:03",&tempo);
    //time_to_string(tempo,str);
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
    tm = *localtime(&t);
    strftime(temp,80,"%H:%M", &tm);
    strptime(temp, "%H:%M", &tm);
    t_now = mktime(&tm);
    time_to_string(g_device.records[0].value,temp);
    strptime(temp, "%H:%M", &tm);
    t_begin = mktime(&tm);
    time_to_string(g_device.records[1].value,temp);
    strptime(temp, "%H:%M", &tm);
    t_end = mktime(&tm);

    double diff_begin = difftime(t_now,t_begin);
    double diff_end = difftime(t_now,t_end);

    if(diff_begin>=0 && diff_end<=0){
        if(children_devices.size != 0){
            switch(g_device.records[2].value){
                case 0:
                    send_command_to_child(0,"switch power on");
                    if(diff_end == 0.0)
                        send_command_to_child(0,"switch power off");
                    break;
                case 1:
                    send_command_to_child(0,"switch power off");
                    if(diff_end == 0.0)
                        send_command_to_child(0,"switch power on");
                    break;
                case 2:
                    send_command_to_child(0,"switch open on");
                    if(diff_end == 0.0)
                        send_command_to_child(0,"switch close on");
                    break;
                case 3:
                    send_command_to_child(0,"switch close on");
                    if(diff_end == 0.0)
                        send_command_to_child(0,"switch opem on");
                    break;
                default:
                    printf("Non so\n");
            }
            read_child_response(0, &line_buffer);
            print_error("%s\n",line_buffer.buffer);
        }
    }

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

int string_to_action(const char* string, int *id){
    if(strcmp(string,"power-on") == 0)
        *id = 0;
    else if(strcmp(string,"power-off") == 0)
        *id = 1;
    else if(strcmp(string,"open-on") == 0)
        *id = 2;
    else if(strcmp(string,"close-on") == 0)
        *id = 3;
    else 
        return -1;
    return 0;
}

int action_to_string(int action, char* string){
    switch(action){
        case 0:
            sprintf(string,"power-on");
            break;
        case 1:
            sprintf(string,"power-off");
            break;
        case 2:
            sprintf(string,"open-on");
            break;
        case 3:
            sprintf(string,"close-on");
            break;
        default:
            return -1;
    }
}