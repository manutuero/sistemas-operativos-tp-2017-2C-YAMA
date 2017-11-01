#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

#include <utils.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <ctype.h>
#include "../funcionesFileSystem.h"

/* Defines */
#define ESPACIO " "
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/* Firmas de funciones */
int cantidadArgumentos(char**);
void inicializarComando(t_comando*);
bool esValido(char *path);
char** cargarArgumentos(char*);
void  ejecutarFormat(char**);
char* invocarFuncionRm(char**);
char* invocarFuncionRmDirectory(char**);
char* invocarFuncionRmBloque(char**);
char* invocarFuncionCat(char**);
void  ejecutarMkdir(char **argumentos);
char* invocarFuncionMd5(char**);
void  ejecutarLs(char **argumentos);
void  ejecutarInfo(char **argumentos);
void  ejecutarRename(char **argumentos);
void  ejecutarMv(char **argumentos);
char* invocarFuncionCpfrom(char**);
char* invocarFuncionCpto(char**);
char* invocarFuncionCpblok(char**);
void* levantarConsola();
#endif
