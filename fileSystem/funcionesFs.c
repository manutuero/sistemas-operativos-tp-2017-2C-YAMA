/*
 * funcionesFs.c
 *
 *  Created on: 17/9/2017
 *      Author: utnso
 */

#include "funcionesFs.h"

void cargarArchivoDeConfiguracion(char*nombreArchivo) {
	//log_info(vg_logger, "Cargando archivo de configuracion del FileSystem.");
	char      cwd[1024]; // Variable donde voy a guardar el path absoluto hasta el /Debug
	char *    pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)), nombreArchivo); // String que va a tener el path absoluto para pasarle al config_create
	t_config *config         = config_create(pathArchConfig);
	//log_info(vg_logger, "El directorio sobre el que se esta trabajando es %s.", pathArchConfig);

	if (config_has_property(config, "PUERTO")) {
			PUERTO = config_get_int_value(config, "PUERTO");
	}
	/*if (config_has_property(config, "IP_FILESYSTEM")) {
			IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	}*/
	/*if (config_has_property(config, "PUNTO_MONTAJE")) {
			PUNTO_MONTAJE = config_get_string_value(config, "PUNTO_MONTAJE");
	}*/

	printf("Puerto: %d\n", PUERTO);
	//printf("IP Filesystem: %s\n", IP_FILESYSTEM);
	//printf("Punto montaje: %s\n", PUNTO_MONTAJE);
	//log_info(vg_logger,"Archivo de configuracion cargado exitosamente");
}

void procesarComandoConsola(void *buffer,int sd){
	comando *comando=buffer;
	printf("Recibi el comando codigo= %d. Envio respuesta",comando->funcion);
	free(comando);
	//int enviarPorSocket(int socket, const void * mensaje, int tamanio)
	char * respuesta = "Comando recibido y ejecutado";
	enviarPorSocket(sd,respuesta,sizeof(respuesta));
}

void procesarMensaje(void * buffer, int sd ,header header){

	switch (header.id){
	case 1:
		procesarComandoConsola(buffer,sd);
		//printf("Recibi un comando de consola con id= %d",comando.funcion);
		break;
	default :
		perror("Recibi verdura y no entendi nada");
	break;
}



}
