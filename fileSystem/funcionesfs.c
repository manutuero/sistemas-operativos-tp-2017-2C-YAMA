/*
 * funcionesfs.c
 *
 *  Created on: 7/9/2017
 *      Author: utnso
 */
#include "funcionesfs.h"

void procesarMensaje(void * buffer, int sd,header header){


	switch (header.id){
	case 1:
		comando comando = (comando)buffer;
		printf("Recibi un comando de consola con id= %d",comando.funcion);
		break;
	default :
		perrror("Recibi verdura y no entendi nada");
	break;

	}

}
