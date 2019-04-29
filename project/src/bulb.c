#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include "devices.h"

int main(int argc, char *argv[]){

    int sfd;
    struct signalfd_siginfo fdsi;
    ssize_t s;

    printf("PID: %d\n", getpid());

    sfd = create_signalfd(-1, SIG_POWER, SIG_OPEN, SIG_CLOSE);

    if (sfd == -1)
        handle_error("signalfd");

    // Test invio segnale a me stesso
    union sigval val_union = {.sival_int = 45 };
    sigqueue(getpid(), SIGRTMIN + SIG_POWER, val_union);

    for (;;) {
       
        s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s != sizeof(struct signalfd_siginfo))
            handle_error("read");

        int signal = fdsi.ssi_signo - SIGRTMIN;

        switch(signal){

            case SIG_POWER:
                printf("Got SIG_POWER, int val: %d\n", fdsi.ssi_int);
                break;

            case SIG_OPEN:
                printf("Got SIG_OPEN, int val: %d\n", fdsi.ssi_int);
                break;

            case SIG_CLOSE:
                printf("Got SIG_CLOSE, int val: %d\n", fdsi.ssi_int);
                exit(EXIT_SUCCESS);

            default:
                printf("Read unexpected signal\n");
        }
    }
}