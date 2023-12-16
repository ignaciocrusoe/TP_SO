#include "planificador_corto_plazo.h"

extern sem_t grado_de_multiprogramacion;
extern sem_t mutex_cola_new;
extern sem_t mutex_cola_ready;
extern sem_t mutex_cola_exit;
extern sem_t procesos_en_new;
extern sem_t procesos_en_ready;
extern sem_t procesos_en_exit;
extern sem_t planificacion_largo_plazo;
extern sem_t planificacion_corto_plazo;
extern sem_t mutex_file_management;
extern sem_t mutex_tabla_global_de_archivos;
  
extern t_queue *cola_ready;
extern t_queue *cola_exit;
extern t_queue *cola_new;
 
extern t_pcb* execute;
extern t_list* recursos_disponibles;
extern t_list* tabla_global_de_archivos;
t_motivo_desalojo motivo;
extern t_log* logger;

void planificador_rr(void* arg)
{
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    //log_info(logger, "Empieza el planificador fifo");
    pthread_t generador_de_interrupciones;
    while(1)
    {
        sem_wait(&procesos_en_ready);
        sem_wait(&planificacion_corto_plazo);
        sem_post(&planificacion_corto_plazo);
        //log_info(logger,"Hice wait del gdmp");
        sem_wait(&mutex_cola_ready);
        //log_info(logger,"Hice wait de la cola de new: %i",cola_new);

        execute = queue_pop(cola_ready);
        sem_post(&mutex_cola_ready);
        execute->estado = EXEC;

        log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", execute->pid);
  
        send(arg_h->socket_dispatch, &(execute->pid), sizeof(uint32_t), 0);
        //log_info(logger, "Envié %i a %i", execute->pid, arg_h->socket_dispatch);
        enviar_contexto_de_ejecucion(execute->contexto, arg_h->socket_dispatch);

        arg_h->pcb = execute;
        pthread_create(&generador_de_interrupciones, NULL, &clock_interrupt, (void*)arg_h);
        pthread_detach(&generador_de_interrupciones);

        execute->contexto = recibir_contexto_de_ejecucion(arg_h->socket_dispatch);
        motivo = recibir_motivo_desalojo(arg_h->socket_dispatch);
        evaluar_motivo_desalojo(logger, motivo, arg);
        

        
    }
}

void planificador_fifo(void* arg)
{
    t_log* logger = iniciar_logger("log_plani.log","HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    //log_info(logger, "Empieza el planificador fifo");
    while(1)
    {
        sem_wait(&procesos_en_ready);
        sem_wait(&planificacion_corto_plazo);
        sem_post(&planificacion_corto_plazo);
        log_info(logger,"Hice wait del gdmp");
        sem_wait(&mutex_cola_ready);
        //log_info(logger,"Hice wait de la cola de new: %i",cola_new);

        execute = queue_pop(cola_ready);
        sem_post(&mutex_cola_ready);
        execute->estado = EXEC;

        log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", execute->pid);
  
        send(arg_h->socket_dispatch, &(execute->pid), sizeof(uint32_t), 0);
        
        
        enviar_contexto_de_ejecucion(execute->contexto, arg_h->socket_dispatch);
        
        //log_info(logger, "Envié %i a %i", execute->pid, arg_h->socket_dispatch);
        
        execute->contexto = recibir_contexto_de_ejecucion(arg_h->socket_dispatch);
        motivo = recibir_motivo_desalojo(arg_h->socket_dispatch);
        evaluar_motivo_desalojo(logger, motivo, arg);
        
    }
}

void planificador_prioridades(void* arg)
{
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    //log_info(logger, "Empieza el planificador por prioridades");
    op_code operacion;
    char* recurso;
    while(1)
    {
        sem_wait(&procesos_en_ready);
        sem_wait(&planificacion_corto_plazo);
        sem_post(&planificacion_corto_plazo);
        //log_info(logger,"Hice wait del gdmp");
        sem_wait(&mutex_cola_ready);
        //log_info(logger,"Hice wait de la cola de new: %i",cola_new);

        ordenar_colas_segun_prioridad(cola_ready);
        execute = queue_pop(cola_ready);
        sem_post(&mutex_cola_ready);
        execute->estado = EXEC;

        log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", execute->pid);

        send(arg_h->socket_dispatch, &(execute->pid), sizeof(uint32_t), 0);
        //log_info(logger, "Envié %i a %i", execute->pid, arg_h->socket_dispatch);
        enviar_contexto_de_ejecucion(execute->contexto, arg_h->socket_dispatch);

        execute->contexto = recibir_contexto_de_ejecucion(arg_h->socket_dispatch);
        motivo = recibir_motivo_desalojo(arg_h->socket_dispatch);
        evaluar_motivo_desalojo(logger, motivo, arg);

    }
}

void clock_interrupt(void* arg)
{
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    //log_info(logger_ci, "Empieza clock_interrupt");
    op_code operacion = INTERRUPT;
    sleep(arg_h->quantum / 1000);
    if (arg_h->pcb->estado == EXEC){
        //printf("Mando interrupcion por el proceso %i", arg_h->pcb->pid);
        //log_info(logger, "PID: %i, Desalojado por fin de Quantum”, arg_h->pcb->pid);
        send(arg_h->socket_interrupt, &operacion, sizeof(op_code), 0);
    }
}

void ordenar_colas_segun_prioridad(t_queue* queue)
{
    bool comparar_prioridad(void* arg1, void* arg2)
    {
        t_pcb* pcb1 = (t_pcb*) arg1;
        t_pcb* pcb2 = (t_pcb*) arg2;
        return (pcb1->prioridad <= pcb2->prioridad);
    }
    list_sort(queue->elements, (bool*)&comparar_prioridad);
}

char* de_t_motivo_a_string(t_motivo_desalojo motivo)
{
    switch (motivo)
    {
    case SUCCESS:
        return "SUCCESS";
        break;

    case INVALID_RESOURCE:
        return "INVALID_RESOURCE";
        break;

    case CLOCK_INTERRUPT:
        return "CLOCK_INTERRUPT";
        break;

    case INVALID_WRITE:
        return "INVALID_WRITE";
        break;

    default:
        return "OTRO";
        break;
    }
}

void evaluar_motivo_desalojo(t_log* logger, t_motivo_desalojo motivo, void* arg)
{
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    uint32_t tam_archivo;
    uint32_t pagina;
    uint32_t sleep_time;
    char* recurso;
    char* nombre_archivo;
    char* lock; //Podríamos usar un enum y traducirlo en CPU o en Kernel
    t_direccion_fisica* direccion;
    t_response respuesta;
    t_archivo* archivo;
    t_args_hilo_archivos* args_hilo;
    uint32_t puntero;
    t_args_hilo_archivos* argumentos_file_management;
    switch (motivo)
    {
        case SUCCESS:
            execute->estado = EXIT;
            sem_wait(&mutex_cola_exit);
            queue_push(cola_exit, execute);
            sem_post(&mutex_cola_exit);
            sem_post(&procesos_en_exit);
            log_info(logger, "Fin de proceso %i motivo %s (%i)", execute->pid, de_t_motivo_a_string(motivo));
            break;

        case CLOCK_INTERRUPT:
            execute->estado = READY;
            sem_wait(&mutex_cola_ready);
            queue_push(cola_ready, execute);
            sem_post(&mutex_cola_ready);
            sem_post(&procesos_en_ready);
            break;

        case WAIT:
            //printf("Me pidieron WAIT\n");
            recurso = recibir_mensaje(arg_h->socket_dispatch);
            //printf("Me pidieron WAIT de %s\n", recurso);
            wait_recurso(logger, recurso, arg_h->socket_dispatch);
            break;

        case SIGNAL:
            recurso = recibir_mensaje(arg_h->socket_dispatch);
            signal_recurso(logger, recurso, arg_h->socket_dispatch, execute);
            break;

        case F_OPEN:
            nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
            lock = recibir_mensaje(arg_h->socket_dispatch);
            printf("Me pidieron abrir de %s con lock = %i\n", nombre_archivo, de_string_a_t_lock(lock));
            //printf("lock = %s\n", lock);
            
            log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", execute->pid);
            pthread_t h_file_open;
            argumentos_file_management = crear_parametros(arg_h, nombre_archivo, logger);
            argumentos_file_management->lock = de_string_a_t_lock(lock);
            argumentos_file_management->execute = execute;
            //printf("execute = %i - aarg_h->pcb->pid = %i\n",execute->pid, arg_h->pcb->pid);
            pthread_create(&h_file_open, NULL, &file_open, (void*)argumentos_file_management);
            pthread_detach(h_file_open);
                
            break;

        case F_READ:
            printf("F_READ\n");
            nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
            direccion = recibir_direccion(arg_h->socket_dispatch);
            printf("F_OPEN - Mando a FS\n");
            log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", execute->pid);
            pthread_t h_file_read;
            argumentos_file_management = crear_parametros(arg_h, nombre_archivo, logger);
            argumentos_file_management->direccion = direccion;
            argumentos_file_management->execute = execute;
            printf("arg_h->socket_filesystem = %i\n", arg_h->socket_filesystem);
            pthread_create(&h_file_read, NULL, &file_read, (void*)argumentos_file_management);
            pthread_detach(h_file_read);

            break;

        case F_WRITE:
            printf("---F_WRITE---\n");
            nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
            direccion = recibir_direccion(arg_h->socket_dispatch);
            log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", execute->pid);
            pthread_t h_file_write;
            argumentos_file_management = crear_parametros(arg_h, nombre_archivo, logger);
            argumentos_file_management->direccion = direccion;
            argumentos_file_management->execute = execute;
            printf("Direccion = %i:%i\n", direccion->frame, direccion->offset);
            pthread_create(&h_file_write, NULL, &file_write, (void*)argumentos_file_management);
            pthread_detach(h_file_write);

            break;

        case F_SEEK: //Crear un hilo
            //sem_wait(&mutex_file_management);
            nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
            recv(arg_h->socket_dispatch, &puntero, sizeof(uint32_t), MSG_WAITALL);

            pthread_t h_file_seek;
            argumentos_file_management = crear_parametros(arg_h, nombre_archivo, logger);
            argumentos_file_management->tam_archivo = tam_archivo;
            argumentos_file_management->execute = execute;
            argumentos_file_management->puntero = puntero;

            pthread_create(&h_file_seek, NULL, &file_seek, (void*)argumentos_file_management);
            pthread_detach(h_file_seek);

            break;

        case F_TRUNCATE:
        printf("ME LLEGO UN F_TRUCNATE\n");
            nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
            recv(arg_h->socket_dispatch, &tam_archivo, sizeof(uint32_t), MSG_WAITALL);
            log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", execute->pid);
            pthread_t h_file_truncate;  
            argumentos_file_management = crear_parametros(arg_h, nombre_archivo, logger);
            argumentos_file_management->tam_archivo = tam_archivo;
            argumentos_file_management->execute = execute;
            argumentos_file_management->puntero = puntero;
            pthread_create(&h_file_truncate, NULL, &file_truncate, (void*)argumentos_file_management);
            pthread_detach(h_file_truncate);
            printf("pthread_detach(h_file_truncate);\n");

            break;

        case F_CLOSE:
            nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
            printf("pid= %i - ip= %i\n", execute->pid, execute->contexto->PC);
            argumentos_file_management = crear_parametros(arg_h, nombre_archivo, logger);
            argumentos_file_management->execute = execute;
            pthread_create(&h_file_truncate, NULL, &file_close, (void*)argumentos_file_management);
            pthread_detach(h_file_truncate);
     
            break;

        case PAGE_FAULT:
            recv(arg_h->socket_dispatch, &pagina, sizeof(uint32_t), MSG_WAITALL);

            pthread_t h_page_fault;
            //char* null_string = "NULL";
            args_hilo = crear_parametros(arg_h, "", logger);
            args_hilo->execute = execute;
            args_hilo->pagina = pagina;
            pthread_create(&h_page_fault, NULL, &atender_page_fault, (void*)args_hilo);
            pthread_detach(h_page_fault);

            //Pedirle a memoria que cargue la página del proceso

            break;

        case SLEEP:
            recv(arg_h->socket_dispatch, &sleep_time, sizeof(uint32_t), MSG_WAITALL);
            log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", execute->pid);
            log_info(logger, "PID: %i- Bloqueado por: SLEEP", execute->pid);
            execute->estado = BLOCKED;
            pthread_t h_sleep;
            args_hilo = crear_parametros(arg_h, "", logger);
            args_hilo->execute = execute;
            args_hilo->sleep_time = sleep_time;
            pthread_create(&h_sleep, NULL, &sleep_function, (void*)args_hilo);
            pthread_detach(h_sleep);
            break;

        case KILL:
            break;
        default:
            break;
    }
    int temp1, temp2, temp3;
    sem_getvalue(&mutex_cola_ready, &temp1);
    sem_getvalue(&mutex_file_management, &temp2);
    sem_getvalue(&procesos_en_ready, &temp3);
    printf("mutex_cola_ready = %i\n", temp1);
    printf("mutex_file_management = %i\n", temp2);
    printf("procesos_en_ready = %i\n", temp3);

}