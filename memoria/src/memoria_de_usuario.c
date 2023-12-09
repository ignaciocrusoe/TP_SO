#include "memoria_de_usuario.h"

extern void* memoria_de_usuario;
extern t_bitarray* frame_bitarray;
extern t_list* procesos_en_memoria;
extern sem_t mutex_lista_procesos;
extern sem_t cantidad_de_procesos;
extern int tam_pagina;
extern int tam_memoria;
extern t_log* logger;

uint8_t leer_de_memoria(t_direccion_fisica* direccion)
{
    uint8_t leido;
    memcpy(&leido, memoria_de_usuario + (direccion->frame * tam_pagina) + direccion->offset, sizeof(uint8_t));
    return leido;
}

void escribir_en_memoria(t_direccion_fisica* direccion, uint8_t a_escribir)
{
    memcpy(memoria_de_usuario + (direccion->frame * tam_pagina) + direccion->offset, &a_escribir, sizeof(uint8_t));
}

int32_t obtener_numero_de_marco(uint32_t pid, uint32_t pagina_buscada)
{
/*
    printf("obtener_numero_de_marco\n");
    bool es_el_pid(void* arg){
        t_proceso* proceso = (t_proceso*)arg;
        return proceso->pid == pid;
    }
*/
    bool es_la_pagina(void* arg){
        t_pagina* pagina = (t_pagina*)arg;
        return pagina->pagina == pagina_buscada;
    }
    t_proceso* proceso = buscar_proceso(pid);
    if(proceso != NULL)
    {
        //printf("Existe el proceso del que se busca el marco\n");
        t_pagina* pagina = list_find(proceso->tabla_de_paginas, es_la_pagina);
        if(pagina != NULL)
        {
            printf("El frame de la página %i del proceso %i es %i\n", pagina_buscada, pid, pagina->frame);
            return pagina->frame;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        printf("No existe el proceso del que se busca el marco\n");
        //Page fault
        return -1;
    }
}

void asignar_memoria(uint32_t pid, uint32_t size, uint32_t (*algoritmo)(void))
{
    uint32_t frame;
    for(int nro_pagina = 0; nro_pagina < (size / tam_pagina); nro_pagina++)
    {
        frame = algoritmo();
        printf("Se asigna el frame %i\n", frame);
        agregar_pagina(pid, nro_pagina, frame);
        printf("Se asigna el frame %i al proceso %i para la página %i\n", frame, pid, nro_pagina);
    }
}

void agregar_pagina(uint32_t pid, uint32_t nro_pagina, uint32_t nro_frame)
{
    bool tiene_mismo_pid(void* proceso) {
        return (((t_proceso*)proceso)->pid == pid);
    }

    t_proceso* proceso;
    proceso = list_find(procesos_en_memoria, tiene_mismo_pid);
    
    if(proceso != NULL)
    {
        printf("Se crea una página\n");
        t_pagina* pagina = crear_pagina(nro_pagina, nro_frame);
        printf("Se creó una página (p = %i - f = %i)\n", pagina->pagina, pagina->frame);
        if(pagina != NULL)
        {
            list_add(proceso->tabla_de_paginas, pagina);
            printf("Se insertó la página\n");
        }
    }
    else
    {
        /*NO EXISTE EL PROCESO*/
    }
}

/*ALGORTMOS DE REEMPLAZO*/

uint32_t contador_frame = 0;
uint32_t buscar_victima_fifo(void)
{
    uint32_t victima = contador_frame;
    contador_frame++;
    //Lógica de desalojo

    if(contador_frame >= tam_memoria / tam_pagina)
    {
        contador_frame = 0;
    }
    return victima;
}

//uint32_t buscar_victima_lru(void)
//{
//    uint32_t victima = contador_frame;
//
//    contador_frame++;
//    //Lógica de desalojo
//
//    if(contador_frame > tam_memoria / tam_pagina)
//    {
//        victima = ultima_pagina_accedida();
//    }
//    //for(uint32_t i; i < tam_memoria / tam_pagina)
//
//    //bitarray_set_bit(frame_bitarray, victima);
//    return victima;
//}
//
//uint32_t ultima_pagina_accedida()
//{
//    bool es_menor_el_timestamp(void* e1, void* e2)
//    {
//        return(((t_pagina*)e1)->timestamp > ((t_pagina*)e2)->timestamp);
//    }
//    bool el_menor_timestamp(void* e1, void* e2)
//    {
//        t_pagina* p1, p2;
//        p1 = list_find(((t_proceso*)e1)->tabla_de_paginas, es_menor_el_timestamp);
//    }
//    t_proceso* respuesta = (t_proceso*)list_find(procesos_en_memoria, el_menor_timestamp);
//    return list_find(respuesta->tabla_de_paginas, es_menor_el_timestamp);
//}


uint32_t buscar_victima_lru(void)
{
    uint32_t victima = contador_frame;

    contador_frame++;
    // Lógica de desalojo

    if (contador_frame >= tam_memoria / tam_pagina) {
        victima = ultima_pagina_accedida();
    }

    return victima;
}

//Función para encontrar la página con el menor timestamp
uint32_t ultima_pagina_accedida()
{
    bool es_menor_el_timestamp(void* e1, void* e2) //Entre páginas
    {
        return temporal_diff(((t_pagina*)e1)->timestamp, ((t_pagina*)e2)->timestamp) < 0;
    }

    bool el_menor_timestamp(void* e1, void* e2) //Entre procesos
    {
        t_pagina* p1 = list_get_minimum(((t_proceso*)e1)->tabla_de_paginas, es_menor_el_timestamp);
        t_pagina* p2 = list_get_minimum(((t_proceso*)e2)->tabla_de_paginas, es_menor_el_timestamp);
        return temporal_diff(p1->timestamp, p2->timestamp) < 0;
    }

    t_proceso* proceso_con_menor_timestamp = list_get_minimum(procesos_en_memoria, el_menor_timestamp);
    t_pagina* pagina_menor_timestamp = list_get_minimum(proceso_con_menor_timestamp->tabla_de_paginas, es_menor_el_timestamp);
    
    return pagina_menor_timestamp->pagina; 
}



t_proceso* buscar_proceso(uint32_t pid)
{
    bool comparar(void* arg){
        //printf("COMPARAR");
        t_proceso* proceso = arg;
        return proceso->pid == pid;
    }
    //printf("EMPIEZA BUCAR_PROCESO\n");
    t_proceso* proceso;
    printf("EMPIEZA BUCAR_PROCESO\n");
    sem_wait(&mutex_lista_procesos);
    //printf("hice waitss BUCAR_PROCESO\n");
    //pid_funcion = pid;
    proceso = list_find(procesos_en_memoria, comparar);
    sem_post(&mutex_lista_procesos);
    //printf("TERMINA BUCAR_PROCESO\n");
    return proceso;
}

void conexion_filesystem(void* arg)
{
    t_log* logger_hilo = iniciar_logger("logger_hilo_conec_FS.log","HILO_CPU");
    log_info(logger_hilo, "HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    log_info(logger_hilo,"Socket: %i", arg_h->socket_filesystem);
    t_direccion_fisica* direccion;
    //enviar_mensaje("LISTO_PARA_RECIBIR_PEDIDOS",arg_h->socket_cpu);
    while(1)
    {
        op_code codigo = recibir_operacion(arg_h->socket_filesystem);
        log_info(logger_hilo,"op_code: %i", codigo);
        uint32_t pid;
        switch (codigo)
        {
        case PEDIDO_LECTURA:
            direccion = recibir_direccion(arg_h->socket_filesystem);
            uint32_t lectura = leer_de_memoria(direccion);
            send(arg_h->socket_filesystem, &lectura, sizeof(uint32_t), NULL);
            break;

        case PEDIDO_ESCRITURA:
            uint32_t a_escribir;
            direccion = recibir_direccion(arg_h->socket_filesystem);
            recv(arg_h->socket_filesystem, &a_escribir, sizeof(uint32_t), MSG_WAITALL);
            printf("Voy a escribir en memoria\n");
            escribir_en_memoria(direccion, a_escribir);
            break;

        default:
            liberar_conexion(arg_h->socket_filesystem);
            return;
        }

    }
}