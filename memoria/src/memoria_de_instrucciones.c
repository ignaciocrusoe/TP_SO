#include "memoria_de_instrucciones.h"

extern t_list* procesos_en_memoria;
extern char* saveptr;
extern sem_t mutex_lista_procesos;
extern sem_t cantidad_de_procesos;
extern int tam_pagina;

void conexion_cpu(void* arg)
{
    t_log* logger_hilo = iniciar_logger("logger_hilo_conec_cpu.log","HILO_CPU");
    log_info(logger_hilo, "HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    log_info(logger_hilo,"Socket: %i", arg_h->socket_cpu);
    t_direccion_fisica* direccion;
    //enviar_mensaje("LISTO_PARA_RECIBIR_PEDIDOS",arg_h->socket_cpu);
    while(1)
    {
        op_code codigo = recibir_operacion(arg_h->socket_cpu);
        log_info(logger_hilo,"op_code: %i", codigo);
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
            log_info(logger_hilo,"HAY QUE ENVIAR LA INSTRUCCION");
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
            printf("RECIBI PAGINA\n");
            int32_t marco = obtener_numero_de_marco(pid, pagina_buscada);
            enviar_frame(arg_h->socket_cpu, marco);
            break;

        case PEDIDO_LECTURA:
            direccion = recibir_direccion(arg_h->socket_cpu);
            uint32_t lectura = leer_de_memoria(direccion);
            send(arg_h->socket_cpu, &lectura, sizeof(uint32_t), NULL);
            break;

        case PEDIDO_ESCRITURA:
            uint32_t a_escribir;
            direccion = recibir_direccion(arg_h->socket_cpu);
            recv(arg_h->socket_cpu, &a_escribir, sizeof(uint32_t), MSG_WAITALL);
            printf("Voy a escribir en memoria\n");
            escribir_en_memoria(direccion, a_escribir);
            //send(arg_h->socket_cpu, &direccion, sizeof(uint32_t), NULL);
            break;

        case PEDIDO_SIZE_PAGINA:
            send(conexion_cpu, &tam_pagina, sizeof(int), NULL);
            break;

        case FINALIZAR_PROCESO:
        
            break;

        default:
            liberar_conexion(arg_h->socket_cpu);
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
    char* str_cpy = strdup(str);
    char* token = strtok(str_cpy, "\n");
    //log_info(logger, "Instruccion: %s", token);
    while(token != NULL)
    {
        log_info(logger, "Instruccion: %s", token);
        if (strlen(token) > 0)
        {
            char* token_cpy = malloc(strlen(token));
            memcpy(token_cpy, token,strlen(token));
            token_cpy[strlen(token_cpy)] = '\0';
            list_add(proceso->instrucciones, token);
            log_info(logger, "Hice list_add de: %s", token);
            free(token_cpy);
        }
        token = strtok(NULL, "\n");
    }
    log_info(logger,"TOKEN NULL");
}

char* leer_pseudocodigo(t_log* logger, char* nombre_archivo)
{
    log_info(logger, "leer_pseudocodigo.");
    char* ruta = malloc(strlen(nombre_archivo) + 15);
    strcpy(ruta, "./pseudocodigo/");
    log_info(logger, "char*.");
    strcat(ruta, nombre_archivo);
    log_info(logger, "strcat.");
    log_info(logger, "ruta: %s", ruta);    
    FILE* archivo = fopen(ruta, "r");
    log_info(logger, "Se abrió el archivo.");
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

    pseudocodigo = malloc(size + 1);

    if(pseudocodigo == NULL)
    {
        log_error(logger, "Error asignando memoria.");
        fclose(archivo);
    }

    fread(pseudocodigo, 1, size, archivo);
    pseudocodigo[size] = '\0';

    fclose(archivo);

    printf("%s", pseudocodigo);
    return pseudocodigo;

}

t_proceso* crear_proceso(uint32_t pid)
{
    t_proceso* proceso = malloc(sizeof(t_proceso));
    proceso->pid = pid;
    proceso->instrucciones = list_create();
    proceso->tabla_de_paginas = malloc(sizeof(t_pagina));
    return proceso;
}

void conexion_kernel(void* arg)
{
    t_log* logger_hilo = iniciar_logger("logger_hilo.log","HILO");
    log_info(logger_hilo, "HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    log_info(logger_hilo,"Socket: %i", arg_h->socket_kernel);
    while(1)
    {
        op_code codigo = recibir_operacion(arg_h->socket_kernel);
        log_info(logger_hilo,"op_code: %i", codigo);
        switch (codigo)
        {
        case INICIAR_PROCESO:
            uint32_t pid, size;
            recv(arg_h->socket_kernel, &pid, sizeof(uint32_t), MSG_WAITALL);
            t_proceso* proceso = crear_proceso(pid);
            log_info(logger_hilo,"pid: %i", pid);
            char* ruta = recibir_mensaje(arg_h->socket_kernel);
            recv(arg_h->socket_kernel, &size, sizeof(uint32_t), MSG_WAITALL);
            log_info(logger_hilo, "%s", ruta);
            parsear_instrucciones(logger_hilo, proceso, leer_pseudocodigo(logger_hilo, ruta));
            sem_wait(&mutex_lista_procesos);
            list_add(procesos_en_memoria, proceso);
            sem_post(&mutex_lista_procesos);
            sem_post(&cantidad_de_procesos);
            log_info(logger_hilo, "SIGNAL cantidad_de_procesos");

            log_info(logger_hilo, "Se asignó %i bytes al proceso %i", size, pid);

            uint32_t (*algoritmo)(void);
            algoritmo = reemplazo_fifo;
            asignar_memoria(pid, size, algoritmo);

            break;
        
        default:
            liberar_conexion(arg_h->socket_kernel);
            return;
        }

    }
    
}
