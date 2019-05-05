#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include "devices.h"
#include "utils.h"

void read_incoming_signal(int sfd, Signal *signal_res){

    static struct signalfd_siginfo fdsi;
    ssize_t s;

    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo))
        perror_and_exit("read signalfd");

    //Il cast non dovrebbe dare problemi...
    signal_res->signal_type = (SignalType)fdsi.ssi_signo - SIGRTMIN;
    signal_res->signal_val = fdsi.ssi_int;
}

sigset_t set_signal_mask(SignalType signal1, ...){

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

ssize_t read_incoming_command(FILE* stream, char** command){

    static size_t n = 0;
    ssize_t nread = getline(command, &n, stream);

    if(nread < 0)
        perror_and_exit("read_incoming_command");

    //Se alla fine della linea letta c'è un \n lo sostituisce con \0
    if(nread > 0 && (*command)[nread-1] == '\n')
        (*command)[nread-1] = '\0';

    return nread;
}

void power_signal(const int value){

    //TODO
    printf("Got SIG_POWER, int val: %d\n", value);
    device_data.state = value;
}