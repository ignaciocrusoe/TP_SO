#include "planificador_corto_plazo.h"

extern sem_t grado_de_multiprogramacion;
extern sem_t mutex_cola_new;
extern sem_t mutex_cola_ready;
extern sem_t mutex_cola_exit;
extern sem_t procesos_en_new;
extern sem_t procesos_en_ready;
extern sem_t planificacion_largo_plazo;
extern sem_t planificacion_corto_plazo;
  
extern t_queue *cola_ready;
extern t_queue *cola_exit;
extern t_queue *cola_new;
 
extern t_pcb* execute;
extern t_list* recursos_disponibles;
extern t_list* tabla_global_de_archivos;
t_motivo_desalojo motivo;

void planificador_rr(void* arg)
{
    t_log* logger_hilo = iniciar_logger("log_plani.log","HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    log_info(logger_hilo, "Empieza el planificador fifo");
    pthread_t generador_de_interrupciones;
    while(1)
    {
        sem_wait(&planificacion_corto_plazo);
        sem_wait(&procesos_en_ready);
        log_info(logger_hilo,"Hice wait del gdmp");
        sem_wait(&mutex_cola_ready);
        log_info(logger_hilo,"Hice wait de la cola de new: %i",cola_new);

        execute = queue_pop(cola_ready);
        sem_post(&mutex_cola_ready);
        execute->estado = EXEC;

        log_info(logger_hilo, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", execute->pid);
  
        send(arg_h->socket_dispatch, &(execute->pid), sizeof(uint32_t), 0);
        log_info(logger_hilo, "Envié %i a %i", execute->pid, arg_h->socket_dispatch);
        enviar_contexto_de_ejecucion(execute->contexto, arg_h->socket_dispatch);

        arg_h->pcb = execute;
        pthread_create(&generador_de_interrupciones, NULL, &clock_interrupt, (void*)arg_h);
        pthread_detach(&generador_de_interrupciones);

        execute->contexto = recibir_contexto_de_ejecucion(arg_h->socket_dispatch);
        motivo = recibir_motivo_desalojo(arg_h->socket_dispatch);
        evaluar_motivo_desalojo(logger_hilo, motivo, arg);
        
        sem_post(&planificacion_corto_plazo);
    }
}

void planificador_fifo(void* arg)
{
    t_log* logger_hilo = iniciar_logger("log_plani.log","HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    log_info(logger_hilo, "Empieza el planificador fifo");
    while(1)
    {
        sem_wait(&planificacion_corto_plazo);
        sem_wait(&procesos_en_ready);
        log_info(logger_hilo,"Hice wait del gdmp");
        sem_wait(&mutex_cola_ready);
        log_info(logger_hilo,"Hice wait de la cola de new: %i",cola_new);

        execute = queue_pop(cola_ready);
        sem_post(&mutex_cola_ready);
        execute->estado = EXEC;

        log_info(logger_hilo, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", execute->pid);
  
        send(arg_h->socket_dispatch, &(execute->pid), sizeof(uint32_t), 0);
        
        
        enviar_contexto_de_ejecucion(execute->contexto, arg_h->socket_dispatch);
        
        log_info(logger_hilo, "Envié %i a %i", execute->pid, arg_h->socket_dispatch);
        
        execute->contexto = recibir_contexto_de_ejecucion(arg_h->socket_dispatch);
        motivo = recibir_motivo_desalojo(arg_h->socket_dispatch);
        evaluar_motivo_desalojo(logger_hilo, motivo, arg);
        
        sem_post(&planificacion_corto_plazo);
    }
}

void planificador_prioridades(void* arg)
{
    t_log* logger_hilo = iniciar_logger("log_plani.log","HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    log_info(logger_hilo, "Empieza el planificador por prioridades");
    op_code operacion;
    char* recurso;
    while(1)
    {
        sem_wait(&planificacion_corto_plazo);
        sem_wait(&procesos_en_ready);
        log_info(logger_hilo,"Hice wait del gdmp");
        sem_wait(&mutex_cola_ready);
        log_info(logger_hilo,"Hice wait de la cola de new: %i",cola_new);

        ordenar_colas_segun_prioridad(cola_ready);
        execute = queue_pop(cola_ready);
        sem_post(&mutex_cola_ready);
        execute->estado = EXEC;

        log_info(logger_hilo, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", execute->pid);

        send(arg_h->socket_dispatch, &(execute->pid), sizeof(uint32_t), 0);
        log_info(logger_hilo, "Envié %i a %i", execute->pid, arg_h->socket_dispatch);
        enviar_contexto_de_ejecucion(execute->contexto, arg_h->socket_dispatch);

        execute->contexto = recibir_contexto_de_ejecucion(arg_h->socket_dispatch);
        motivo = recibir_motivo_desalojo(arg_h->socket_dispatch);
        evaluar_motivo_desalojo(logger_hilo, motivo, arg);

        sem_post(&planificacion_corto_plazo);
    }
}

void clock_interrupt(void* arg)
{
    t_log* logger_hilo_ci = iniciar_logger("log_plani.log","HILO");
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    log_info(logger_hilo_ci, "Empieza clock_interrupt");
    op_code operacion = INTERRUPT;
    sleep(arg_h->quantum / 1000);
    if (arg_h->pcb->estado == EXEC){
        printf("Mando interrupcion por el proceso %i", arg_h->pcb->pid);
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

void evaluar_motivo_desalojo(t_log* logger_hilo, t_motivo_desalojo motivo, void* arg)
{
    t_args_hilo* arg_h = (t_args_hilo*) arg;
    int32_t tam_archivo;
    char* recurso, direccion;
    char* nombre_archivo;
    char* lock; //Podríamos usar un enum y traducirlo en CPU o en Kernel
    t_response respuesta;
    t_archivo* archivo;
    switch (motivo)
    {
    case SUCCESS:
        execute->estado = EXIT;
        sem_wait(&mutex_cola_exit);
        queue_push(cola_exit, execute);
        sem_post(&mutex_cola_exit);
        log_info(logger_hilo, "Fin de proceso %i motivo %s (%i)", execute->pid, de_t_motivo_a_string(motivo));
        break;

    case CLOCK_INTERRUPT:
        execute->estado = READY;
        sem_wait(&mutex_cola_ready);
        queue_push(cola_ready, execute);
        sem_post(&mutex_cola_ready);
        sem_post(&procesos_en_ready);
        break;

    case WAIT:
        printf("Me pidieron WAIT\n");
        recurso = recibir_mensaje(arg_h->socket_dispatch);
        printf("Me pidieron WAIT de %s\n", recurso);
        wait_recurso(logger_hilo, recurso, arg_h->socket_dispatch);
        break;
    case SIGNAL:
        recurso = recibir_mensaje(arg_h->socket_dispatch);
        signal_recurso(logger_hilo, recurso, arg_h->socket_dispatch);
        break;
    case F_OPEN:
        nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
        printf("Me pidieron abrir de %s\n", nombre_archivo);
        lock = recibir_mensaje(arg_h->socket_dispatch);
        printf("lock = %s\n", lock);
        archivo = crear_archivo(nombre_archivo, tam_archivo, de_string_a_t_lock(lock));
        list_add(tabla_global_de_archivos, archivo);
        list_add(execute->tabla_de_archivos_abiertos, archivo);

        enviar_operacion(arg_h->socket_filesystem, ABRIR_ARCHIVO);
        enviar_mensaje(nombre_archivo, arg_h->socket_filesystem);
        recv(arg_h->socket_filesystem, &tam_archivo, sizeof(int32_t), MSG_WAITALL);
        if(tam_archivo != -1)
        {
            printf("El archivo tiene un tamaño de %i bytes\n", tam_archivo);

            sem_wait(&mutex_cola_ready);
            queue_push(cola_ready, execute); // Debería ser en la primera posición.
            sem_post(&mutex_cola_ready);
            sem_post(&procesos_en_ready);
        }
        else
        {
            printf("El archivo no existe\n");
            //Se le pide a Filesystem que cree el archivo
            enviar_operacion(arg_h->socket_filesystem, CREAR_ARCHIVO);
            enviar_mensaje(nombre_archivo, arg_h->socket_filesystem);
            //Podríamos recibir un OK, de hecho creo que hay que recibirlo
            respuesta = recibir_respuesta(arg_h->socket_filesystem);
            switch (respuesta)
            {
            case OK:
                sem_wait(&mutex_cola_ready);
                queue_push(cola_ready, execute); // Debería ser en la primera posición.
                sem_post(&mutex_cola_ready);
                sem_post(&procesos_en_ready);
                break;
            
            default:
                break;
            }
        }
        break;
    
    case F_READ:
        printf("F_READ\n");
        nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
        direccion = recibir_direccion(arg_h->socket_cpu);
        printf("F_OPEN - Mando a FS\n");
        enviar_operacion(arg_h->socket_filesystem, LEER_ARCHIVO);
        enviar_mensaje(nombre_archivo, arg_h->socket_filesystem);

        enviar_direccion(arg_h->socket_memoria, direccion);
        send(arg_h->socket_filesystem, &(archivo->puntero), sizeof(uint32_t),0); //falto buscar el archivo 
        recv(arg_h->socket_filesystem, &tam_archivo, sizeof(int32_t), MSG_WAITALL);
        
        break;

        case F_WRITE:
        printf("F_WRITE\n");
        nombre_archivo = recibir_mensaje(arg_h->socket_dispatch);
        direccion = recibir_direccion(arg_h->socket_cpu);
        t_archivo* archivo = buscar_archivo_segun_nombre(nombre_archivo, execute->tabla_de_archivos_abiertos);
        if(archivo->lock == WRITE)
        {
             printf("F_OPEN - Mando a FS\n");
        enviar_operacion(arg_h->socket_filesystem, ESCRIBIR_ARCHIVO);
        enviar_mensaje(nombre_archivo, arg_h->socket_filesystem);

        enviar_direccion(arg_h->socket_memoria, direccion);
        send(arg_h->socket_filesystem, &(archivo->puntero), sizeof(uint32_t),0); //falto buscar el archivo 
        recv(arg_h->socket_filesystem, &tam_archivo, sizeof(int32_t), MSG_WAITALL);
        }
        else
        {
            //falta mandar a exit al proceso 
            //falta que los planificadores pongan en ready al proceso una vez recibido el OK de FS 
        }
        break;

    default:
        break;
    }
}