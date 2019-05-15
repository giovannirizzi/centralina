#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include "device.h"
#include "utils.h"

const CommandBind BASE_COMMANDS[] = {{"info", &info_command},
                                     {"del", &del_command},
                                     {"getconf", &getconf_command},
                                     {"setconf", &setconf_command},
                                     {"getpid", &getpid_command},
                                     {"gettype", &gettype_command},
                                     {"switch", &switch_command}};

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
    g_device.state = value;
}

void open_signal(const int value){

    //TODO
    printf("Got SIG_OPEN, int val: %d\n", value);
    g_device.state = value;
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
    g_device.running = true;
    g_curr_out_stream = stdout;
    setlinebuf(stdout);

    if(n_args >= 2 && string_to_int(args[1], &id) == 0 && id >= 0){

        char fifo_path[100];

        sprintf(fifo_path, "/tmp/centralina/devices/%d", id);
        int fifo_in_fd = open_fifo(fifo_path, O_RDONLY | O_CLOEXEC);
        g_fifo_in_stream = fdopen(fifo_in_fd, "r");
        g_device.id = id;

        int fifo_out_fd = open_fifo(FIFO_DEVICES_RESPONSE, O_WRONLY | O_CLOEXEC);
        g_fifo_out_stream = fdopen(fifo_out_fd, "w");
        setlinebuf(g_fifo_out_stream);

    }
    else{
        g_fifo_out_stream = NULL;
        g_fifo_in_stream = NULL;
        g_device.id = -1;
        print_error("Debug mode, id: %d, signal_fd: %d\n", g_device.id, g_signal_fd);
    }

    //se il signal_fd è nell'argomento lo salvo
    if(n_args == 3 && string_to_int(args[2], &signal_fd) == 0)
        g_signal_fd = signal_fd;
    else{
        //creo un nuova signal_fd
        sigset_t mask;
        mask = set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE, SIG_BEGIN, SIG_END,
                               SIG_DELAY, SIG_PERC, SIG_TEMP, SIG_CLOCK);
        g_signal_fd = signalfd(-1, &mask, 0);
        if (g_signal_fd == -1)
            perror_and_exit("init_base_device: signalfd");
    }
}

void getpid_command(const char** args, const size_t n_args){

    //Se non scrivi \n alla fino lo mette, BISOGNA SCRIVERLO SEMPREEE
    //send_command(g_curr_out_stream, "%d", getpid());
    print_error("Device %d: recived getpid command\n", g_device.id);
    fprintf(g_curr_out_stream, "%d\n", getpid());
}
void gettype_command(const char** args, const size_t n_args){

    //Se non scrivi \n alla fino lo mette, BISOGNA SCRIVERLO SEMPREEE
    //send_command(g_curr_out_stream, "%d", getpid());
    print_error("Device %d: recived gettype command\n", g_device.id);
    fprintf(g_curr_out_stream, "%s\n", device_type_to_string(g_device.type));
}
void info_command(const char** args, const size_t n_args){

    //Se non scrivi \n alla fino lo mette, BISOGNA SCRIVERLO SEMPREEE
    //send_command(g_curr_out_stream, "%d", getpid());
    print_error("Device %d: recived info command\n", g_device.id);
    char info_string[200], tmp[50];
    sprintf(info_string, "%d-%d-%d", g_device.id, g_device.type, g_device.state);
    int i;
    for(i=0; i<g_device.num_records; i++){
        sprintf(tmp, "-%s=%d", g_device.records[i].label, g_device.records[i].value);
        strcat(info_string, tmp);
    }

    fprintf(g_curr_out_stream, "%s\n", info_string);
}

void del_command(const char** args, const size_t n_args){

    fprintf(g_curr_out_stream, "del command\n");
    g_device.running = false;
}

_Bool is_controlled(){

    return g_fifo_in_stream &&
                fileno(g_fifo_in_stream) != STDIN_FILENO;
}

void switch_power_action(int state){

    g_device.state = state;
}