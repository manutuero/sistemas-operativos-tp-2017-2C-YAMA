/*
 * funcionesYAMA.h
 *
 *  Created on: 20/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESYAMA_H_
#define FUNCIONESYAMA_H_

#include "utils.h"

void cargarArchivoDeConfiguracion();
void conectarseAFS();
void yamaEscuchando();

struct sockaddr_in direccionFS;
struct sockaddr_in direccionYama;
int socketYama;

#endif /* FUNCIONESYAMA_H_ */
