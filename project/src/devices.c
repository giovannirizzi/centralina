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

void init_base_device(char *args[], size_t n_args){

    /*
     * esempio ./bulb <id> <fd_signal>
     * se l'id non è presente significa che il dispositivo non viene controllato
     * (usato per debuging) quindi il device accetta comandi solo da stdin
     * se il fd_signal non è presente ne creo uno nuovo
     */

    device_id id;
    int signal_fd;

    curr_out_stream = stdout;
    setlinebuf(stdout);

    if(n_args >= 2 && string_to_int(args[1], &id) == 0 && id >= 0){

        char fifo_path[100];
        sprintf(fifo_path, "/tmp/centralina/devices/%d", id);

        int fifo_in_fd = open_fifo(fifo_path, O_RDONLY);
        fifo_in_stream = fdopen(fifo_in_fd, "r");

        device_data.id = id;

        int fifo_out_fd = open_fifo(FIFO_DEVICES_RESPONSE, O_WRONLY);
        fifo_out_stream = fdopen(fifo_out_fd, "w");
        setlinebuf(fifo_out_stream);

    }
    else{
        fifo_out_stream = NULL;
        fifo_in_stream = NULL;
        device_data.id = -1;
        print_error("Debug mode\n");
    }

    //se il signal_fd è nell'argomento lo salvo
    if(n_args == 3 && string_to_int(args[2], &signal_fd) == 0)
        device_data.signal_fd = signal_fd;
    else{
        //creo un nuova signal_fd
        sigset_t mask;
        mask = set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE, SIG_DELAY,
                               SIG_PERC, SIG_TIME);
        device_data.signal_fd = signalfd(-1, &mask, 0);
        if (device_data.signal_fd == -1)
            perror_and_exit("init_base_device: signalfd");
    }

    print_error("id: %d, signal_fd: %d\n", device_data.id, device_data.signal_fd);
}

void getpid_command(const char** args, const size_t n_args){

    //Se non scrivi \n alla fino lo mette, BISOGNA SCRIVERLO SEMPREEE
    //send_command(curr_out_stream, "%d", getpid());
    fprintf(curr_out_stream, "%d\n", getpid());
}