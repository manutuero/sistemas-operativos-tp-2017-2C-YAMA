#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "funcionesConsola.h"

#define ESPACIO " "
#define IPDESTINO "127.0.0.1"
#define PUERTO 6667

int main(void) {
	comando instruccion;
	char *linea;
	char **argumentos;
	int cantElementos;
	int socket=-1;
	header header;
	int prueba=0;
	while(1){

	linea = (char*) readline("> ");
	if (!linea)
	break; // Si recibe NULL corta la ejecucion, evita un segmentation fault.

	add_history(linea);

	argumentos = cargarArgumentos(linea);
	instruccion.funcion=0;
	cantElementos = cantArgumentos(argumentos);
	switch(cantElementos){

		case 1:
			if(strcmp(argumentos[0],"format")==0)
				{
					instruccion=empaquetarFuncionFormat(argumentos);
					break;
				}
			if(strcmp(argumentos[0],"exit")==0)
				{
					printf("Que tengas un buen dia\n");
					break;
				}
			else
				printf("el comando ingresado no es valido\n");
		break;

		case 2:

			if(strcmp(argumentos[0],"rm")==0)
				{
					instruccion=empaquetarFuncionRm(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"cat")==0)
				{
					instruccion=empaquetarFuncionCat(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"mkdir")==0)
				{
					instruccion=empaquetarFuncionMkdir(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"MD5")==0)
				{
					instruccion=empaquetarFuncionMd5(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"ls")==0)
				{
					instruccion=empaquetarFuncionLs(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"info")==0)
				{
					instruccion=empaquetarFuncionInfo(argumentos);
					break;
				}
			else
				{
					printf("El comando %s no es valido",argumentos[0]);
				}

			break;

		case 3:

			if(strcmp(argumentos[0],"rename")==0)
				{
					instruccion=empaquetarFuncionRename(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"rm")==0)
				{
					instruccion=empaquetarFuncionRmDirectory(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"mv")==0)
				{
					instruccion=empaquetarFuncionMv(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"cpfrom")==0)
				{
					instruccion=empaquetarFuncionCpfrom(argumentos);
					break;
				}

			if(strcmp(argumentos[0],"cpto")==0)
				{
					instruccion=empaquetarFuncionCpto(argumentos);
					break;
				}
			else
				{
					printf("El comando %s no es valido",argumentos[0]);
					break;
				}
		case 4:
			if(strcmp(argumentos[0],"cpblock")==0)
				{
					instruccion=empaquetarFuncionCpblok(argumentos);
					break;
				}
			else
				{
					printf("El comando %s no es valido",argumentos[0]);
					break;
				}
		case 5:
			if(strcmp(argumentos[0],"rm") == 0)
				{
					instruccion=empaquetarFuncionRmBloque(argumentos);
					break;
				}
			else
				{
					printf("El comando %s no es valido",argumentos[0]);
					break;
				}
		default:
			printf("El comando ingresado no es valido\n");
			break;
	}

	if(argumentos[0] != NULL)
		if(strcmp(argumentos[0], "exit") == 0)
			break;
	if(instruccion.funcion != 0){
		socket = nuevoSocket();
		if(conectarSocket(socket, IPDESTINO, PUERTO)>=0){
			header.id = 1;
			void* payload = serializarComandoConsola(&instruccion, &header);
			enviarPaquete(socket, payload, header);
			cerrarSocket(socket);
		} else{
			perror("No se conecto al socket");
		}
	}
	}

	return 0;
}
