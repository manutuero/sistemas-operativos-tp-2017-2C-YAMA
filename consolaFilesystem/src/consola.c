/*
 ============================================================================
 Name        : test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "funcionesConsola.h"

#define ESPACIO " "



int main(void) {
	comando instruccion;
	char *linea;
	char **argumentos;
	int cantElementos;
	int socket;

	while(1){

	linea = (char*) readline("> ");
	if (!linea)
	break; // Si recibe NULL corta la ejecucion, evita un segmentation fault.

	add_history(linea);

	argumentos = cargarArgumentos(linea);

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

			if(strcmp(argumentos[0],"rm")==0)
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

	}

	if(argumentos[0]!=NULL)
		if(strcmp(argumentos[0],"exit")==0)
			break;


	if(instruccion.funcion!=0)
		printf("utils.h");

	//	socket = newSocket();

	//	conectarSocket(socket, const char * ipDestino, int puerto);

	//	enviarPaquete(socket, instruccion, 1);



	}

	return 0;
}
