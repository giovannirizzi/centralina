#ifndef DEVICES_H
#define DEVICES_H

typedef int device_id;
typedef char* device_name;

//Numeri negativi per i dispositivi controllo e positivi per quelli d'iterazione
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

    // 0 spento 1 acceso
    char state;


    //Questi sono puntatori ad altre strutture, possono cambiare in base al tipo di device
    char* interruttori_struct;
    char* data_struct;

} DeviceBase;

/*
* Mappa i nomi dei device al loro device type
* @return INVALID_TYPE se viene passata una stringa che non corrisponde ha nessun device
*/
DeviceType device_string_to_type(const char* device_string);

/*
* Mappa i device_type a delle stringhe
*/
const char* device_type_to_string(DeviceType device_type);

/*
* Converte una stringa in un device_id
* @param *id puntatore ad un id dove verrà salvato il risultato
* @return 1 se la conversione è andata a buon fine altrimenti 0
*/
int string_to_device_id(const char* string, device_id *id);

#endif // DEVICES_H