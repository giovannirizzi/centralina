#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"
#include "control_device.h"

void tick_signal(int a);
void set_begin_action(int state);
void set_end_action(int state);
int string_to_action(const char* string, int *id);
int action_to_string(int action, char* string);

timer_t tim;
time_t t;
struct tm tm;
LineBuffer line_buffer = {NULL, 0};
_Bool is_in_range = false;

void update_state();
void canadd_command(const char** args, size_t n_args);

int main(int argc, char *argv[]){

    CommandBind timer_commands[] = {
            {"canadd", &canadd_command}
    };

    int default_begin, default_end;
    string_to_time("8:00",&default_begin);
    string_to_time("10:49",&default_end);

    Registry records[] = {
            {"begin", "starting time", default_begin, &string_to_time, &time_to_string, true},
            {"end", "ending time", default_end, &string_to_time, &time_to_string, true},
            {"action", "timer action", 2, &string_to_action, &action_to_string, true}
    };

    SignalBind signal_bindings[] = {
            {SIG_TICK, &tick_signal},
            {SIG_BEGIN, &set_begin_action},
            {SIG_END, &set_end_action}
    };

    DeviceData timer = {
            TIMER, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            sizeof(records) / sizeof(Registry),
            NULL,
            0
    };

    g_device = timer;

    create_timer(&tim);
    set_timer_tick(tim, true);

    init_base_device(argv, argc);
    
    device_loop(signal_bindings, sizeof(signal_bindings)/ sizeof(SignalBind),
                timer_commands, 1);

    clean_control_device();

    if(line_buffer.length > 0) free(line_buffer.buffer);

    exit(EXIT_SUCCESS);
}

void tick_signal(int a){

    time_t t_now,t_begin,t_end;
    char temp[80];
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
        if(g_children_devices.size != 0){
            if(!is_in_range && diff_end != 0.0){
                switch(g_device.records[2].value){
                case 0:
                    send_command_to_child(0,"switch power on");
                    break;
                case 1:
                    send_command_to_child(0,"switch power off");
                    break;
                case 2:
                    send_command_to_child(0,"switch open on");
                    break;
                case 3:
                    send_command_to_child(0,"switch close on");
                    break;
                }
                is_in_range = true;
                read_child_response(0, &line_buffer);
                print_error("1: %s\n",line_buffer.buffer);
                update_state();
            }
            if(is_in_range && diff_end == 0.0){
                switch(g_device.records[2].value){
                case 0:
                    send_command_to_child(0,"switch power off");
                    break;
                case 1:
                    send_command_to_child(0,"switch power on");
                    break;
                case 2:
                    send_command_to_child(0,"switch close on");
                    break;
                case 3:
                    send_command_to_child(0,"switch open on");
                    break;
                }
                is_in_range = false;
                read_child_response(0, &line_buffer);
                print_error("2: %s\n",line_buffer.buffer);
                update_state();
            }
        }
    }
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
    else if(g_children_devices.size == 0)
        send_response("yes");
    else
        send_response("timer can control only one device");
}

void set_begin_action(int state){
    g_device.records[0].value = state;
    is_in_range = false;
}

void set_end_action(int state){
    g_device.records[1].value = state;
    is_in_range = false;
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

void update_state(){

    //Aggiorno lo stato in base a quello dei figli
    if(send_command_to_child(0, "getstate")==0)
        if(read_child_response(0, &line_buffer)>0)
            if(string_to_int(line_buffer.buffer, &g_device.state)!=0)
                g_device.state = 0;

}