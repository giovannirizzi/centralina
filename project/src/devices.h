#ifndef DEVICES_H
#define DEVICES_H

typedef int device_id;
typedef char* device_name;


//Numeri negativi per i dispositivi controllo e positivi per quelli d'iterazione
typedef enum {
    INVALID_TYPE = 0,
    CENTRALINA = -1,
    BULB = 1
} DeviceType;

typedef struct{

    DeviceType type;

    // 0 spento 1 acceso
    char state;


    //Questi sono puntatori ad altre strutture, possono cambiare in base al tipo di device
    char* interruttori_struct;
    char* data_struct;

} DeviceBase;

DeviceType device_string_to_type(const char* device_string);
const char* device_type_to_string(DeviceType device_type);

#endif // DEVICES_H