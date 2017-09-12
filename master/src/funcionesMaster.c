#include "funcionesMaster.h"

int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado){


	if(!string_starts_with(archivoAprocesar,"yamafs:/")){
		printf("Parametro archivo a procesar invalido.: %s \n",archivoAprocesar);
		return 0;
	}
	if(!string_starts_with(direccionDeResultado,"yamafs:/")){
		printf("La direccion de guardado de resultado es invalida: %s \n",direccionDeResultado);
		return 0;
	}

	if(!file_exists(transformador)){
		printf("El programa transformador no se encuentra en : %s  \n",archivoAprocesar);
		return 0;
	}
	if(!file_exists(reductor)){
		printf("El programa reductor no se encuentra en : %s  \n",archivoAprocesar);
		return 0;
		}




	return 1;

}



//Chequea existencia de archivo en linux
int file_exists (char * fileName)
{
   struct stat buf;
   int i = stat ( fileName, &buf );
     /* File found */
     if ( i == 0 )
     {
       return 1;
     }
     return 0;

}

