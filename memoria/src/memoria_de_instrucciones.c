#include "memoria_de_instrucciones.h"

extern t_list* procesos_en_memoria;
extern char* saveptr;
extern sem_t mutex_lista_procesos;
extern t_bitarray* frame_bitarray;
extern sem_t cantidad_de_procesos;
extern int tam_pagina;
extern t_log* logger;
extern t_pagina* (*algoritmo)(void);

void conexion_cpu(void* arg)
{
    t_log* logger_hilo = iniciar_logger("logger_hilo_conec_cpu.log","HILO_CPU");
    //log_info(logger_hilo, "HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    //log_info(logger_hilo,"Socket: %i", arg_h->socket_cpu);
    t_direccion_fisica* direccion;
    t_pagina* pagina;
    //enviar_mensaje("LISTO_PARA_RECIBIR_PEDIDOS",arg_h->socket_cpu);
    while(1)
    {
        op_code codigo = recibir_operacion(arg_h->socket_cpu);
        //log_info(logger_hilo,"op_code: %i", codigo);
        uint32_t pid;
        switch (codigo)
        {
        case FETCH_INSTRUCCION:
            uint32_t program_counter;
            recv(arg_h->socket_cpu, &pid, sizeof(uint32_t), MSG_WAITALL);
            log_info(logger_hilo,"pid: %i", pid);
            recv(arg_h->socket_cpu, &program_counter, sizeof(uint32_t), MSG_WAITALL);
            log_info(logger_hilo,"ip: %i", program_counter);

            sem_wait(&cantidad_de_procesos);
            t_proceso* proceso = buscar_proceso(pid);
            //log_info(logger_hilo,"HAY QUE ENVIAR LA INSTRUCCION");
            sem_wait(&mutex_lista_procesos);

            if(program_counter >= proceso->instrucciones->elements_count){
                //Habría que mandarle un mensaje a CPU
                log_info(logger_hilo, "No hay más isntrucciones");
            }
            sleep(arg_h->retardo_memoria / 1000);
            log_info(logger_hilo,"Envío: %s", list_get(proceso->instrucciones, program_counter));
            enviar_mensaje(list_get(proceso->instrucciones, program_counter), arg_h->socket_cpu);
            sem_post(&mutex_lista_procesos);
            sem_post(&cantidad_de_procesos);
            break;
        
        case PEDIDO_DE_FRAME:
            printf("PEDIDO_DE_FRAME\n");
            uint32_t pagina_buscada;
            recv(arg_h->socket_cpu, &pid, sizeof(uint32_t), MSG_WAITALL);
            printf("Recibí pid\n");
            recv(arg_h->socket_cpu, &pagina_buscada, sizeof(uint32_t), MSG_WAITALL);
            printf("RECIBI PAGINA %i\n", pagina_buscada);
            int32_t marco = obtener_numero_de_marco(pid, pagina_buscada);
            enviar_frame(arg_h->socket_cpu, marco);
            break;

        case PEDIDO_LECTURA:
            direccion = recibir_direccion(arg_h->socket_cpu);
            uint32_t lectura = leer_de_memoria(direccion);
            pagina = buscar_pagina_segun_frame(direccion->frame);
            pagina->timestamp_uso = time(NULL);
            send(arg_h->socket_cpu, &lectura, sizeof(uint32_t), NULL);
            free(direccion);
            break;

        case PEDIDO_ESCRITURA:
            uint32_t a_escribir;
            direccion = recibir_direccion(arg_h->socket_cpu);
            recv(arg_h->socket_cpu, &a_escribir, sizeof(uint32_t), MSG_WAITALL);

            pagina = buscar_pagina_segun_frame(direccion->frame);
            pagina->timestamp_uso = time(NULL);
            printf("pagina->modificado = %i\n", pagina->modificado);
            pagina->modificado = 1;
            printf("pagina->modificado = %i\n", pagina->modificado);

            //printf("Voy a escribir en memoria\n");
            escribir_en_memoria(direccion, a_escribir);
            free(direccion);
            //send(arg_h->socket_cpu, &direccion, sizeof(uint32_t), NULL);
            break;

        case PEDIDO_SIZE_PAGINA:
            send(conexion_cpu, &tam_pagina, sizeof(int), NULL);
            break;

        default:
            
            liberar_conexion(arg_h->socket_cpu);
            log_destroy(logger_hilo);
            return;
        }

    }
}

void parsear_instrucciones(t_log* logger,t_proceso* proceso, char* str)
{
    if(str == NULL)
    {
        log_error(logger, "No hay instrucciones.");
        return NULL;
    }
    //t_list* instrucciones = list_create();
    char* str_cpy = strdup(str); ///*
    
    char* token = strtok(str_cpy, "\n");
    
    //log_info(logger, "Instruccion: %s", token);
    while(token != NULL)
    {
        //log_info(logger, "Instruccion: %s", token);
        if (strlen(token) > 0)
        {
            char* token_cpy = malloc(strlen(token) + 1); ///*
            memcpy(token_cpy, token,strlen(token));
            token_cpy[strlen(token_cpy)] = '\0';
            list_add(proceso->instrucciones, token);
            //log_info(logger, "Hice list_add de: %s", token);
            free(token_cpy);
        }
        token = strtok(NULL, "\n");
    }
    //free(token);
    //free(str);
    //free(str_cpy);
    //log_info(logger,"TOKEN NULL");
}

char* leer_pseudocodigo(t_log* logger, char* nombre_archivo)
{
    //log_info(logger, "leer_pseudocodigo.");
    char* ruta = malloc(strlen(nombre_archivo) + 15 + 1); ///*
    strcpy(ruta, "./pseudocodigo/");
    //log_info(logger, "char*.");
    strcat(ruta, nombre_archivo);
    //log_info(logger, "strcat.");
    //log_info(logger, "ruta: %s", ruta);    
    FILE* archivo = fopen(ruta, "r");
    
    //log_info(logger, "Se abrió el archivo.");
    if(archivo == NULL)
    {
        log_error(logger, "Error al abrir el archivo.");
        return NULL;
    }

    int size;
    char* pseudocodigo;

    fseek(archivo, 0, SEEK_END);
    size = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    pseudocodigo = malloc(size + 1); ///*

    if(pseudocodigo == NULL)
    {
        log_error(logger, "Error asignando memoria.");
        fclose(archivo);
    }

    fread(pseudocodigo, 1, size, archivo);
    pseudocodigo[size] = '\0';

    fclose(archivo);
    free(ruta);
    printf("%s", pseudocodigo);
    return pseudocodigo;

}

t_proceso* crear_proceso(uint32_t pid)
{
    t_proceso* proceso = malloc(sizeof(t_proceso));
    proceso->pid = pid;
    proceso->instrucciones = list_create();
    proceso->tabla_de_paginas = list_create();
    return proceso;
}

void conexion_kernel(void* arg)
{
    t_log* logger_hilo = iniciar_logger("logger_hilo.log","HILO");
    log_info(logger_hilo, "HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    t_list* bloques_swap;
    uint32_t cant_bloques_swap, nro_pagina, nro_frame;
    void* pagina;
    t_pagina* pagina_a_agregar;
    t_algoritmo_response* pagina_a_sacar;
    t_proceso* proceso_a_agregar;
    uint32_t pid, size;
    char* ruta;
    bool es_la_pagina(void* arg)
    {
        t_pagina* pagina = (t_pagina*)arg;
        return pagina->pagina == nro_pagina;
    }
    //log_info(logger_hilo,"Socket: %i", arg_h->socket_kernel);
    while(1)
    {
        op_code codigo = recibir_operacion(arg_h->socket_kernel);
        log_info(logger_hilo,"op_code: %i", codigo);
        switch (codigo)
        {
        case INICIAR_PROCESO:
            bloques_swap = list_create();
            recv(arg_h->socket_kernel, &pid, sizeof(uint32_t), MSG_WAITALL);
            t_proceso* proceso = crear_proceso(pid);
            //log_info(logger_hilo,"pid: %i", pid);
            ruta = recibir_mensaje(arg_h->socket_kernel);
            recv(arg_h->socket_kernel, &size, sizeof(uint32_t), MSG_WAITALL);
            //log_info(logger_hilo, "%s", ruta);
            parsear_instrucciones(logger_hilo, proceso, leer_pseudocodigo(logger_hilo, ruta));
            
            sem_wait(&mutex_lista_procesos);
            list_add(procesos_en_memoria, proceso);
            sem_post(&mutex_lista_procesos);
            sem_post(&cantidad_de_procesos);
            //log_info(logger_hilo, "SIGNAL cantidad_de_procesos");

            log_info(logger, "PID: %i - Tamaño: %i", pid, size);

            enviar_operacion(arg_h->socket_swap, RESERVAR_BLOQUES_SWAP);
            cant_bloques_swap = size / tam_pagina;
            send(arg_h->socket_swap, &cant_bloques_swap, sizeof(uint32_t), NULL);
            uint32_t* elemento;
            for(int i = 0; i < size / tam_pagina; i++)
            {
                elemento = malloc(sizeof(uint32_t));
                recv(arg_h->socket_swap, elemento, sizeof(uint32_t), MSG_WAITALL);
                printf("Recibo: %i\n", *elemento);
                list_add(bloques_swap, *elemento);
                //free(elemento);
            }

            asignar_memoria(pid, size, bloques_swap);
            list_destroy(bloques_swap);
            //free(cant_bloques_swap);
            //free(ruta);
            break;
        
        case FINALIZAR_PROCESO:
            recv(arg_h->socket_kernel, &pid, sizeof(uint32_t), MSG_WAITALL);
            //enviar_operacion(arg_h->socket_swap, LIBERAR_BLOQUES_SWAP);
            printf("Kernel me pid liberar el proceso %i\n", pid);
            finalizar_proceso(pid, arg_h->socket_swap);
            break; 

        case OP_PAGE_FAULT:
            recv(arg_h->socket_kernel, &pid, sizeof(uint32_t), MSG_WAITALL);
            recv(arg_h->socket_kernel, &nro_pagina, sizeof(uint32_t), MSG_WAITALL);
            printf("Voy a enviar\n");
            proceso_a_agregar = buscar_proceso(pid);
            pagina_a_agregar = list_find(proceso_a_agregar->tabla_de_paginas, es_la_pagina);

            nro_frame = buscar_frame_libre();

            if(nro_frame != -1)
            {
                printf("Voy a hacer SWAP_IN: Pid: %i - Página: %i\n", proceso_a_agregar->pid, pagina_a_agregar->pagina);
                bitarray_set_bit(frame_bitarray, nro_frame);
                swap_in(arg_h->socket_swap, pagina_a_agregar, nro_frame, proceso_a_agregar);
                pagina_a_agregar->timestamp_carga = time(NULL);
                pagina_a_agregar->timestamp_uso = pagina_a_agregar->timestamp_carga;
                pagina_a_agregar->presencia = 1;
                pagina_a_agregar->frame = nro_frame;
            }
            else
            {
                pagina_a_sacar = algoritmo();
                printf("Víctima = %i\n", pagina_a_sacar->pagina->pagina);
                printf("pagina_a_sacar->pagina->modificado = %i\n", pagina_a_sacar->pagina->modificado);
                if(pagina_a_sacar->pagina->modificado == 1)
                {
                    swap_out(arg_h->socket_swap, pagina_a_sacar->pagina, pagina_a_sacar->pagina->frame, pagina_a_sacar->proceso);
                    pagina_a_sacar->pagina->modificado = 0;
                    pagina_a_sacar->pagina->presencia = 0;
                    swap_in(arg_h->socket_swap, pagina_a_agregar, pagina_a_sacar->pagina->frame, proceso_a_agregar);
                    pagina_a_agregar->timestamp_carga = time(NULL);
                    pagina_a_agregar->timestamp_uso = pagina_a_agregar->timestamp_carga;
                    pagina_a_agregar->presencia = 1;
                    pagina_a_agregar->frame = pagina_a_sacar->pagina->frame;
                }
                else
                {
                    pagina_a_sacar->pagina->presencia = 0;
                    swap_in(arg_h->socket_swap, pagina_a_agregar, pagina_a_sacar->pagina->frame, proceso_a_agregar);
                    pagina_a_agregar->timestamp_carga = time(NULL);
                    pagina_a_agregar->timestamp_uso = pagina_a_agregar->timestamp_carga;
                    pagina_a_agregar->presencia = 1;
                    pagina_a_agregar->frame = pagina_a_sacar->pagina->frame; 
                }
                free(pagina_a_sacar);
            }

            
            enviar_respuesta(arg_h->socket_kernel, OK);
            printf("Envié\n");
            //free(pagina_a_agregar);
            
            break;

        default:
            liberar_conexion(arg_h->socket_kernel);
            log_destroy(logger_hilo);
            
            return;
        }

    }
    
}

void finalizar_proceso(uint32_t pid, int socket_swap)
{
    t_proceso* proceso = buscar_proceso(pid);
    sem_wait(&cantidad_de_procesos);
    printf("Socket = %i\n", socket_swap);
    void desasignar_paginas(t_pagina* pagina)
    {
        printf("Libero %i (p = %i)\n", pagina->pagina, pagina->presencia);
        if(pagina->presencia == 1)
        {
            bitarray_clean_bit(frame_bitarray, pagina->frame);
        }
        send(socket_swap, &(pagina->posicion_en_swap), sizeof(uint32_t), NULL);
        free(pagina);
    }
    void* obtener_bloques_swap(t_pagina* pagina)
    {
        return pagina->posicion_en_swap;
    }
    //void enviar_bloques(void* arg)
    //{
    //    send(socket_sw, arg, sizeof(uint32_t), NULL);
    //}
    //t_list* bloques_en_swap = list_map(proceso->tabla_de_paginas, obtener_bloques_swap);
    
    enviar_operacion(socket_swap, LIBERAR_BLOQUES_SWAP);
    send(socket_swap, &(proceso->tabla_de_paginas->elements_count), sizeof(proceso->tabla_de_paginas->elements_count), NULL);
    list_iterate(proceso->tabla_de_paginas, desasignar_paginas);
    //list_destroy_and_destroy_elements(proceso->instrucciones, free);
    list_destroy(proceso->instrucciones);
    //list_iterate(bloques_en_swap, enviar_bloques);
    
}