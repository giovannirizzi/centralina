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

const CommandBind BASE_COMMANDS[] = {{"getinfo", &getinfo_command},
                                     {"del", &del_command},
                                     {"getconf", &getconf_command},
                                     {"setconf", &setconf_command},
                                     {"getpid", &getpid_command},
                                     {"gettype", &gettype_command},
                                     {"switch", &switch_command}};

FILE* g_curr_out_stream;

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

void init_base_device(char *args[], size_t n_args){

    /*
     * esempio ./bulb <id> <fd_signal>
     * se l'id non è presente significa che il dispositivo non viene controllato
     * (usato per debuging) quindi il device accetta comandi solo da stdin
     * se il fd_signal non è presente ne creo uno nuovo
     */

    device_id id;
    int signal_fd;
    g_running = true;
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
    }

    //se il signal_fd è nell'argomento lo salvo
    if(n_args == 3 && string_to_int(args[2], &signal_fd) == 0)
        g_signal_fd = signal_fd;
    else{
        //creo un nuova signal_fd
        sigset_t mask;
        mask = set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE, SIG_BEGIN, SIG_END,
                               SIG_DELAY, SIG_PERC, SIG_TEMP, SIG_TICK);
        g_signal_fd = signalfd(-1, &mask, 0);
        if (g_signal_fd == -1)
            perror_and_exit("init_base_device: signalfd");

        print_error("Debug mode, id: %d, signal_fd: %d\n", g_device.id, g_signal_fd);
    }
}

void getpid_command(const char** args, const size_t n_args){

    print_error("Device %d: received getpid command\n", g_device.id);

    send_response("%d", getpid());
}
void gettype_command(const char** args, const size_t n_args){

    print_error("Device %d: received gettype command\n", g_device.id);

    send_response("%s", device_type_to_string(g_device.type));
}
void getinfo_command(const char **args, size_t n_args){

    print_error("Device %d: received getinfo command\n", g_device.id);

    char info_string[200], tmp[50];
    sprintf(info_string, "%d|%d|%d", g_device.id, g_device.type, g_device.state);
    int i;
    for(i=0; i<g_device.num_records; i++){
        sprintf(tmp, "|%s=%d", g_device.records[i].label, g_device.records[i].value);
        strcat(info_string, tmp);
    }

    send_response("%s", info_string);
}

void del_command(const char** args, const size_t n_args){

    fprintf(g_curr_out_stream, "del command\n");
    g_running = false;
}

_Bool is_controlled(){

    return g_fifo_in_stream &&
                fileno(g_fifo_in_stream) != STDIN_FILENO;
}

void device_loop(const SignalBind signal_bindings[], const size_t n_sb,
        const CommandBind extra_commands[], const size_t n_dc){

    LineBuffer line_buffer = {NULL, 0};
    Command input_command;
    RTSignal input_signal;

    fd_set rfds;
    FILE *curr_in;
    int stdin_fd = fileno(stdin);

    int fifo_fd = -1;
    if(g_fifo_in_stream)
        fifo_fd = fileno(g_fifo_in_stream);

    int max_fd = MAX(g_signal_fd, fifo_fd);

    while(g_running){
        FD_ZERO(&rfds);

        FD_SET(g_signal_fd, &rfds);
        FD_SET(stdin_fd, &rfds);
        if(is_controlled())
            FD_SET(fifo_fd, &rfds);

        if(select(max_fd+1, &rfds, NULL, NULL, NULL) == -1)
            perror_and_exit("select");
        else{

            //SIGNAL
            if (FD_ISSET(g_signal_fd, &rfds)) {

                read_incoming_signal(g_signal_fd, &input_signal);

                handle_signal(&input_signal, signal_bindings, n_sb);

                g_curr_out_stream = NULL;
            }
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
                    g_running = false;

                if(handle_device_command(&input_command, extra_commands, n_dc) == -1)
                    send_response("UNKNOWN COMMAND");
            }
        }
    }
    free(line_buffer.buffer);
}

void clean_base_device(){

    if(g_fifo_in_stream) fclose(g_fifo_in_stream);
    if(g_fifo_out_stream ) fclose(g_fifo_out_stream);

    if(close(g_signal_fd) != 0);
        perror("clean_base_device: close singal fd");

    print_error("Device %d: sto terminando\n", g_device.id);
}

int send_response(char* response, ...){

    int n_write = 0;
    if(g_curr_out_stream != NULL){
        va_list args;
        va_start(args,response);
        n_write = vfprintf(g_curr_out_stream, response, args);
        if(response[strlen(response)-1] != '\n')
            n_write = fprintf(g_curr_out_stream,"\n");
        va_end(args);
    }
    return n_write;
}

int get_records_string(char* buffer){

    int i;
    char tmp[50];
    memset(tmp, 0, sizeof(tmp) / sizeof(char));
    for(i=0; i<g_device.num_records; i++){
        if(g_device.records->is_settable){
            if(i==0)
                sprintf(tmp, "%s=%d", g_device.records->label, g_device.records->value);
            else
                sprintf(tmp, "&%s=%d", g_device.records->label, g_device.records->value);
            strcat(buffer, tmp);
        }
    }
    return i;
}

int set_records_from_string(char *records){

    if(strlen(records) == 0)
        return 0;

    char* registry[10];
    int i, value;
    int num_registry = divide_string(records, registry, 10, "&");

    char* value_str[1];
    divide_string(records, value_str, 1, "=");
    string_to_int(value_str[0], &value);

    for(i=0; i<g_device.num_records; i++)
        if(strcmp(g_device.records[i].label, records)==0){
            g_device.records[i].value = value;
            num_registry++;
            break;
        }

    for (i = 0; i < num_registry-1; i++) {

        divide_string(registry[i], value_str, 1, "=");
        string_to_int(value_str[0], &value);
        for (i = 0; i < g_device.num_records; i++)
            if (strcmp(g_device.records[i].label, registry[i]) == 0)
                g_device.records[i].value = value;
    }
    return num_registry;
}

