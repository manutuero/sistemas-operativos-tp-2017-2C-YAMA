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

  printf("\nPuerto: %d\n", PUERTO);
  //printf("IP Filesystem: %s\n", IP_FILESYSTEM);
  //printf("Punto montaje: %s\n", PUNTO_MONTAJE);
  //log_info(vg_logger,"Archivo de configuracion cargado exitosamente");
}

char *ejecutarFuncionFormat(comando *unComando){
	return "";
}

char * ejecutarFuncionRm(comando *unComando){

	return "";
}

char * ejecutarFuncionRmDirectory(comando *unComando){

	return "";
}

char * ejecutarFuncionRmBloque(comando *unComando){

	return "";
}

char * ejecutarFuncionCat(comando *unComando){

	return "";
}

char * ejecutarFuncionMkdir(comando *unComando){

	return "";
}

char * ejecutarFuncionMd5(comando *unComando){

	return "";
}

char * ejecutarFuncionLs(comando *unComando){

	return "";
}

char * ejecutarFuncionInfo(comando *unComando){

	return "";
}

char * ejecutarFuncionRename(comando *unComando){

	return "";
}

char * ejecutarFuncionMv(comando *unComando){

	return "";
}

char * ejecutarFuncionCpfrom(comando *unComando){

	return "";
}

char * ejecutarFuncionCpto(comando *unComando){

	return "";
}

char * ejecutarFuncionCpblok(comando *unComando){

	return "";
}



void procesarComandoConsola(void *buffer,int sd){
 comando *unComando=(comando*)buffer;
 printf("Recibi el comando codigo= %d. Envio respuesta \n",unComando->funcion);
 char * respuesta="";
 switch(unComando->funcion){
 case 1:
	 respuesta=ejecutarFuncionFormat(unComando);
	 break;
 case 2:
	 respuesta=ejecutarFuncionRm(unComando);
	 break;
 case 3:
	 respuesta=ejecutarFuncionRmDirectory(unComando);
	 break;
 case 4:
	 respuesta=ejecutarFuncionRmBloque(unComando);
	 break;
 case 5:
	 respuesta=ejecutarFuncionCat(unComando);
	 break;
 case 6:
	 respuesta=ejecutarFuncionMkdir(unComando);
	 break;
 case 7:
	 respuesta=ejecutarFuncionMd5(unComando);
	 break;
 case 8:
	 respuesta=ejecutarFuncionLs(unComando);
	 break;
 case 9:
	 respuesta=ejecutarFuncionInfo(unComando);
	 break;
 case 10:
	 respuesta=ejecutarFuncionRename(unComando);
	 break;
 case 11:
	 respuesta=ejecutarFuncionMv(unComando);
	 break;
 case 12:
	 respuesta=ejecutarFuncionCpfrom(unComando);
	 break;
 case 13:
	 respuesta=ejecutarFuncionCpto(unComando);
	 break;
 case 14:
	 respuesta=ejecutarFuncionCpblok(unComando);
	 break;
 default:
	 respuesta=strcat("Error en la consola. No se reconocio el codigo de comando enviado:",(char*)unComando->funcion);
	 break;

	 //Enviar respuesta por socket al cliente. Veremos como.
 }



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
