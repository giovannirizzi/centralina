#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "utils.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

timer_t timerid;
struct itimerspec itval;

void clock_signal(int a);

timer_t create_timer(int sec){
    struct sigevent sigev;

    // Create the POSIX timer to generate signo
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGRTMIN+SIG_CLOCK;
    sigev.sigev_value.sival_ptr = &timerid;

    if (timer_create(CLOCK_REALTIME, &sigev, &timerid) == 0) {
        itval.it_value.tv_sec = sec;
        itval.it_value.tv_nsec = 0;
        itval.it_interval.tv_sec = itval.it_value.tv_sec;
        itval.it_interval.tv_nsec = 0;
        if (timer_settime(timerid, 0, &itval, NULL) != 0) {
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
    if (timer_settime(timerid, 0, &itval, NULL) != 0) {
        perror("time_settime error!");
    }
}

int main(int argc, char *argv[]){

    //timerid = create_timer(1);
    g_device.type = BULB;
    SignalBind signal_bindings[] = {{SIG_POWER, &power_signal}};
                                    //{SIG_CLOCK, &clock_signal}};
    Registry records[] = {"time", 0, &string_to_int};
    Switch switches[] = {"power", NULL};

    g_device.switches = (Switch*)&switches;
    g_device.num_switches = 1;
    g_device.records = (Registry*)&records;
    g_device.num_records = 1;

    LineBuffer line_buffer = {NULL, 0};
    Command input_command;
    RTSignal input_signal;
    g_device.type = BULB;

    //Inizializzo il device in base agli argomenti passaati
    init_base_device(argv, argc);
    
    fd_set rfds;
    FILE *curr_in;
    int stdin_fd = fileno(stdin);

    int fifo_fd = -1;
    if(g_fifo_in_stream)
        fifo_fd = fileno(g_fifo_in_stream);

    int max_fd = MAX(g_signal_fd, fifo_fd);

    while(g_device.running){
        FD_ZERO(&rfds);

        FD_SET(g_signal_fd, &rfds);
        FD_SET(stdin_fd, &rfds);
        if(is_controlled())
            FD_SET(fifo_fd, &rfds);

        if(select(max_fd+1, &rfds, NULL, NULL, NULL) == -1)
            perror_and_exit("select");
        else{

            if (FD_ISSET(stdin_fd, &rfds)) {
                curr_in = stdin;
                g_curr_out_stream = stdout;
            }

            if(FD_ISSET(fifo_fd, &rfds)){
                curr_in = g_fifo_in_stream;
                g_curr_out_stream = g_fifo_out_stream;
            }

            //Legge un comando (una linea)
            if(read_incoming_command(curr_in, &input_command, &line_buffer) == -1)
                g_device.running = false;

            if(handle_device_command(&input_command, NULL, 0) == -1)
                fprintf(g_curr_out_stream, "device: unknown command %s\n",
                        input_command.name);

            //SIGNAL
            if (FD_ISSET(g_signal_fd, &rfds)) {
            
                read_incoming_signal(g_signal_fd, &input_signal);

                printf("Got signal: %d, int val: %d\n",
                        input_signal.type, input_signal.value);

                handle_signal(&input_signal, signal_bindings,
                              sizeof(signal_bindings) / sizeof(SignalBind));
            }
        }
    }

    print_error("Device %d: sto terminando\n", g_device.id);

    /**
     * CLEANUP RISORSE
     */
    free(line_buffer.buffer);

    exit(EXIT_SUCCESS);
}

void clock_signal(const int a){
    printf("%d\n",g_device.records[0].value);
}