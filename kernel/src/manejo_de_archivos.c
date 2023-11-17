#include "manejo_de_archivos.h"

t_archivo* crear_archivo(char* nombre_archivo, uint32_t tam_archivo, t_lock lock)
{
    t_archivo* archivo = malloc(sizeof(t_archivo));
    archivo->cola_blocked = queue_create();
    archivo->tam_archivo = tam_archivo;
    archivo->nombre = nombre_archivo;
    archivo->puntero = 0;
    archivo->lock = lock;
    return archivo;
}

t_lock de_string_a_t_lock(char* str)
{
    t_lock lock = NONE;
    if(!strcmp(str, "R"))
    {
        lock = READ;
    }
    else if(!strcmp(str, "W"))
    {
        lock = WRITE;
    }
}

t_archivo* buscar_archivo_segun_nombre(char* nombre, t_list* lista)
{


    bool tiene_mismo_nombre(void* nombre) {
        return (((t_archivo*)archivo)->nombre == nombre);
    }

    t_archivo* archivo = NULL;
    archivo = list_find(lista, tiene_mismo_nombre);
    return archivo;

}