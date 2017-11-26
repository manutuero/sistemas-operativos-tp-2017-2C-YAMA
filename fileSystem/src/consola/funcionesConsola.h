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
void  ejecutarFormat();
void  ejecutarRmArchivo(char **argumentos);
void  ejecutarRmDirectorio(char**);
void  ejecutarRmBloque(char **argumentos);
void  ejecutarCat(char **argumentos);
void  ejecutarMkdir(char **argumentos);
void  ejecutarMd5(char **argumentos);
void  ejecutarLs(char **argumentos);
void  ejecutarInfo(char **argumentos);
void  ejecutarRename(char **argumentos);
void  ejecutarMv(char **argumentos);
void  ejecutarCpfrom(char **argumentos);
void  ejecutarCpto(char **argumentos);
void  ejecutarCpblock(char **argumentos);
void* levantarConsola();
#endif
