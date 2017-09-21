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

  printf("\nPuerto: %d\n", PUERTO);
  //log_info(vg_logger,"Archivo de configuracion cargado exitosamente");
}


char* ejecutarFuncionFormat(t_comando *comando) {
	return "";
}

char* ejecutarFuncionRm(t_comando *comando) {

	return "";
}

char* ejecutarFuncionRmDirectory(t_comando *comando) {

	return "";
}

char* ejecutarFuncionRmBloque(t_comando *comando) {

	return "";
}

char* ejecutarFuncionCat(t_comando *comando) {

	return "";
}

char* ejecutarFuncionMkdir(t_comando *comando) {

	return "";
}

char* ejecutarFuncionMd5(t_comando *comando) {

	return "";
}

char* ejecutarFuncionLs(t_comando *comando) {

	return "";
}

char* ejecutarFuncionInfo(t_comando *comando) {

	return "";
}

char* ejecutarFuncionRename(t_comando *comando) {

	return "";
}

char* ejecutarFuncionMv(t_comando *comando) {

	return "";
}

char* ejecutarFuncionCpfrom(t_comando *comando) {

	return "";
}

char* ejecutarFuncionCpto(t_comando *comando) {

	return "";
}

char* ejecutarFuncionCpblok(t_comando *comando) {

	return "";
}
