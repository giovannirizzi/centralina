#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "utils.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

int main(int argc, char *argv[]){

    Command input_command = {NULL, 0, NULL, 0};
    RTSignal input_signal;

    SignalBind signal_bindings[] = {{SIG_POWER, &power_signal}};

    //Inizializzo il device in base agli argomenti passaati
    init_base_device(argv, argc);

    fd_set rfds;
    int stdin_fd = fileno(stdin);

    int fifo_fd = -1;
    if(fifo_in_stream)
        fifo_fd = fileno(fifo_in_stream);

    while(1){
        FD_ZERO(&rfds);

        FD_SET(device_data.signal_fd, &rfds);
        FD_SET(stdin_fd, &rfds);
        if(fifo_fd != -1)
            FD_SET(fifo_fd, &rfds);

        if(select(device_data.signal_fd+1, &rfds, NULL, NULL, NULL) == -1)
            perror_and_exit("select");
        else{

            //STDIN
            if (FD_ISSET(stdin_fd, &rfds)) {

                //Legge un comando (una linea)
                read_incoming_command(stdin, &input_command);

                //SETTA LA VARIABILE GLOBALE FILE* dove scrivere l'output dei comandi
                curr_out_stream = stdout;

                if(handle_device_command(&input_command, NULL, 0) == -1)
                    fprintf(curr_out_stream, "unknown command %s\n",
                            input_command.name);
            }

            //FIFO
            if(fifo_fd != -1 && FD_ISSET(fifo_fd, &rfds)){
                
                read_incoming_command(fifo_in_stream, &input_command);

                curr_out_stream = fifo_out_stream;

                if(handle_device_command(&input_command, NULL, 0) == -1)
                    fprintf(curr_out_stream, "unknown command %s\n",
                            input_command.name);
            }

            //SIGNAL
            if (FD_ISSET(device_data.signal_fd, &rfds)) {

                read_incoming_signal(device_data.signal_fd, &input_signal);

                handle_signal(&input_signal, signal_bindings,
                              sizeof(signal_bindings) / sizeof(SignalBind));
            }
        }
    }
}