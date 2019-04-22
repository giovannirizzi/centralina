#ifndef DEVICES_H
#define DEVICES_H

typedef int DEVICE_ID;
typedef char* DEVICE_NAME;


 //pensavo di usare numeri negativi per i dispositivi di controlloe positivi per quelli di iterazione
 //cosi è facile fare il check se è un dispositivo di controllo
typedef enum {CENTRALINA = -1} DEVICE_TYPE;

typedef struct{

    DEVICE_TYPE type;

    // 0 spento 1 acceso
    char state;


    //Questi sono puntatori ad altre strutture, possono cambiare in base al tipo di device
    char* interruttori_struct;
    char* data_struct;

} Base_Device;


#endif // DEVICES_H