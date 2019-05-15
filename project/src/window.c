#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "utils.h"
#include "iteration_device.h"

timer_t timerid;
struct itimerspec itval;
struct itimerspec oitval;
int i = 0;

timer_t start_timer(int sec){
    struct sigevent sigev;

    // Create the POSIX timer to generate signo
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGRTMIN+SIG_CLOCK;
    sigev.sigev_value.sival_ptr = &timerid;

    if (timer_create(CLOCK_REALTIME, &sigev, &timerid) == 0) {
        itval.it_value.tv_sec = sec;
        itval.it_value.tv_nsec = (long)(sec * 1000000L);
        itval.it_interval.tv_sec = itval.it_value.tv_sec;
        itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
        if (timer_settime(timerid, 0, &itval, &oitval) != 0) {
            perror("time_settime error!");
        }
    } else {
        perror("timer_create error!");
        return -1;
    }
    return timerid;
}

void stop_timer(){
    itval.it_value.tv_sec = 0;
    itval.it_value.tv_nsec = 0;
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 0;
    if (timer_settime(timerid, 0, &itval, &oitval) != 0) {
        perror("time_settime error!");
    }
}

void restart_timer(int sec){
    itval.it_value.tv_sec = sec;
    itval.it_value.tv_nsec = (long)(sec * 1000000L);
    itval.it_interval.tv_sec = itval.it_value.tv_sec;
    itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    if (timer_settime(timerid, 0, &itval, &oitval) != 0) {
        perror("time_settime error!");
    }   
}

void delete_timer(){
    timerid = timer_delete(&timerid);
}

void switch_open_action(int state);
void switch_close_action(int state);
void clock_signal(int a);

int main(int argc, char *argv[]){

    Registry records[] = {"time", "descrizione", 0, &string_to_int};
    Switch switches[] = {
            {"open", &switch_open_action},
            {"close", &switch_close_action}
    };

    SignalBind signal_bindings[] = {
            {SIG_OPEN, &switch_open_action},
            {SIG_CLOSE, &switch_close_action},
            {SIG_CLOCK, &clock_signal}
    };

    DeviceBase window = {
            WINDOW, //DEVICE TYPE
            -1, //ID
            0, //STATE
            (Registry*)&records,
            1, //NUM RECORDS
            (Switch*)&switches,
            1 //NUM SWITCHES
    };

    g_device = window;


    //Inizializzo il device in base agli argomenti passaati
    init_base_device(argv, argc);
    
    device_loop(signal_bindings, 2, NULL, 0);

    print_error("Device %d: sto terminando\n", g_device.id);

    /**
     * CLEANUP RISORSE
     */

    exit(EXIT_SUCCESS);
}

void clock_signal(const int a){
    g_device.records[0].value++;
}

void switch_open_action(int state){
    if(g_curr_out_stream != NULL){
        if((state == g_device.state) || (state == 0))
            fprintf(g_curr_out_stream,"Already set\n");
        else{
            //timerid = start_timer(1);
            g_device.state = state;
            fprintf(g_curr_out_stream,"Done\n");
        }
    }
}

void switch_close_action(int state){
    if(g_curr_out_stream != NULL){
        if((state == g_device.state) || (state == 0))
            fprintf(g_curr_out_stream,"Already set\n");
        else{
            /*stop_timer();
            g_device.records[0].value = 0;*/
            g_device.state = state;
            fprintf(g_curr_out_stream,"Done\n");
        }
    }
}