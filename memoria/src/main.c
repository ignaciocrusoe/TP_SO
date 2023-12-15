#include "main.h"

/*ESTRUCTURAS PARA MEMORIA DE INSTRUCCIONES*/
t_list* procesos_en_memoria;
char* saveptr;
sem_t mutex_lista_procesos;
sem_t cantidad_de_procesos;

/*ESTRUCTURAS PARA MEMORIA DE USUARIO*/
void* memoria_de_usuario;
t_bitarray* frame_bitarray;
t_pagina* (*algoritmo)(void);

int tam_pagina; 
int tam_memoria;

t_log* logger;

int main(int argc, char* argv[]){
    logger = iniciar_logger("log_memoria.log","CPU");
    t_config* config = iniciar_config("./cfg/memoria.config");

    char* puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
    char* ip_filesystem = config_get_string_value(config,"IP_FILESYSTEM");
    char* puerto_filesystem = config_get_string_value(config,"PUERTO_FILESYSTEM");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    int retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
    char* algoritmo_reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");

    if(!strcmp(algoritmo_reemplazo, "LRU"))
    {
        algoritmo = buscar_victima_lru;
    }
    else
    {
        algoritmo = buscar_victima_fifo;
    }

    memoria_de_usuario = malloc(tam_memoria);
    char* c_bitarray = malloc(tam_memoria / 8);
    frame_bitarray = bitarray_create_with_mode(c_bitarray, sizeof(c_bitarray), LSB_FIRST);
    
    for(int i = 0; i < tam_memoria; i++)
    {
        bitarray_clean_bit(frame_bitarray, i);
        //printf("%i", bitarray_test_bit(frame_bitarray, i));
    }

    printf("PUERTO_ESCUCHA=%s\n",puerto_escucha);
    int socket_servidor = iniciar_servidor(logger,puerto_escucha);
    int socket_filesystem = esperar_cliente(logger, socket_servidor);
    int socket_swap;
    if(socket_filesystem){
        log_info(logger,"Se conectó filesystem");
    }
    int socket_cpu = esperar_cliente(logger, socket_servidor);
    if(socket_cpu){
        log_info(logger,"Se conectó cpu");
    }
    int socket_kernel = esperar_cliente(logger, socket_servidor);
    if(socket_kernel){
        log_info(logger,"Se conectó kernel");
    }
    socket_swap = crear_conexion(logger, ip_filesystem, puerto_filesystem);
    send(socket_swap, &tam_pagina, sizeof(uint32_t), NULL);
    sem_init(&mutex_lista_procesos, 0, 1);
    sem_init(&cantidad_de_procesos, 0, 0);
    procesos_en_memoria = list_create();
    t_args_hilo args_conexion_kernel;
    args_conexion_kernel.socket_kernel = socket_kernel;
    args_conexion_kernel.socket_swap = socket_swap;
    args_conexion_kernel.socket_filesystem = socket_filesystem;
    pthread_t hilo_conexion_kernel;
    log_info(logger, "Declaré el hilo.");
    log_info(logger, "socket: %i", args_conexion_kernel.socket_kernel);
    pthread_create(&hilo_conexion_kernel, NULL, &conexion_kernel, (void*)&args_conexion_kernel);
    pthread_detach(hilo_conexion_kernel);
    log_info(logger, "Creé el hilo.");

    pthread_t hilo_conexion_cpu;
    t_args_hilo args_conexion_cpu;
    args_conexion_cpu.socket_cpu = socket_cpu;
    args_conexion_cpu.retardo_memoria = retardo_respuesta;

    pthread_t hilo_conexion_filesystem;
    t_args_hilo args_conexion_filesystem;
    args_conexion_filesystem.socket_filesystem = socket_filesystem;
    args_conexion_filesystem.socket_swap = socket_swap;
    pthread_create(&hilo_conexion_filesystem, NULL, &conexion_filesystem, (void*)&args_conexion_filesystem);
    pthread_detach(hilo_conexion_filesystem);
    //pthread_create(&hilo_conexion_cpu, NULL, &conexion_cpu, (void*)&args_conexion_cpu);
    //pthread_join(&hilo_conexion_kernel, NULL);
    //pthread_join(&hilo_conexion_cpu, NULL);

    conexion_cpu((void*)&args_conexion_cpu);
    config_destroy(config);
    log_destroy(logger);
    bitarray_destroy(frame_bitarray);
    free(memoria_de_usuario);
    free(c_bitarray);
}