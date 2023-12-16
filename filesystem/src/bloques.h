#ifndef BLOQUES_H
#define BLOQUES_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <log/log_utils.h>
#include <sockets/sockets.h>
#include <sockets/client_utils.h>
#include <sockets/server_utils.h>
#include <pcb/pcb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>

void* leer_bloque(uint32_t puntero);
void escribir_dato(uint32_t bloque, uint32_t offset, uint32_t dato);
uint32_t leer_dato(uint32_t bloque, uint32_t offset);
int abrir_archivo_de_bloques(char* path);
void* leer_bloque(uint32_t bloque);
void escribir_bloque(uint32_t bloque, void* dato);

#endif