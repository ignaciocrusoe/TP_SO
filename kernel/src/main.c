#include "main.h"

sem_t grado_de_multiprogramacion;
sem_t mutex_cola_new;
sem_t mutex_cola_ready;
sem_t mutex_cola_exit;
sem_t procesos_en_new;
sem_t procesos_en_exit;
sem_t procesos_en_ready;
sem_t planificacion_largo_plazo;
sem_t planificacion_largo_plazo_exit;
sem_t planificacion_corto_plazo;
sem_t mutex_file_management;
sem_t mutex_tabla_global_de_archivos;

t_queue *cola_new;
t_queue *cola_ready;
t_queue *cola_exit;

t_pcb* execute;
t_list* recursos_disponibles;
t_list* tabla_global_de_archivos;
t_log* logger;

uint8_t planificacion_iniciada;

char* ip_filesystem;
char* puerto_filesystem;
char* algoritmo_planificacion;

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

void iniciar_planificacion()
{
    log_info(logger, "Iniciar");
    if(!planificacion_iniciada)
    {
        sem_post(&planificacion_largo_plazo);
        printf("OK!\n");
        sem_post(&planificacion_largo_plazo_exit);
        printf("OK!\n");
        sem_post(&planificacion_corto_plazo);
        printf("OK!\n");
        planificacion_iniciada = 1;
        log_info(logger, "Se inició");
    }
}
void detener_planificacion()
{
    log_info(logger, "Detener");
    if(planificacion_iniciada)
    {
        sem_wait(&planificacion_largo_plazo);
        printf("OK!\n");
        sem_wait(&planificacion_largo_plazo_exit);
        printf("OK!\n");
        sem_wait(&planificacion_corto_plazo);
        printf("OK!\n");
        planificacion_iniciada = 0;
    }
    log_info(logger, "Se detuvo");
}

int main(int argc, char* argv[])
{  
    logger = iniciar_logger("log_kernel","Kernel");
    t_config* config = iniciar_config("./cfg/kernel.config");
    
    char* ip_cpu = config_get_string_value(config, "IP_CPU");
    char* puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    char* puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
    puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
    char* grado_max_de_multiprogramacion = config_get_string_value(config, "GRADO_MULTIPROGRAMACION_INI");
    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    char** recursos = config_get_array_value(config, "RECURSOS");
    char** instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
    int quantum = config_get_int_value(config, "QUANTUM");

    int conexion_cpu_dispatch = crear_conexion(logger, ip_cpu, puerto_cpu_dispatch);
    int conexion_cpu_interrupt = crear_conexion(logger, ip_cpu, puerto_cpu_interrupt);
    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    //int conexion_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);

    sem_init(&mutex_cola_new, 0, 1);
    sem_init(&mutex_cola_ready, 0, 1);
    sem_init(&mutex_cola_exit, 0, 1);
    sem_init(&procesos_en_new, 0, 0);
    sem_init(&procesos_en_ready, 0, 0);
    sem_init(&procesos_en_exit, 0, 0);
    sem_init(&grado_de_multiprogramacion, 0, atoi(grado_max_de_multiprogramacion));
    sem_init(&planificacion_largo_plazo, 0, 0);
    sem_init(&planificacion_largo_plazo_exit, 0, 0);
    sem_init(&planificacion_corto_plazo, 0, 0);
    sem_init(&mutex_file_management, 0, 1);
    sem_init(&mutex_tabla_global_de_archivos, 0, 1);

    cola_new = queue_create();
	cola_ready = queue_create();
	cola_exit = queue_create();
    tabla_global_de_archivos = list_create();

    recursos_disponibles = iniciar_lista_de_recursos(recursos, instancias_recursos);
    planificacion_iniciada = 0;
 
    t_args_hilo args_hilo;
    args_hilo.socket_dispatch = conexion_cpu_dispatch;
    args_hilo.socket_interrupt = conexion_cpu_interrupt;
    args_hilo.socket_memoria = conexion_memoria;
    //args_hilo.socket_filesystem = conexion_filesystem;
    args_hilo.quantum = quantum; 
    
    pthread_t hilo_planificador_de_largo_plazo;
    pthread_create(&hilo_planificador_de_largo_plazo, NULL, &planificador_largo_plazo, (void*)&args_hilo);
    pthread_detach(&hilo_planificador_de_largo_plazo);

    pthread_t hilo_planificador_de_largo_plazo_exit;
    pthread_create(&hilo_planificador_de_largo_plazo_exit, NULL, &finalizar_procesos_en_exit, (void*)&args_hilo);
    pthread_detach(&hilo_planificador_de_largo_plazo_exit);

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
        //printf(" Parametros %i",i);

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
            

            //send a memoria para liberar espacio
            finalizar_proceso (atoi(c_argv[1]), conexion_cpu_dispatch, conexion_memoria, &args_hilo, conexion_cpu_interrupt);
            //send(arg_h->socket_memoria, &(pcb->pid), sizeof(int), 0); //mandamos el pid 
            //liberar_recursos_archivos(pcb);
            
        }
        else if(!strcmp(c_argv[0], "DETENER_PLANIFICACION"))
        {
            //sem_getvalue(&planificacion_largo_plazo,&sem_value_lp);
            //sem_getvalue(&planificacion_corto_plazo,&sem_value_cp);
            //if(!sem_value_lp) sem_wait(&planificacion_largo_plazo);
            //if(!sem_value_cp) sem_wait(&planificacion_corto_plazo);
            detener_planificacion();
        }
        else if(!strcmp(c_argv[0], "INICIAR_PLANIFICACION"))
        {
            //sem_getvalue(&planificacion_largo_plazo,&sem_value_lp);
            //sem_getvalue(&planificacion_corto_plazo,&sem_value_cp);
            //if(!sem_value_lp) sem_post(&planificacion_largo_plazo);
            //if(!sem_value_cp) sem_post(&planificacion_corto_plazo);
            iniciar_planificacion();
        }
        else if(!strcmp(c_argv[0], "MULTIPROGRAMACION"))
        {
            //solo seria cambiar el valor del semaforo?     
        }
        else if(!strcmp(c_argv[0], "PROCESO_ESTADO"))
        {   
            imprimir_procesos_por_estado();
            
        }
        else if(!strcmp(c_argv[0], "RECURSOS_DISPONIBLES"))
        {   
            imprimir_recursos();
            
        }
        else if(!strcmp(c_argv[0], "INTERRUPT"))
        {
            op_code operacion = INTERRUPT;
            send(conexion_cpu_interrupt, &operacion, sizeof(op_code), 0);
        }
        /*----------------------------------------------------*/
        //Código temporal para probar interaccion instrucciones
        else if(!strcmp(c_argv[0], "FS")) {
            //ejecutarSecuencia(conexion_filesystem);
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

void imprimir_recursos()
{
    void imprimir_recurso(void* arg)
    {
        printf("REC: %s - Instancias: %i\n", ((t_recurso*)arg)->nombre, ((t_recurso*)arg)->instancias);
    }
    printf("RECURSOS\n");
    list_iterate(recursos_disponibles, imprimir_recurso);
}

void imprimir_procesos_por_estado()
{
    void imprimir_proceso(void* arg)
    {
        printf("PID: %i - Archivo de pseudocódigo: %s\n", ((t_pcb*)arg)->pid, ((t_pcb*)arg)->archivo_de_pseudocodigo);
    }
            //MOSTRAMOS LOS PROCESOS EN NEW 
            printf("Estado: NEW\n");
            list_iterate(cola_new->elements, imprimir_proceso);
            
            //MOSTRAMOS LOS PROCESOS EN READY
            printf("Estado: READY\n");
            list_iterate(cola_ready->elements, imprimir_proceso);
            
            //PRIMERO LOS BLOQUEADOS POR RECURSOS
            printf("Estado: BLOQUEADOS - Motivo: Recursos\n");
            for(int i = 0; i < list_size(recursos_disponibles); i++)
            {
                t_recurso* recurso = list_get(recursos_disponibles, i);
                list_iterate(recurso->cola_blocked->elements, imprimir_proceso);
            }

            //SEGUNDO LOS BLOQUEADOS POR ARCHIVOS
            printf("Estado: BLOQUEADOS - Motivo: Archivos\n");
            for(int i = 0; i < list_size(tabla_global_de_archivos); i++)
            {
                t_archivo* archivo = list_get(tabla_global_de_archivos, i);
                list_iterate(cola_new->elements, imprimir_proceso);
            }

            //MOSTRAMOS LOS PROCESOS EN EXIT 
            printf("Estado: EXIT\n");
            list_iterate(cola_exit->elements, imprimir_proceso);
}



t_queue* buscar_proceso_en_cola_bloqueados(t_list* recursos_disponibles, t_list* tabla_global_de_archivos, uint32_t pid)
{

    t_pcb* pcb = NULL;
    t_list_iterator* recursos_disponibles_iterator = list_iterator_create(recursos_disponibles);
    t_recurso* recurso = NULL;

    while(list_iterator_has_next(recursos_disponibles_iterator))
    {
        recurso = list_iterator_next(recursos_disponibles_iterator);
        pcb = buscar_proceso_segun_pid(pid, recurso->cola_blocked);
        if(pcb != NULL)
        {
            return recurso->cola_blocked;
        }
    }

    if(pcb == NULL)
    {   
        t_archivo* archivo = NULL;
        t_list_iterator* tabla_global_de_archivos_iterator = list_iterator_create(tabla_global_de_archivos);

        while(list_iterator_has_next(tabla_global_de_archivos_iterator))
        {
            
            t_archivo* archivo = list_iterator_next(tabla_global_de_archivos_iterator);
            pcb = buscar_proceso_segun_pid(pid, archivo->cola_blocked);
            if(pcb != NULL)
            {
                return archivo->cola_blocked;
            }
        }
    }

}
/*
void liberar_recursos_archivos(t_pcb* pcb, int socket_filesystem)
{
    void hacer_f_close(void* arg)
    {
        pthread_t h_file_close_deallocate;
        t_args_hilo_archivos* argumentos_file_management = malloc(sizeof(t_args_hilo_archivos));
        argumentos_file_management->execute = pcb;
        argumentos_file_management->nombre_archivo = (((t_archivo*)arg)->nombre);
        argumentos_file_management->socket_filesystem = socket_filesystem;
        pthread_create(&h_file_close_deallocate, NULL, &file_close, (void*)argumentos_file_management);
        pthread_detach(h_file_close_deallocate);
    }
    list_iterate(pcb->tabla_de_archivos_abiertos, hacer_f_close);
}
*/
void finalizar_proceso (uint32_t pid, int socket_cpu_dispatch, int socket_memoria, void* arg_h, int socket_cpu_interrupt)
{
    t_pcb* pcb;
    
    void hacer_f_close(void* arg)
    {
        pthread_t h_file_close_deallocate;
        t_args_hilo_archivos* argumentos_file_management = crear_parametros(arg_h, ((t_archivo_local*)arg)->archivo->nombre, logger);
        //argumentos_file_management->socket_filesystem = socket_filesystem;
        argumentos_file_management->execute = pcb;
        printf("pcb->pid =  %i\n", argumentos_file_management->execute->pid);
        pthread_create(&h_file_close_deallocate, NULL, &cerrar_archivo, (void*)argumentos_file_management);
        pthread_detach(h_file_close_deallocate);
        //cerrar_archivo(argumentos_file_management);
    }
    void hacer_signal(void* arg)
    {
        t_recurso* recurso_local = (t_recurso*)arg;
        t_recurso* recurso = buscar_recurso(((t_recurso*)arg)->nombre);
        if(recurso->cola_blocked == obtener_queue(pcb->pid))
        {
            recurso->instancias++;
            //desbloquear_procesos(recurso->nombre);
        }
        for(uint32_t i = recurso_local->instancias; i > 0; i--)
        {
            recurso->instancias++;
            desbloquear_procesos(recurso->nombre);
        }
        printf("Libero %i instancias del recurso %s\n", recurso_local->instancias, recurso_local->nombre);
        recurso_local->instancias = 0;
        //signal_recurso(logger, ((t_recurso*)arg)->nombre, socket_cpu_dispatch, pcb);
    }
    t_queue* cola = obtener_queue(pid);
    if(execute->pid == pid)
    {
        pcb = execute;        //recibir_contexto_de_ejecucion(socket_cpu_dispatch);
        //t_motivo_desalojo motivo = recibir_motivo_desalojo(socket_cpu_dispatch);
        list_iterate(pcb->recursos_asignados, hacer_signal);
        list_iterate(pcb->tabla_de_archivos_abiertos, hacer_f_close);
        printf("Mando la interrupcion\n");
        enviar_operacion(socket_cpu_interrupt, FINALIZAR_PROCESO);
        printf("Mandé la interrupcion\n");
        //evaluar_motivo_desalojo(logger, motivo, arg)
    }
    else if(cola != NULL)
    {
        pcb = buscar_proceso_segun_pid(pid, cola);
        //list_iterate(pcb->tabla_de_archivos_abiertos, hacer_f_close);
        list_iterate(pcb->recursos_asignados, hacer_signal);
        list_iterate(pcb->tabla_de_archivos_abiertos, hacer_f_close);
        list_remove_element(cola->elements, pcb);
    }
    enviar_operacion(socket_memoria, FINALIZAR_PROCESO);
    send(socket_memoria, &(pcb->pid), sizeof(uint32_t), NULL);
}

t_queue* obtener_queue(uint32_t pid)
{
    bool tiene_mismo_pid(void* pcb) {
        return (((t_pcb*)pcb)->pid == pid);
    }
    int a;
    if(list_find(cola_new->elements, tiene_mismo_pid) != NULL)
    {
        return cola_new;
    }
    else if(list_find(cola_ready->elements, tiene_mismo_pid) != NULL)
    {
        return cola_ready;
    }
    else if(list_find(cola_exit->elements, tiene_mismo_pid) != NULL)
    {
        return cola_exit;
    }
    else
    {
        return buscar_proceso_en_cola_bloqueados(recursos_disponibles, tabla_global_de_archivos, pid);
    }
}
