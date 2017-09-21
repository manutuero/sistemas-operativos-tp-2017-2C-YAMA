#ifndef FUNCIONESFS_H_
#define FUNCIONESFS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <commons/config.h>
#include <commons/string.h>
#include <string.h>
#include "utils/utils.h"

#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define MAX_PACKAGE_SIZE 1024	//El servidor no admitira paquetes de mas de 1024 bytes
#define MAXUSERNAME 30
#define MAX_MESSAGE_SIZE 300
#define TRUE 1

typedef struct {
	int funcion;
	int opcion;
	char *parametro1;
	char *parametro2;
	int bloque;
	int idNodo;
} t_comando;

char *ARCHCONFIG;

void cargarArchivoDeConfiguracion(char*);
void levantarConsola();
char* ejecutarFuncionFormat(t_comando*);
char* ejecutarFuncionRm(t_comando*);
char* ejecutarFuncionRmDirectory(t_comando*);
char* ejecutarFuncionRmBloque(t_comando*);
char* ejecutarFuncionCat(t_comando*);
char* ejecutarFuncionMkdir(t_comando*);
char* ejecutarFuncionMd5(t_comando*);
char* ejecutarFuncionLs(t_comando*);
char* ejecutarFuncionInfo(t_comando*);
char* ejecutarFuncionRename(t_comando*);
char* ejecutarFuncionMv(t_comando*);
char* ejecutarFuncionCpfrom(t_comando*);
char* ejecutarFuncionCpto(t_comando*);
char* ejecutarFuncionCpblok(t_comando*);

#endif
