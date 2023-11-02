#include "main.h"

sem_t grado_de_multiprogramacion;
sem_t mutex_cola_new;
sem_t mutex_cola_ready;
sem_t mutex_cola_blocked;
sem_t procesos_en_new;
sem_t procesos_en_ready;
sem_t planificacion_largo_plazo;
sem_t planificacion_corto_plazo;
sem_t start_interrupts;
sem_t mutex_flag_p_finished;


t_queue *cola_ready;
t_queue *cola_blocked;
t_queue *cola_exit;
t_queue *cola_new;

t_pcb* execute;
t_list* recursos_disponibles;

int p_finished;

uint32_t* instancias_recursos(char** instancias)
{
    uint32_t* rec_instancias;
    int i = 0;
    do{

        rec_instancias[i] = instancias[i];
        
    }while(instancias[i] != NULL);

    return rec_instancias;
}


void recibir_ordenes_cpu(void* arg)
{
    
}


t_pcb* buscar_proceso_segun_pid(uint32_t pid, t_queue* cola)
{

    bool tiene_mismo_pid(void* pcb) {
        return (((t_pcb*)pcb)->pid == pid);
    }

    t_pcb* pcb = NULL;
    pcb = list_find(cola, tiene_mismo_pid);
    return pcb;
}

int main(int argc, char* argv[]){
   
    t_log* logger = iniciar_logger("log_kernel","Kernel");
    t_config* config = iniciar_config("./cfg/kernel.config");
    
    char* ip_cpu = config_get_string_value(config, "IP_CPU");
	char* puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    char* puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
	char* puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
    char* grado_max_de_multiprogramacion = config_get_string_value(config, "GRADO_MULTIPROGRAMACION_INI");
    char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    char** recursos = config_get_array_value(config, "RECURSOS");
    char** intancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");


    int conexion_cpu_dispatch = crear_conexion(logger, ip_cpu, puerto_cpu_dispatch);
    int conexion_cpu_interrupt = crear_conexion(logger, ip_cpu, puerto_cpu_interrupt);
    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    int conexion_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);

    sem_init(&mutex_cola_new, 0, 1);
    sem_init(&mutex_cola_ready, 0, 1);
    sem_init(&procesos_en_new, 0, 0);
    sem_init(&procesos_en_ready, 0, 0);
    sem_init(&grado_de_multiprogramacion, 0, atoi(grado_max_de_multiprogramacion));
    sem_init(&planificacion_largo_plazo, 0, 0);
    sem_init(&planificacion_corto_plazo, 0, 0);

    cola_new = queue_create();
	cola_ready = queue_create();
	cola_blocked = queue_create();
	cola_exit = queue_create();

    recursos_disponibles = list_create();

    t_args_hilo args_hilo;
    args_hilo.socket_dispatch = conexion_cpu_dispatch;
    args_hilo.socket_interrupt = conexion_cpu_interrupt;
    args_hilo.socket_memoria = conexion_memoria;
    
    pthread_t hilo_planificador_de_largo_plazo;
    pthread_create(&hilo_planificador_de_largo_plazo, NULL, &planificador_largo_plazo, (void*)&args_hilo);
    pthread_detach(&hilo_planificador_de_largo_plazo);

    pthread_t hilo_planificador_de_corto_plazo;

    void (*planificador_corto_plazo) (void*) = NULL;
    if(!strcmp(algoritmo_planificacion, "FIFO"))
    {
        planificador_corto_plazo = &planificador_fifo;
    }
    else if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
    {
        planificador_corto_plazo = &planificador_prioridades;
    }
    else if(!strcmp(algoritmo_planificacion, "RR"))
    {
        planificador_corto_plazo = &planificador_rr;
    }

    pthread_create(&hilo_planificador_de_corto_plazo, NULL, planificador_corto_plazo, (void*)&args_hilo);
    pthread_detach(&hilo_planificador_de_corto_plazo);
    log_info(logger,"Creé los hilos");

    int sem_value_lp, sem_value_cp;

    while(1){
        t_mensaje mensaje;
        fflush(stdin);
        char* leido = readline("> ");
        add_history(leido);
        char* token = strtok(leido, " ");
        char* c_argv[4]; //Va a haber como máximo 4 tokens.
        int i = 0;
        c_argv[i] = strdup(token);
        token = strtok(NULL, " ");


        while(token != NULL && i < 4)
        {
            i++;
            if(i < 4){
                c_argv[i] = strdup(token);
            }
            token = strtok(NULL, " ");
        }
        printf(" Parametros %i",i);

        if(!strcmp(c_argv[0], "INICIAR_PROCESO"))
        {
            /*INICIAR_PROCESO [PATH] [SIZE] [PRIORIDAD]*/
            if (i < 3)
            {
                log_warning(logger, "Se pasaron parámetros de menos");
            }
            else if (i > 3)
            {
                log_warning(logger, "Se pasaron parámetros de más");
            }
            else{
                t_pcb* pcb = crear_pcb(atoi(c_argv[3]), c_argv[1]);
                sem_wait(&mutex_cola_new);
                //log_info(logger,"Hice wait");
                queue_push(cola_new, pcb);
                //log_info(logger,"Hice push");
                sem_post(&mutex_cola_new);
                sem_post(&procesos_en_new);

                log_info(logger, "Se crea el proceso %i en NEW", pcb->pid);
            }
        }
        else if(!strcmp(c_argv[0], "FINALIZAR_PROCESO"))
        {
            t_pcb* pcb;
            //buscar el proceso (primero fijarse si esta ejecutando, segundo en la lista de blocked y tercero ready...)
            if(execute->pid == c_argv[1]){
                op_code operacion = FINALIZAR_PROCESO;
                send(conexion_cpu_interrupt, &operacion, sizeof(op_code), 0); 
            }else
            {
                if(buscar_proceso_segun_pid(c_argv[1], cola_blocked) != NULL){
                    pcb = buscar_proceso_segun_pid(c_argv[1], cola_blocked);
                    list_remove_element(cola_blocked, pcb);
                   
                }
                else if(buscar_proceso_segun_pid(c_argv[1], cola_ready) != NULL)
                {
                    pcb = buscar_proceso_segun_pid(c_argv[1], cola_ready);
                    list_remove_element(cola_ready, pcb);

                }
                else if(buscar_proceso_segun_pid(c_argv[1], cola_new) != NULL)
                {
                    pcb = buscar_proceso_segun_pid(c_argv[1], cola_new);
                    list_remove_element(cola_new, pcb);

                }
                else
                {
                    log_error(logger, "No se encontro el proceso");
                }
                
            }

            //send a memoria para liberar espacio
            //liberar_recursos(pcb);
            
        }
        else if(!strcmp(c_argv[0], "DETENER_PLANIFICACION"))
        {
            //sem_getvalue(&planificacion_largo_plazo,&sem_value_lp);
            //sem_getvalue(&planificacion_corto_plazo,&sem_value_cp);
            //if(!sem_value_lp) sem_wait(&planificacion_largo_plazo);
            //if(!sem_value_cp) sem_wait(&planificacion_corto_plazo);
            log_info(logger, "Detener");
            sem_wait(&planificacion_largo_plazo);
            sem_wait(&planificacion_corto_plazo);
            log_info(logger, "Se detuvo");

        }
        else if(!strcmp(c_argv[0], "INICIAR_PLANIFICACION"))
        {
            //sem_getvalue(&planificacion_largo_plazo,&sem_value_lp);
            //sem_getvalue(&planificacion_corto_plazo,&sem_value_cp);
            //if(!sem_value_lp) sem_post(&planificacion_largo_plazo);
            //if(!sem_value_cp) sem_post(&planificacion_corto_plazo);
            log_info(logger, "Iniciar");
            sem_post(&planificacion_largo_plazo);
            sem_post(&planificacion_corto_plazo);
            log_info(logger, "Se inició");
        
        }
        else if(!strcmp(c_argv[0], "MULTIPROGRAMACION"))
        {

        }
        else if(!strcmp(c_argv[0], "PROCESO_ESTADO"))
        {   

        }
        else if(!strcmp(c_argv[0], "INTERRUPT"))
        {
            op_code operacion = INTERRUPT;
            send(conexion_cpu_interrupt, &operacion, sizeof(op_code), 0);
        }
        else
        {
            log_warning(logger, "La función %s no existe.", c_argv[0]);
        }
    }

    log_destroy(logger);
    config_destroy(config);
    liberar_conexion(conexion_cpu_dispatch);
    
}


