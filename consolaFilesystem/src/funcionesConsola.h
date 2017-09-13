/*
 * funciones_consola.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

typedef struct{
	int funcion;
	int opcion;
	char *parametro1;
	char *parametro2;
	int bloque;
	int idNodo;
}comando;

int cantArgumentos(char **argumentos);

comando empaquetarFuncionFormat(char **componentes);

comando empaquetarFuncionRm(char **componentes);

comando empaquetarFuncionRmDirectory(char **componentes);

comando empaquetarFuncionRmBloque(char **componentes);

comando empaquetarFuncionCat(char **componentes);

comando empaquetarFuncionMkdir(char **componentes);

comando empaquetarFuncionMd5(char **componentes);

comando empaquetarFuncionLs(char **componentes);

comando empaquetarFuncionInfo(char **componentes);

comando empaquetarFuncionRename(char **componentes);

comando empaquetarFuncionMv(char **componentes);

comando empaquetarFuncionCpfrom(char **componentes);

comando empaquetarFuncionCpto(char **componentes);

comando empaquetarFuncionCpblok(char **componentes);

int validarParametro(char * parametro);

char **cargarArgumentos(char* linea);

#endif /* FUNCIONESCONSOLA_H_ */
