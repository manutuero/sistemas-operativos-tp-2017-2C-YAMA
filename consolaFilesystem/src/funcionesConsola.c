#include "funcionesConsola.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <commons/string.h>
#define ESPACIO " "

int validarParametro(char *parametro){
	if ((string_starts_with(parametro,"/")))
		return 1;
	else
		return 0;
}

int cantArgumentos(char **argumentos){
	int cantidad=0 ,i = 0;
	char *palabra;

	palabra = argumentos[i];

	while (palabra!=NULL)
		{
			cantidad++;
			i++;
			palabra = argumentos[i];
		}

	return cantidad;
}

char** cargarArgumentos(char* linea)
{
	string_trim(&linea);
	return(string_split(linea,ESPACIO));
}

comando empaquetarFuncionFormat(char **argumentos){
	comando aux;
	aux.funcion = 1;

	printf("funcion de format\n");

	return aux;
}

comando empaquetarFuncionRm(char **argumentos){
	comando aux;
	if (validarParametro(argumentos[1])==1){
	aux.funcion=2;
	aux.parametro1=argumentos[1];

	printf("Funcion de remove archivo estandar\n");
	printf("El archivo a remover es: %s\n",argumentos[1]);
	}
	else{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionRmDirectory(char **argumentos){
	//revisar que directorio esta vacio primero
	comando aux;
	if((strcmp(argumentos[1],"-d")==0)&&(validarParametro(argumentos[1])==1))
		{
			aux.funcion=3;
			aux.opcion=1;
			aux.parametro1=argumentos[2];

			printf("funcion de remove directorio\n");
			printf("el directorio a remover es: %s\n",argumentos[2]);
		}
	else
			printf("La opcion: %s no es valida\n",argumentos[1]);

	return aux;

}

comando empaquetarFuncionRmBloque(char **argumentos){
	comando aux;
	//revisar que no es la ultima copia del bloque
	if((strcmp(argumentos[1],"-b")==0)&&(isdigit(argumentos[3]))&&(isdigit(argumentos[4])))
		{
			aux.funcion=4;
			aux.opcion=2;
			aux.bloque=(int)argumentos[3];
			aux.idNodo=(int)argumentos[4];

			printf("Funcion de remover un bloque\n");
			printf("Se removera del archivo: %s\n",argumentos[2]);
			printf("El bloque: %s de la copia: %s\n",argumentos[3],argumentos[4]);
		}
	else
		{
			printf("La opcion: %s no es valida\n",argumentos[1]);
			aux.funcion=0;
		}

	return aux;

}

comando empaquetarFuncionCat(char **argumentos){
	comando aux;
	if (validarParametro(argumentos[1])==1){
		aux.funcion=5;
		aux.parametro1=argumentos[1];

		printf("Funcion de concatenar texto plano\n");
		printf("el archivo es: %s\n",argumentos[1]);
	}
	else{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionMkdir(char **argumentos){
	comando aux;

	if(validarParametro(argumentos[1]))
	{
		aux.funcion=6;
		aux.parametro1 = argumentos[1];

		printf("funcion de crear directorio\n");
		printf("El directorio a crear es: %s\n",argumentos[1]);
		return aux;
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionMd5(char **argumentos){
	comando aux;

	if(validarParametro(argumentos[1]))
	{
		aux.funcion=7;
		aux.parametro1 = argumentos[1];

		printf("funcion de md5\n");
		printf("El archivo es: %s\n",argumentos[1]);
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionLs(char **argumentos){
	comando aux;

	if(validarParametro(argumentos[1]))
	{
		aux.funcion=8;
		aux.parametro1=argumentos[1];

		printf("funcion ls\n");
		printf("El directorio a listar es: %s\n",argumentos[1]);
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionInfo(char **argumentos){
	comando aux;

	if(validarParametro(argumentos[1]))
	{
		aux.funcion=9;
		aux.parametro1=argumentos[1];

		printf("funcion info\n");
		printf("El archivo es: %s\n",argumentos[1]);
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionRename(char **argumentos){
	comando aux;

	if(validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
	{
		aux.funcion=10;
		aux.parametro1=argumentos[1];
		aux.parametro2=argumentos[2];

		printf("funcion rename\n");
		printf("El nombre original era: %s\n",argumentos[1]);
		printf("El nuevo nombre es: %s\n",argumentos[2]);
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionMv(char **argumentos){
	comando aux;

	if(validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
	{
		aux.funcion=11;
		aux.parametro1=argumentos[1];
		aux.parametro2=argumentos[2];

		printf("funcion move\n");
		printf("La ruta original era: %s\n",argumentos[1]);
		printf("La nueva ruta es: %s\n",argumentos[2]);
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionCpfrom(char **argumentos){
	comando aux;

	if(validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
	{
		aux.funcion=12;
		aux.parametro1=argumentos[1];
		aux.parametro2=argumentos[2];

		printf("funcion cpfrom\n");
		printf("Los argumentos son: %s y %s\n",argumentos[1],argumentos[2]);
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionCpto(char **argumentos){
	comando aux;

	if (validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
	{
		aux.funcion=13;
		aux.parametro1=argumentos[1];
		aux.parametro2=argumentos[2];

		printf("funcion cpto\n");
		printf("Los argumentos son: %s y %s\n",argumentos[1],argumentos[2]);
	}
	else
	{
		aux.funcion=0;
		printf("El parametro ingresado no es valido\n");
	}

	return aux;
}

comando empaquetarFuncionCpblok(char **argumentos){
	comando aux;

	if((isdigit(argumentos[2]))&&(isdigit(argumentos[3])))
	{
		aux.funcion=14;
		aux.parametro1=argumentos[1];
		aux.bloque=(int)argumentos[2];
		aux.idNodo=(int)argumentos[3];

		printf("funcion cpblock\n");
		printf("El archivo al que pertenece el bloque: %s\n",argumentos[1]);
		printf("El numero de bloque es: %s\n",argumentos[2]);
		printf("El nodo en el que copiar es: %s\n",argumentos[3]);
	}
	else
		{
			printf("El comando ingresado no es valido\n");
			aux.funcion=0;
		}
	return aux;
}
void* serializarComandoConsola(comando* comando, header* header) {
	int tamanioHeader = sizeof(header->tamanio);
	int tamanioTotal = 0;
	int desplazamientoAux = 0;
	int desplazamiento = 0;

	// Primera parte: serializa la estructura comando.
	void *bufferAux = malloc(sizeof(int));
	memcpy(bufferAux + desplazamientoAux, &(comando->funcion), sizeof(int));
	desplazamientoAux += sizeof(int);
	tamanioTotal += sizeof(bufferAux);

	int tamanioRedimensionadoAux = sizeof(int) + sizeof(comando->opcion);
	bufferAux=realloc(bufferAux, tamanioRedimensionadoAux);
	memcpy(bufferAux + desplazamiento, &(comando->opcion), sizeof(comando->opcion));
	desplazamientoAux += sizeof(comando->opcion);
	tamanioTotal += sizeof(bufferAux);

	int tamanioParametro1 = strlen(comando->parametro1);
	tamanioRedimensionadoAux += sizeof(int);
	bufferAux = realloc(bufferAux, tamanioRedimensionadoAux);
	memcpy(bufferAux + desplazamientoAux, &(tamanioParametro1), sizeof(int));
	desplazamientoAux += sizeof(int);
	tamanioTotal += sizeof(bufferAux);

	tamanioRedimensionadoAux += strlen(comando->parametro1);
	bufferAux=realloc(bufferAux, tamanioRedimensionadoAux);
	memcpy(bufferAux + desplazamientoAux, comando->parametro1, strlen(comando->parametro1));
	desplazamientoAux += sizeof(strlen(comando->parametro1));
	tamanioTotal += sizeof(bufferAux);

	int tamanioParametro2 = strlen(comando->parametro2);
	tamanioRedimensionadoAux += sizeof(int);
	bufferAux=realloc(bufferAux, tamanioRedimensionadoAux);
	memcpy(bufferAux + desplazamientoAux, &(tamanioParametro2), sizeof(int));
	desplazamientoAux += sizeof(int);
	tamanioTotal += sizeof(bufferAux);

	tamanioRedimensionadoAux += strlen(comando->parametro2);
	bufferAux=realloc(bufferAux, tamanioRedimensionadoAux);
	memcpy(bufferAux+desplazamientoAux, comando->parametro2, strlen(comando->parametro2));
	desplazamientoAux += sizeof(strlen(comando->parametro2));
	tamanioTotal += sizeof(bufferAux);

	tamanioRedimensionadoAux += sizeof(int);
	bufferAux=realloc(bufferAux, tamanioRedimensionadoAux);
	memcpy(bufferAux + desplazamientoAux, &(comando->bloque), sizeof(int));
	desplazamientoAux += sizeof(comando->bloque);
	tamanioTotal += sizeof(bufferAux);

	tamanioRedimensionadoAux += sizeof(int);
	bufferAux = realloc(bufferAux, tamanioRedimensionadoAux);
	memcpy(bufferAux + desplazamientoAux, &(comando->idNodo), sizeof(int));
	desplazamientoAux += sizeof(comando->idNodo);
	tamanioTotal += sizeof(bufferAux);

	// Segunda parte: serializa el header junto con la estructura ya serializada.
	void* buffer = malloc(tamanioHeader);
	memcpy(buffer + desplazamiento, &(header), sizeof(int));
	desplazamiento += sizeof(int);

	int tamanioRedimensionado = sizeof(int) + sizeof(int);
	buffer = realloc(buffer, tamanioRedimensionado);
	memcpy(buffer + desplazamiento, &(tamanioTotal), tamanioTotal);
	desplazamiento += sizeof(int);

	tamanioRedimensionado += tamanioTotal;
	buffer=realloc(buffer, tamanioRedimensionado);
	memcpy(buffer + desplazamiento, bufferAux, tamanioTotal);

	return buffer;
}

