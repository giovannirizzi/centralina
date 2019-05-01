#ifndef DEVICES_H
#define DEVICES_H

#define handle_error(msg) \
            do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef int device_id;
typedef char* device_name;

//Numeri negativi per i devices di controllo e positivi per iterazione
typedef enum {
    TIMER = -3,
    HUB = -2,
    CENTRALINA = -1,
    INVALID_TYPE = 0,
    BULB = 1,
    WINDOW = 2,
    FRIDGE = 3
} DeviceType;

typedef struct{

    DeviceType type;
    device_id id;

    // 0 spento 1 acceso
    char state;

    /*
     * Questi sono puntatori ad altre strutture,
     * possono cambiare in base al tipo di device */
    char* interruttori_struct;
    char* data_struct;

} DeviceBase;

/* Segnali real-time usati per simulare il controllo manuale
 * Per ogni segnale si utilizza anche il valore intero associato
 * ad esso (sival_int) inviato con la syscall sigqueue().
*/
typedef enum{
    SIG_POWER = 0,
    SIG_OPEN,
    SIG_CLOSE
} SignalType;

/*
 * TODO
 */
typedef struct{
    SignalType signal_type;
    int signal_val;
} SignalResponse;

/*
 * TODO
 */
int read_incoming_signal(int sfd, SignalResponse *signal_res);

/*
 * Aggiorna la signal mask del processo, aggiungendo i segnali real-time
 * passati come parametro
 * @param signal1 segnale real-time da mascherare
 * @param ... altri segnali
 * @return la mask dei segnali passati come argomento
 */
sigset_t update_signal_mask(SignalType signal1, ...);


/**
 * TODO
 * @param device_base
 * @param id
 */
pid_t whois_command(DeviceBase *device_base, device_id id);


#endif // DEVICES_H