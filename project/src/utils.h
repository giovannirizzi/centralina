#ifndef UTILS_H
#define UTILS_H

#include "devices.h"

/*
 * Mappa i nomi dei device al loro device type
 * @return INVALID_TYPE se viene passata una stringa che non
 * corrisponde ha nessun device
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

#endif // UTILS_H