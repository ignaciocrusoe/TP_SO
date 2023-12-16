#include "bloques.h"

extern uint32_t cant_bloques_total;
extern uint32_t cant_bloques_swap;
extern uint32_t tam_bloque;
extern uint32_t retardo_acceso_bloque;
extern uint32_t retardo_acceso_fat;

extern t_log* logger;

extern FILE* bloques;

void* leer_bloque(uint32_t puntero)
{
    log_info(logger,"Lee datos de un bloque de: bloques.dat");
    void* bloque = malloc(tam_bloque);
    fseek(bloques, (cant_bloques_swap + puntero) * tam_bloque, SEEK_SET);
    fread(bloque, tam_bloque, 1, bloques);
    sleep(retardo_acceso_bloque / 1000);
    return bloque;
}

uint32_t leer_dato(uint32_t bloque, uint32_t offset)
{
    uint32_t dato;
    fseek(bloques, (cant_bloques_swap + bloque) * tam_bloque + offset, SEEK_SET);
    fread(&dato, sizeof(uint32_t), 1, bloques);
    sleep(retardo_acceso_bloque / 1000);
    log_info(logger,"Lee datos en tamaño de un bloque");
    return dato;
}
/*
void* leer_bloque(uint32_t bloque)
{
    void* dato = malloc(tam_bloque);
    fseek(bloques, (cant_bloques_swap + bloque) * tam_bloque, SEEK_SET);
    fread(&dato, tam_bloque, 1, bloques);
    sleep(retardo_acceso_bloque / 1000);
    log_info(logger,"Lee datos en tamaño de un bloque");
    return dato;
}
*/
void escribir_dato(uint32_t bloque, uint32_t offset, uint32_t dato)
{
    log_info(logger,"Escribe datos en el archivo: bloues.dat");
    fseek(bloques, (cant_bloques_swap + bloque) * tam_bloque + offset, SEEK_SET);
    fwrite(&dato, sizeof(uint32_t), 1, bloques);
    sleep(retardo_acceso_bloque / 1000);
}

void escribir_bloque(uint32_t bloque, void* dato)
{
    log_info(logger,"Escribe datos en el archivo: bloues.dat");
    fseek(bloques, (cant_bloques_swap + bloque) * tam_bloque, SEEK_SET);
    fwrite(dato, tam_bloque, 1, bloques);
    sleep(retardo_acceso_bloque / 1000);
}
/*
void escribir(uint32_t puntero, )
{
    void* bloque = malloc(tam_bloque);
    fseek(bloques, (cant_bloques_swap + puntero) * tam_bloque, SEEK_SET);
    fwrite()
    return bloque;
}
*/

int abrir_archivo_de_bloques(char* path)
{
    int archivo_de_bloques = open(path, O_RDWR);
    struct stat file_stat;
    fstat(archivo_de_bloques, &file_stat);
    off_t file_size = file_stat.st_size;
    if(archivo_de_bloques == -1)
    {
        archivo_de_bloques = open(path, O_CREAT | O_RDWR, (mode_t) 0664);
    }

    if(file_size != tam_bloque * cant_bloques_total)
    {
        truncate(archivo_de_bloques, tam_bloque * cant_bloques_total);
    }
    bloques = fdopen(archivo_de_bloques, "rb+");
    log_info(logger,"Abre archivo de bloques: bloques.dat");
    return archivo_de_bloques;
}