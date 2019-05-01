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

int read_incoming_signal(int sfd, SignalResponse *signal_res){

    static struct signalfd_siginfo fdsi;
    ssize_t s;

    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo))
        return -1;

    //Il cast non dovrebbe dare problemi...
    signal_res->signal_type = (SignalType)fdsi.ssi_signo - SIGRTMIN;
    signal_res->signal_val = fdsi.ssi_int;

    return 0;
}

sigset_t update_signal_mask(SignalType signal1, ...){

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
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        handle_error("sigprocmask");

    return mask;
}