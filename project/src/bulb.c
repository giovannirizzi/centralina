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
    RTSignal input_singal;

    SignalBind signal_bindings[] = {{SIG_POWER, &power_signal}};

    init_device(0, 0);

    /**
     * Il signal fd deve essere passato per il main non serve crearlo ogni
     * volta perché esso viene ereditato dai processi filgi, quindi basta
     * fare la set_signal_mask e singalfd nella centrlaina.
     * Però per debug se esso non viene passato per il main si può creare...
     */

    /**
     * Init signal file descriptor, dovebbe stare in init_device però
     */

    int signal_fd;
    sigset_t mask;

    mask = set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE);
    signal_fd = signalfd(-1, &mask, 0);

    if (signal_fd == -1)
        perror_and_exit("signalfd");

    fd_set rfds;
    int stdin_fd = fileno(stdin);

    while(1){
        FD_ZERO(&rfds);

        FD_SET(signal_fd, &rfds);
        FD_SET(stdin_fd, &rfds);

        if(select(signal_fd+1, &rfds, NULL, NULL, NULL) == -1)
            perror_and_exit("select");
        else{

            //STDIN
            if (FD_ISSET(stdin_fd, &rfds)) {

                //Legge un comando (una linea)
                read_incoming_command(stdin, &input_command);

                //SETTA LA VARIABILE GLOBALE FILE* dove scrivere l'output dei comandi
                command_output = stdout;

                if(handle_device_command(&input_command, NULL, 0) == -1)
                    fprintf(command_output, "Unknown command %s\n",
                            input_command.name);
            }

            //SIGNAL
            if (FD_ISSET(signal_fd, &rfds)) {

                read_incoming_signal(signal_fd, &input_singal);

                handle_signal(&input_singal, signal_bindings,
                              sizeof(signal_bindings) / sizeof(SignalBind));
            }
        }
    }
}