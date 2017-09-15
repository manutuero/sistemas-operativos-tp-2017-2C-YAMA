#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

#include "utils.h"


//Variables globales empiezan en mayuscula

int PuertoFS;

char * IP;

//Firma de funciones

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

void cargarArchivoDeConfiguracion(char *rutaAConfig);

void inicializarInstruccion(comando *instruccion);

int validarParametro(char * parametro);

char **cargarArgumentos(char* linea);

void *serializarComandoConsola(comando*, header*);

#endif /* FUNCIONESCONSOLA_H_ */
