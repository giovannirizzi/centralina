#ifndef DEVICES_H
#define DEVICES_H

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
    int singal_fd;

    // 0 spento 1 acceso
    int state;

    /*
     * Questi sono puntatori ad altre strutture,
     * possono cambiare in base al tipo di device */
    const void* interruttori_struct;
    const void* data_struct;

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


/**
 * Setta la signal mask del processo, con i segnali real-time
 * passati come parametro
 * @param signal1 segnale real-time da mascherare
 * @param ... altri segnali
 * @return la mask dei segnali passati come argomento
 */
sigset_t set_signal_mask(SignalType signal1, ...);


/**
 * Signal function implementation
 *
 */

void power_signal(const int value);

/**
 * Global var for all devices
 */
DeviceBase device_data;


#endif // DEVICES_H