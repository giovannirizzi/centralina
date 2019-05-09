#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include "devices.h"
#include "utils.h"

const CommandBind BASE_COMMANDS[] = {{"info", &info_command},
                                     {"del", &del_command},
                                     {"getconf", &getconf_command},
                                     {"setconf", &setconf_command},
                                     {"getpid", &getpid_command}};

sigset_t set_signal_mask(RTSignalType signal1, ...){

    va_list ap;
    int i;
    sigset_t mask;

    sigemptyset(&mask);

    va_start(ap, signal1);

    for (i = signal1; i >= 0; i = va_arg(ap, int))
        sigaddset(&mask, SIGRTMIN + i);

    va_end(ap);

    /* Blocca i segnali settati nella mask cosi da prevenire
     * la chiamata del handler di default */
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
        perror_and_exit("sigprocmask");

    return mask;
}

void power_signal(const int value){

    //TODO
    printf("Got SIG_POWER, int val: %d\n", value);
    device_data.state = value;
}

void init_device(device_id id, int signalfd){

    /**
     * Apri la fifo /tmp/centralina/devices/0 -> fifo_response in write
     * Apri la fifo /tmp/centralina/devices/<id> -> fifo_request in read
     * Inizializza signalfd.....
     */

    curr_out_stream = stdout;
    setlinebuf(stdout);
    print_error("ID: %d\n", id);

    //Se è un valid id apro la fifo per ascoltare i comandi
    if(id > 0){

        char fifo_path[100];
        sprintf(fifo_path, "/tmp/centralina/devices/%d", id);

        int fifo_out_fd = open_fifo(fifo_path, O_RDONLY);
        fifo_out_stream = fdopen(fifo_out_fd, "r");

        setlinebuf(fifo_out_stream);
    }
    else{
        print_error("Invalid device id, cant init fifo\n");
    }
}

void getpid_command(const char** args, const size_t n_args){

    //Se non scrivi \n alla fino lo mette, BISOGNA SCRIVERLO SEMPREEE
    //send_command(curr_out_stream, "%d", getpid());
    fprintf(curr_out_stream, "%d\n", getpid());
}