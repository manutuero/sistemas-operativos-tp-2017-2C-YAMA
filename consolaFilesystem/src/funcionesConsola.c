#include "funcionesConsola.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <commons/string.h>
#define ESPACIO " "

void inicializarInstruccion(comando *instruccion)
{
	instruccion->funcion = 0;
	instruccion->bloque = 0;
	instruccion->idNodo=0;
	instruccion->opcion = 0;
	instruccion->parametro1 = "";
	instruccion->parametro2 = "";
}

void cargarArchivoDeConfiguracion(char *rutaAConfig) {

   char      cwd[1024];                                                                           // Variable donde voy a guardar el path absoluto hasta el /Debug
   char *    pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)), rutaAConfig); // String que va a tener el path absoluto para pasarle al config_create
   t_config *config         = config_create(pathArchConfig);

   if (config_has_property(config, "PUERTO_FILESYSTEM")) {
      PuertoFS = config_get_int_value(config, "PUERTO_FILESYSTEM");  //asigna a PUERTOSERVIDOR el puerto del .cfg
   }

   if (config_has_property(config, "IP_FILESYSTEM")) {
      IP = string_duplicate(config_get_string_value(config, "IP_FILESYSTEM")); //asigna a IPSERVIDOR el puerto del .cfg
   }

   printf("Puerto: %d\n", PuertoFS);
   printf("IP: %s\n\n\n", IP);

   config_destroy(config);

}

int validarParametro(char *parametro)
{
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

char** cargarArgumentos(char* linea){
	string_trim(&linea);
	return(string_split(linea,ESPACIO));
}

comando empaquetarFuncionFormat(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	instruccion.funcion = 1;

	printf("funcion de format\n");

	return instruccion;
}

comando empaquetarFuncionRm(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if (validarParametro(argumentos[1])==1)
		{
			instruccion.funcion=2;
			instruccion.parametro1=argumentos[1];

			printf("Funcion de remove archivo estandar\n");
			printf("El archivo a remover es: %s\n",argumentos[1]);
		}
	else
	{
		printf("El parametro ingresado %s no es valido\n",argumentos[1]);
	}

	return instruccion;

}

comando empaquetarFuncionRmDirectory(char **argumentos){
	//revisar que directorio esta vacio primero
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if((strcmp(argumentos[1],"-d")==0)&&((validarParametro(argumentos[2]))==1))
		{
			instruccion.funcion=3;
			instruccion.opcion=1;
			instruccion.parametro1=argumentos[2];

			printf("funcion de remove directorio\n");
			printf("el directorio a remover es: %s\n",argumentos[2]);
		}
	else
			printf("La opcion: %s no es valida o el parametro: %s no es correcto\n",argumentos[1],argumentos[2]);


	return instruccion;

}

comando empaquetarFuncionRmBloque(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	//revisar que no es la ultima copia del bloque
	if((strcmp(argumentos[1],"-b")==0)&&(isdigit(argumentos[3]))&&(isdigit(argumentos[4])))
		{
			instruccion.funcion=4;
			instruccion.opcion=2;
			instruccion.bloque=(int)argumentos[3];
			instruccion.idNodo=(int)argumentos[4];

			printf("Funcion de remover un bloque\n");
			printf("Se removera del archivo: %s\n",argumentos[2]);
			printf("El bloque: %s de la copia: %s\n",argumentos[3],argumentos[4]);
		}
	else
			printf("La opcion: %s no es valida o uno de los parametros: %s,%s no es correcto\n",argumentos[1],argumentos[2],argumentos[3]);

	return instruccion;

}

comando empaquetarFuncionCat(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if (validarParametro(argumentos[1])==1)
		{
			instruccion.funcion=5;
			instruccion.parametro1=argumentos[1];

			printf("Funcion de concatenar texto plano\n");
			printf("el archivo es: %s\n",argumentos[1]);
		}
	else
			printf("El parametro ingresado: %s no es valido\n",argumentos[1]);

	return instruccion;
}

comando empaquetarFuncionMkdir(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if(validarParametro(argumentos[1]))
		{
			instruccion.funcion=6;
			instruccion.parametro1 = argumentos[1];

			printf("funcion de crear directorio\n");
			printf("El directorio a crear es: %s\n",argumentos[1]);
			return instruccion;
		}
	else
			printf("El parametro ingresado: %s no es valido\n",argumentos[1]);

	return instruccion;
}

comando empaquetarFuncionMd5(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if(validarParametro(argumentos[1]))
		{
			instruccion.funcion=7;
			instruccion.parametro1 = argumentos[1];

			printf("funcion de md5\n");
			printf("El archivo es: %s\n",argumentos[1]);
		}
	else
			printf("El parametro ingresado: %s no es valido\n",argumentos[1]);

	return instruccion;
}

comando empaquetarFuncionLs(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if(validarParametro(argumentos[1]))
		{
			instruccion.funcion=8;
			instruccion.parametro1=argumentos[1];

			printf("funcion ls\n");
			printf("El directorio a listar es: %s\n",argumentos[1]);
		}
	else
			printf("El parametro ingresado: %s no es valido\n",argumentos[1]);

	return instruccion;
}

comando empaquetarFuncionInfo(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if(validarParametro(argumentos[1]))
		{
			instruccion.funcion=9;
			instruccion.parametro1=argumentos[1];

			printf("funcion info\n");
			printf("El archivo es: %s\n",argumentos[1]);
		}
	else
			printf("El parametro ingresado: %s no es valido\n",argumentos[1]);

	return instruccion;
}

comando empaquetarFuncionRename(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if(validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
		{
			instruccion.funcion=10;
			instruccion.parametro1=argumentos[1];
			instruccion.parametro2=argumentos[2];

			printf("funcion rename\n");
			printf("El nombre original era: %s\n",argumentos[1]);
			printf("El nuevo nombre es: %s\n",argumentos[2]);
		}
	else
			printf("Los parametros ingresados: %s,%s no son validos\n",argumentos[1],argumentos[2]);

	return instruccion;
}

comando empaquetarFuncionMv(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if(validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
		{
			instruccion.funcion=11;
			instruccion.parametro1=argumentos[1];
			instruccion.parametro2=argumentos[2];

			printf("funcion move\n");
			printf("La ruta original era: %s\n",argumentos[1]);
			printf("La nueva ruta es: %s\n",argumentos[2]);
		}
	else
		printf("Los parametros ingresados: %s,%s no son validos\n",argumentos[1],argumentos[2]);

	return instruccion;
}

comando empaquetarFuncionCpfrom(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if(validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
		{
			instruccion.funcion=12;
			instruccion.parametro1=argumentos[1];
			instruccion.parametro2=argumentos[2];

			printf("funcion cpfrom\n");
			printf("Los argumentos son: %s y %s\n",argumentos[1],argumentos[2]);
		}
	else
		printf("Los parametros ingresados: %s,%s no son validos\n",argumentos[1],argumentos[2]);

	return instruccion;
}

comando empaquetarFuncionCpto(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if (validarParametro(argumentos[1]) && validarParametro(argumentos[2]))
		{
			instruccion.funcion=13;
			instruccion.parametro1=argumentos[1];
			instruccion.parametro2=argumentos[2];

			printf("funcion cpto\n");
			printf("Los argumentos son: %s y %s\n",argumentos[1],argumentos[2]);
		}
	else
		printf("Los parametros ingresados: %s,%s no son validos\n",argumentos[1],argumentos[2]);

	return instruccion;
}

comando empaquetarFuncionCpblok(char **argumentos){
	comando instruccion;
	inicializarInstruccion(&instruccion);

	if((isdigit(argumentos[2]))&&(isdigit(argumentos[3]))&&(validarParametro(argumentos[1])))
		{
			instruccion.funcion=14;
			instruccion.parametro1=argumentos[1];
			instruccion.bloque=(int)argumentos[2];
			instruccion.idNodo=(int)argumentos[3];

			printf("funcion cpblock\n");
			printf("El archivo al que pertenece el bloque: %s\n",argumentos[1]);
			printf("El numero de bloque es: %s\n",argumentos[2]);
			printf("El nodo en el que copiar es: %s\n",argumentos[3]);
		}
	else
		printf("Los parametros ingresados: %s,%s,%s no son validos\n",argumentos[1],argumentos[2],argumentos[3]);

	return instruccion;
}

void* serializarComandoConsola(comando* comando, header* header) {
	int tamanioMensaje  = 0;
	int desplazamientoMensaje = 0;

	// Primera parte: serializa la estructura comando.
	void *mensaje = malloc(sizeof(comando->funcion));

	memcpy(mensaje + desplazamientoMensaje, &(comando->funcion), sizeof(comando->funcion));
	desplazamientoMensaje += sizeof(comando->funcion);
	tamanioMensaje += sizeof(comando->funcion);

	int tamanioRedimensionadoAux = sizeof(comando->funcion) + sizeof(comando->opcion);
	mensaje=realloc(mensaje, tamanioRedimensionadoAux);
	memcpy(mensaje + desplazamientoMensaje, &(comando->opcion), sizeof(comando->opcion));
	desplazamientoMensaje += sizeof(comando->opcion);
	tamanioMensaje += sizeof(comando->opcion);

	int tamanioParametro1 = strlen(comando->parametro1);
	tamanioRedimensionadoAux += sizeof(int);
	mensaje = realloc(mensaje, tamanioRedimensionadoAux);
	memcpy(mensaje + desplazamientoMensaje, &(tamanioParametro1), sizeof(int));
	desplazamientoMensaje += sizeof(int);
	tamanioMensaje += sizeof(int);

	tamanioRedimensionadoAux += strlen(comando->parametro1);
	mensaje=realloc(mensaje, tamanioRedimensionadoAux);
	memcpy(mensaje + desplazamientoMensaje, comando->parametro1, strlen(comando->parametro1));
	desplazamientoMensaje += sizeof(strlen(comando->parametro1));
	tamanioMensaje += tamanioParametro1;

	int tamanioParametro2 = strlen(comando->parametro2);
	tamanioRedimensionadoAux += sizeof(int);
	mensaje=realloc(mensaje, tamanioRedimensionadoAux);
	memcpy(mensaje + desplazamientoMensaje, &(tamanioParametro2), sizeof(int));
	desplazamientoMensaje += sizeof(int);
	tamanioMensaje += sizeof(int);

	tamanioRedimensionadoAux += strlen(comando->parametro2);
	mensaje=realloc(mensaje, tamanioRedimensionadoAux);
	memcpy(mensaje + desplazamientoMensaje, comando->parametro2, strlen(comando->parametro2));
	desplazamientoMensaje += sizeof(strlen(comando->parametro2));
	tamanioMensaje += sizeof(tamanioParametro2);

	tamanioRedimensionadoAux += sizeof(int);
	mensaje=realloc(mensaje, tamanioRedimensionadoAux);
	memcpy(mensaje + desplazamientoMensaje, &(comando->bloque), sizeof(int));
	desplazamientoMensaje += sizeof(comando->bloque);
	tamanioMensaje += sizeof(comando->bloque);

	tamanioRedimensionadoAux += sizeof(int);
	mensaje = realloc(mensaje, tamanioRedimensionadoAux);
	memcpy(mensaje + desplazamientoMensaje, &(comando->idNodo), sizeof(int));
	desplazamientoMensaje += sizeof(comando->idNodo);
	tamanioMensaje += sizeof(comando->idNodo);

	header->tamanio = tamanioMensaje;

	return mensaje;
}

