#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "manualcontrol.h"


/*
 * Possibile utilizzo:
 *
 * manualcontrol whois <id> (Se usiamo gli id e non i PID)
 * per restituire il PID dato un id
 *
 * manaulcontrol switch <id/PID> <label interruttore> <posizione>
 * per inviare un segnale...
 *
 */


int main(int argc, char *argv[]){

    //Test invio segnale a me stesso
    send_signal(getpid(), SIG_POWER, 0);

    return EXIT_SUCCESS;
}

void send_signal(pid_t pid, SignalType signal, int val){

    union sigval val_union = {.sival_int = val };
    sigqueue(pid, SIGRTMIN + signal, val_union);
}