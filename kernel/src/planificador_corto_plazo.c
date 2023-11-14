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
        char* recurso = recibir_mensaje(arg_h->socket_dispatch);
        printf("Me pidieron WAIT de %s\n", recurso);
        wait_recurso(logger_hilo, recurso, arg_h->socket_dispatch);
        break;
    case SIGNAL:
        recurso = recibir_mensaje(arg_h->socket_dispatch);
        signal_recurso(logger_hilo, recurso, arg_h->socket_dispatch);
        break;
    case F_OPEN:
        enviar_operacion(arg_h->socket_filesystem, F_OPEN);
        break;
    
    default:
        break;
    }
}