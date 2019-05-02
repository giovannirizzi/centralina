#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include "devices.h"
#include "utils.h"
#include "iteration_device.h"

int main(int argc, char *argv[]){

    int sfd;
    sigset_t mask;
    SignalResponse sig_res;

    printf("PID: %d\n", getpid());

    mask = update_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE);

    sfd = signalfd(-1, &mask, 0);

    if (sfd == -1)
        handle_error("signalfd");

    for (;;) {

        if (read_incoming_signal(sfd, &sig_res) != 0)
            handle_error("read");

        switch(sig_res.signal_type){

            case SIG_POWER:
                printf("Got SIG_POWER, int val: %d\n", sig_res.signal_val);
                break;

            case SIG_OPEN:
                printf("Got SIG_OPEN, int val: %d\n", sig_res.signal_val);
                break;

            case SIG_CLOSE:
                printf("Got SIG_CLOSE, int val: %d\n", sig_res.signal_val);
                exit(EXIT_SUCCESS);

            default:
                printf("Read unexpected signal\n");
        }
    }
}