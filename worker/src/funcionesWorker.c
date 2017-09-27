/*
 * funcionesWorker.c

 *
 *  Created on: 13/9/2017
 *      Author: utnso
 */

#include "utils.h"
#include "funcionesWorker.h"


void crearLogger() {
   char *pathLogger = string_new();

   char cwd[1024];

   string_append(&pathLogger, getcwd(cwd, sizeof(cwd)));

   string_append(&pathLogger, "/workerLogs.log");

   char *logWorkerFileName = strdup("workerLogs.log");

   workerLogger = log_create(pathLogger, logWorkerFileName, false, LOG_LEVEL_INFO);

   free(logWorkerFileName);
   logWorkerFileName = NULL;
}
void cargarArchivoConfiguracion(char*nombreArchivo){
	log_info(workerLogger, "Cargando archivo de configuracion del FileSystem.");
	char      cwd[1024]; // Variable donde voy a guardar el path absoluto
	char *    pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)), nombreArchivo); // String que va a tener el path absoluto para pasarle al config_create
	t_config *config         = config_create(pathArchConfig);
	log_info(workerLogger, "El directorio sobre el que se esta trabajando es %s.", pathArchConfig);
	  if (config_has_property(config, "PUERTO_FILESYSTEM")) {
	      PUERTO_FILESYSTEM = config_get_int_value(config, "PUERTO_FILESYSTEM");
	  }
	  if (config_has_property(config, "IP_FILESYSTEM")) {
	      IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	  }
	  if (config_has_property(config, "ID_NODO")) {
	  	  ID_NODO = config_get_string_value(config, "ID_NODO");
	  }
	  if (config_has_property(config, "PUERTO_WORKER")) {
	  	  PUERTO_WORKER = config_get_int_value(config, "PUERTO_WORKER");
	  	  }
	  if (config_has_property(config, "RUTA_DATABIN")) {
	  	  RUTA_DATABIN= config_get_string_value(config, "RUTA_DATABIN");
	  }

	  printf("\nIP Filesystem: %s\n", IP_FILESYSTEM);
	  printf("\nPuerto Filesystem: %d\n", PUERTO_FILESYSTEM);
	  printf("\nID Nodo %s\n", ID_NODO);
	  printf("\nPuerto Worker %d\n",PUERTO_WORKER);
	  printf("\nRuta Data.bin %s\n",RUTA_DATABIN);
	  log_info(workerLogger,"Archivo de configuracion cargado exitosamente");

	//config_destroy(config);  //si se descomenta esta linea, se reduce la cantidad de memory leaks
	}

int recibirArchivo(int cliente){

	FILE* file;

	struct stat buf;
	if(stat ("/home/utnso/Escritorio/archivoSalida",&buf)==0){
		remove("/home/utnso/Escritorio/archivoSalida");
	}

	file = fopen("/home/utnso/Escritorio/archivoSalida","wb");

	//int file = open("/home/utnso/Escritorio/archivoSalida", O_WRONLY | O_CREAT  | O_TRUNC);

	t_archivo* archivo;
	void *buffer;
	t_header header;

	int bytesRecibidos = recibirHeader(cliente, &header);
	buffer = malloc(header.tamanio+1);
	bytesRecibidos = recibirPorSocket(cliente,buffer,header.tamanio);
	//Se podria hacer RecibirPaquete, dependiendo si la deserializacion va a utils o no.
	archivo = (t_archivo*) deserializarArchivo(buffer,header.tamanio);

	printf("Al worker le llego un archivo de %d bytes: \n%s\n",archivo->tamanio, archivo->contenido);
	//write(file, archivo->contenido, archivo->tamanio);
	//close(file);

	fwrite(archivo->contenido, archivo->tamanio,1,file);
	fclose(file);

	free(archivo->contenido);
	free(archivo);
	free(buffer);
	return bytesRecibidos;
}

t_archivo* deserializarArchivo(void *buffer, int tamanio){
	t_archivo *miArchivo = malloc(sizeof(t_archivo));

	int desplazamiento = 0;
	memcpy(&miArchivo->tamanio, buffer, sizeof(miArchivo->tamanio));
	desplazamiento += sizeof(miArchivo->tamanio);

	miArchivo->contenido = malloc(miArchivo->tamanio);
	memcpy(miArchivo->contenido, buffer+desplazamiento, miArchivo->tamanio);

	//free(miArchivo->contenido);
	return miArchivo;

}
