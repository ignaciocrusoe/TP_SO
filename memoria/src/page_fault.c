#include "page_fault.h"

extern void* memoria_de_usuario;
extern t_bitarray* frame_bitarray;
extern t_list* procesos_en_memoria;
extern sem_t mutex_lista_procesos;
extern sem_t cantidad_de_procesos;
extern int tam_pagina;
extern int tam_memoria;
extern t_log* logger;

/*ALGORTMOS DE REEMPLAZO*/
uint32_t contador_frame = 0;
t_algoritmo_response* buscar_victima_fifo(void)
{
    t_algoritmo_response* respuesta = malloc(sizeof(t_algoritmo_response));
    bool esta_presente(void* arg)
    {
        return (((t_pagina*)arg)->presencia == 1);
    }
    t_pagina* pagina_con_el_menor_el_timestamp(void* e1, void* e2)
    {
        if(((t_pagina*)e1)->timestamp_carga > ((t_pagina*)e2)->timestamp_carga)
        {
            return e2;
        }
        else
        {
            return e1;
        }
    }
    t_proceso* proceso_con_el_menor_el_timestamp(void* e1, void* e2)
    {
        t_pagina* p1;
        t_pagina* p2;
        t_list* list_p1 = list_filter(((t_proceso*)e1)->tabla_de_paginas, esta_presente);
        t_list* list_p2 = list_filter(((t_proceso*)e2)->tabla_de_paginas, esta_presente);
        p1 = list_get_minimum(list_filter(((t_proceso*)e1)->tabla_de_paginas, esta_presente), pagina_con_el_menor_el_timestamp);
        p2 = list_get_minimum(list_filter(((t_proceso*)e2)->tabla_de_paginas, esta_presente), pagina_con_el_menor_el_timestamp);
        list_destroy(list_p1);
        list_destroy(list_p2);
        if(p1->timestamp_carga > p2->timestamp_carga)
        {
            return e2;
        }
        else
        {
            return e1;
        }
    }
    respuesta->proceso = list_get_minimum(procesos_en_memoria, proceso_con_el_menor_el_timestamp);
    printf("Proceso víctima = %i\n", respuesta->proceso->pid);
    t_list* lista = list_filter(respuesta->proceso->tabla_de_paginas, esta_presente);
    respuesta->pagina = list_get_minimum(lista, pagina_con_el_menor_el_timestamp);
    
    list_destroy(lista);
    return respuesta;
}

t_algoritmo_response* buscar_victima_lru(void)
{
    t_algoritmo_response* respuesta = malloc(sizeof(t_algoritmo_response));
    bool esta_presente(void* arg)
    {
        return (((t_pagina*)arg)->presencia == 1);
    }
    t_pagina* pagina_con_el_menor_el_timestamp(void* e1, void* e2)
    {
        if(((t_pagina*)e1)->timestamp_uso > ((t_pagina*)e2)->timestamp_uso)
        {
            return e2;
        }
        else
        {
            return e1;
        }
    }
    t_proceso* proceso_con_el_menor_el_timestamp(void* e1, void* e2)
    {
        t_pagina* p1;
        t_pagina* p2;
        t_list* list_p1 = list_filter(((t_proceso*)e1)->tabla_de_paginas, esta_presente);
        t_list* list_p2 = list_filter(((t_proceso*)e2)->tabla_de_paginas, esta_presente);
        p1 = list_get_minimum(list_p1, pagina_con_el_menor_el_timestamp);
        p2 = list_get_minimum(list_p2, pagina_con_el_menor_el_timestamp);
        list_destroy_and_destroy_elements(list_p1, free);
        list_destroy_and_destroy_elements(list_p2, free);
        if(p1->timestamp_uso > p2->timestamp_uso)
        {
            return e2;
        }
        else
        {
            return e1;
        }
        
    }
    respuesta->proceso = list_get_minimum(procesos_en_memoria, proceso_con_el_menor_el_timestamp);
    printf("Proceso víctima = %i\n", respuesta->proceso->pid);
    respuesta->pagina = list_get_minimum(list_filter(respuesta->proceso->tabla_de_paginas, esta_presente), pagina_con_el_menor_el_timestamp);
    return respuesta;
}

int32_t buscar_frame_libre()
{
    uint32_t i;
    for(i = 0; i < tam_memoria / tam_pagina; i++)
    {
        if(!bitarray_test_bit(frame_bitarray, i))
        {
            return i;
        }
    }
    return -1;
}

/*
uint32_t buscar_victima_lru(void)
{
    uint32_t victima = contador_frame;

    contador_frame++;
    //Lógica de desalojo

    if(contador_frame > tam_memoria / tam_pagina)
    {
        victima = ultima_pagina_accedida();
    }
    //for(uint32_t i; i < tam_memoria / tam_pagina)

    //bitarray_set_bit(frame_bitarray, victima);
    return victima;
}

uint32_t ultima_pagina_accedida()
{
    bool es_menor_el_timestamp(void* e1, void* e2)
    {
        return(((t_pagina*)e1)->timestamp > ((t_pagina*)e2)->timestamp);
    }
    bool el_menor_timestamp(void* e1, void* e2)
    {
        t_pagina* p1, p2;
        p1 = list_find(((t_proceso*)e1)->tabla_de_paginas, es_menor_el_timestamp);
    }
    t_proceso* respuesta = (t_proceso*)list_find(procesos_en_memoria, el_menor_timestamp);
    return list_find(respuesta->tabla_de_paginas, es_menor_el_timestamp);
}
*/

void* leer_pagina(uint32_t nro_frame)
{
    void* leido = malloc(tam_pagina);
    memcpy(leido, memoria_de_usuario + (nro_frame * tam_pagina), tam_pagina);
    return leido;
}

void escribir_pagina(uint32_t nro_frame, void* a_escribir)
{
    memcpy(memoria_de_usuario + (nro_frame * tam_pagina) , a_escribir, tam_pagina);
}

void swap_in(int socket_swap, t_pagina* pagina, uint32_t frame, t_proceso* proceso)
{
    void* a_escribir = malloc(tam_pagina);
    enviar_operacion(socket_swap, LEER_SWAP);
    send(socket_swap, &(pagina->posicion_en_swap), sizeof(uint32_t), NULL);
    recv(socket_swap, a_escribir, tam_pagina, MSG_WAITALL);
    escribir_pagina(frame, a_escribir);
    free(a_escribir);
    log_info(logger, "SWAP IN -  PID: %i - Marco: %i - Page In: %i-%i", proceso->pid, frame, proceso->pid, pagina->pagina);
}

void swap_out(int socket_swap, t_pagina* pagina, uint32_t frame,t_proceso* proceso)
{
    t_response respuesta;
    void* a_escribir = leer_pagina(pagina->frame);
    enviar_operacion(socket_swap, ESCRIBIR_SWAP);
    printf("pagina->posicion_en_swap = %i\n", pagina->posicion_en_swap);
    send(socket_swap, &(pagina->posicion_en_swap), sizeof(uint32_t), NULL);
    printf("Mandé la página (ptr = %i)\n", a_escribir);
    send(socket_swap, a_escribir, tam_pagina, NULL);
    respuesta = recibir_respuesta(socket_swap);
    free(a_escribir);
    log_info(logger, "SWAP OUT -  PID: %i - Marco: %i - Page Out: %i-%i", proceso->pid, frame, proceso->pid, pagina->pagina);
}