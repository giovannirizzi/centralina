#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "manualcontrol.h"


int main(int argc, char *argv[]){

    //Test invio segnale a me stesso
    send_signal(getpid(), SIG_POWER, 0);

    return EXIT_SUCCESS;
}

void send_signal(pid_t pid, SignalType signal, int val){

    union sigval val_union = {.sival_int = val };
    sigqueue(pid, SIGRTMIN + signal, val_union);
}