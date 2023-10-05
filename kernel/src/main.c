
#include "../include/main.h"

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

    int conexion_cpu_dispatch = crear_conexion(logger, ip_cpu, puerto_cpu_dispatch);
    int conexion_cpu_interrupt = crear_conexion(logger, ip_cpu, puerto_cpu_interrupt);
    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    int conexion_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);
    /*
    t_mensaje mensaje;
    mensaje.mensaje = strdup("mensaje_de_prueba");
    mensaje.size_mensaje = 17 + 1;
    enviar_mensaje(mensaje ,conexion_cpu_dispatch);
*/
    while(1){
        t_mensaje mensaje;
        char* leido = readline("> ");
        add_history(leido);
        log_info(logger,leido);
        char* token = strtok(leido, " ");
        //Deberíamos mandar un paquete con los parámetros serializados.
        //Por ahora mandamos un mensaje para la función, y uno por parámetro.
        mensaje.mensaje = token;
        mensaje.size_mensaje = strlen(token) + 1;
        enviar_mensaje(mensaje.mensaje ,conexion_cpu_dispatch);
        token = strtok(NULL, " ");
        while(token != NULL){
            mensaje.mensaje = token;
            mensaje.size_mensaje = strlen(token) + 1;
            enviar_mensaje(mensaje.mensaje ,conexion_cpu_dispatch);
            token = strtok(NULL, " ");
        }
    }

    //enviar_mensaje("Mensaje", conexion_cpu);

    log_destroy(logger);
    config_destroy(config);
    liberar_conexion(conexion_cpu_dispatch);
    
}

