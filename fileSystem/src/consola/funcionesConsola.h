#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_
#include "../utils.h"


typedef struct {
	int funcion;
	int opcion;
	char *parametro1;
	char *parametro2;
	int bloque;
	int idNodo;
} t_comando;

//Firmas de funciones
int cantidadArgumentos(char**);
void inicializarComando(t_comando*);
int validarParametro(char*);
char** cargarArgumentos(char*);
char* invocarFuncionFormat(char**);
char* invocarFuncionRm(char**);
char* invocarFuncionRmDirectory(char**);
char* invocarFuncionRmBloque(char**);
char* invocarFuncionCat(char**);
char* invocarFuncionMkdir(char**);
char* invocarFuncionMd5(char**);
char* invocarFuncionLs(char**);
char* invocarFuncionInfo(char**);
char* invocarFuncionRename(char**);
char* invocarFuncionMv(char**);
char* invocarFuncionCpfrom(char**);
char* invocarFuncionCpto(char**);
char* invocarFuncionCpblok(char**);
void* levantarConsola();
#endif
