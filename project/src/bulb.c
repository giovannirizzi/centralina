#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "utils.h"

Signal input_singal;
SignalBind signal_bindings[] = {{SIG_POWER, &power_signal}};

int main(int argc, char *argv[]){

    setlinebuf(stdout);

    int sfd;
    sigset_t mask;

    printf("PID: %d\n", getpid());

    /*
     * Il signal fd deve essere passato per il main non serve crearlo ogni
     * volta perché esso viene ereditato dai processi filgi, quindi basta
     * fare la set_signal_mask e singalfd nella centrlaina.
     * Però per debug se esso non viene passato per il main si può creare...
     */

    mask = set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE);
    sfd = signalfd(-1, &mask, 0);

    if (sfd == -1)
        perror_and_exit("signalfd");

    for (;;) {

        read_incoming_signal(sfd, &input_singal);

        handle_signal(&input_singal, signal_bindings,
                sizeof(signal_bindings)/ sizeof(SignalBind));
    }
}