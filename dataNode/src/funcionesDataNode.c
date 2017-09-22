/*
 * funcionesDataNode.c
 *
 *  Created on: 21/9/2017
 *      Author: utnso
 */

#include "funcionesDataNode.h"

void cargarArchivoConfiguracion(char*nombreArchivo){

	char      cwd[1024]; // Variable donde voy a guardar el path absoluto
	char *    pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)), nombreArchivo); // String que va a tener el path absoluto para pasarle al config_create
	t_config *config         = config_create(pathArchConfig);

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
	  //log_info(vg_logger,"Archivo de configuracion cargado exitosamente");
	}


