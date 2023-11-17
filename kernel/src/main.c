#include "main.h"

sem_t grado_de_multiprogramacion;
sem_t mutex_cola_new;
sem_t mutex_cola_ready;
sem_t mutex_cola_exit;
sem_t procesos_en_new;
sem_t procesos_en_ready;
sem_t planificacion_largo_plazo;
sem_t planificacion_corto_plazo;

t_queue *cola_new;
t_queue *cola_ready;
t_queue *cola_exit;

t_pcb* execute;
t_list* recursos_disponibles;
t_list* tabla_global_de_archivos;

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

void* liberar_t_recurso()
{

}
/*
void liberar_recursos(t_pcb* pcb)
{
    bool buscar_recurso(void* arg)
    {
        t_recurso* elemento = (t_recurso*) arg;
        return (!strcmp(elemento->nombre, elemento->nombre));
    }

    int i = 0;
    int cantidad_de_recursos = list_size(pcb->recursos_asignados);

    while(i < cantidad_de_recursos){


        t_recurso* recurso = list_remove(pcb->recursos_asignados, i);
        t_recurso* recurso_lista_disponibles = (t_recurso*) list_find(recursos_disponibles, buscar_recurso);
        recurso_lista_disponibles->instancias += recurso->instancias;
        //me falta liberar todo
    }
    
}
*/

t_list* iniciar_lista_de_recursos(char** a_recursos, char** a_instancias)
{
    t_list* recursos = list_create();
    int i = 0;
    do
    {
        t_recurso* recurso = crear_recurso(a_recursos[i], atoi(a_instancias[i]));
        printf("Recurso agregado: %s - %i\n", recurso->nombre, recurso->instancias);
        i++;
        list_add(recursos, recurso);
    } while (a_recursos[i] != NULL);

    return recursos;   
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
    char** instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
    int quantum = config_get_int_value(config, "QUANTUM");

    int conexion_cpu_dispatch = crear_conexion(logger, ip_cpu, puerto_cpu_dispatch);
    int conexion_cpu_interrupt = crear_conexion(logger, ip_cpu, puerto_cpu_interrupt);
    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    int conexion_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);

    sem_init(&mutex_cola_new, 0, 1);
    sem_init(&mutex_cola_ready, 0, 1);
    sem_init(&mutex_cola_exit, 0, 1);
    sem_init(&procesos_en_new, 0, 0);
    sem_init(&procesos_en_ready, 0, 0);
    sem_init(&grado_de_multiprogramacion, 0, atoi(grado_max_de_multiprogramacion));
    sem_init(&planificacion_largo_plazo, 0, 0);
    sem_init(&planificacion_corto_plazo, 0, 0);

    cola_new = queue_create();
	cola_ready = queue_create();
	cola_exit = queue_create();
    tabla_global_de_archivos = list_create();

    recursos_disponibles = iniciar_lista_de_recursos(recursos, instancias_recursos);
    t_recurso* recurso_prueba = list_get(recursos_disponibles, 0);  //---------------//
    printf("recurso = %s\n", recurso_prueba->nombre);               //----REVISAR----//
    recurso_prueba = list_get(recursos_disponibles, 1);             //---------------//
    printf("recurso = %s\n", recurso_prueba->nombre);
    recurso_prueba = list_get(recursos_disponibles, 2);
    printf("recurso = %s\n", recurso_prueba->nombre);
 
    t_args_hilo args_hilo;
    args_hilo.socket_dispatch = conexion_cpu_dispatch;
    args_hilo.socket_interrupt = conexion_cpu_interrupt;
    args_hilo.socket_memoria = conexion_memoria;
    args_hilo.socket_filesystem = conexion_filesystem;
    args_hilo.quantum = quantum; 
    
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
                pcb->size = atoi(c_argv[2]);
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
            if(execute->pid == c_argv[1])
            {
                enviar_operacion(conexion_cpu_interrupt, FINALIZAR_PROCESO);
            }
            else
            {

                t_list_iterator* recursos = list_iterator_create(recursos_disponibles);
                while(list_iterator_has_next(recursos))
                {
                    t_recurso* recurso = list_iterator_next(recursos);
                    pcb = buscar_proceso_segun_pid(c_argv[1], recurso->cola_blocked);
                    if(pcb != NULL)
                    {
                        break;
                    }
                }
                if(buscar_proceso_segun_pid(c_argv[1], cola_ready) != NULL)
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

            enviar_operacion(conexion_memoria, FINALIZAR_PROCESO);
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
            //solo seria cambiar el valor del semaforo?     
        }
        else if(!strcmp(c_argv[0], "PROCESO_ESTADO"))
        {   
            /*
            //mostramos los pid de NEW 
            t_list* lista_para_iterar = list_iterator_create(cola_new);
            char* pids = string_new();
            while(list_iterator_has_next(lista_para_iterar))
            {
                t_pcb* proceso = list_iterator_next(lista_para_iterar);
                strcat(pids,string_itoa(proceso->pid));
                strcat(pids, ", ");
            }
            char* log = string_new();
            strcat(log, "Estado: NEW - Procesos: ");
            strcat(log, pids);
            log_info(logger, log);

            //mostramos los pids de READY

            lista_para_iterar = list_iterator_create(cola_ready);
            pids = string_new();
            while(list_iterator_has_next(lista_para_iterar))
            {
                t_pcb* proceso = list_iterator_next(lista_para_iterar);
                strcat(pids,string_itoa(proceso->pid));
                strcat(pids, ", ");
            }
            char* log = string_new();
            strcat(log, "Estado: READY - Procesos: ");
            strcat(log, pids);
            log_info(logger, log);

            //mostramos los pids de bloqueado 

            lista_para_iterar = list_iterator_create(cola);
            pids = string_new();
            while(list_iterator_has_next(lista_para_iterar))
            {
                t_pcb* proceso = list_iterator_next(lista_para_iterar);
                strcat(pids,string_itoa(proceso->pid));
                strcat(pids, ", ");
            }
            char* log = string_new();
            strcat(log, "Estado: BLOCKED - Procesos: ");
            strcat(log, pids);
            log_info(logger, log);

            //mostramos el pid de running

            log_info(logger, "Estado: RUNNING - PID: %i", execute->pid);
        */
        }
        else if(!strcmp(c_argv[0], "INTERRUPT"))
        {
            op_code operacion = INTERRUPT;
            send(conexion_cpu_interrupt, &operacion, sizeof(op_code), 0);
        }
        /*---------------------------------------------------*/
        //Código temporal para probar interaccion con filesystem
        else if(!strcmp(c_argv[0], "OPERAR_FILESYSTEM")) {
            operacionesFilesSystem(conexion_filesystem);
        }
        /*----------------------------------------------------*/
        //Código temporal para probar interaccion instrucciones
        else if(!strcmp(c_argv[0], "INSTR_FILE")) {
            operacionesInstrucciones(conexion_filesystem);
        }
        /*----------------------------------------------------*/
        else
        {
            log_warning(logger, "La función %s no existe.", c_argv[0]);
        }
    }

    log_destroy(logger);
    config_destroy(config);
    liberar_conexion(conexion_cpu_dispatch);
    
}