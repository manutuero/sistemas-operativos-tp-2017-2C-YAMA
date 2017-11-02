/*
 * funcionesMaster.h
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */


#ifndef FUNCIONESMASTER_H_
#define FUNCIONESMASTER_H_

#include <utils.h>
#include <commons/collections/list.h>


/*
typedef struct rutaArchivo{
	int tamanio;
	char* ruta;
}__attribute__((packed)) t_rutaArchivo;
*/

/* Utilizada para enviar la etapa de transformacion */
	typedef struct{
		uint32_t idNodo;
		uint32_t nroBloqueNodo;
		uint32_t puerto;
		uint32_t bytesOcupados;
		uint32_t largoIp;
		char* ip;
		uint32_t largoArchivo;
		char* archivoTransformacion;
	}t_transformacionMaster;

/* Utilizada para enviar la etapa de reduccion local */
	typedef struct{
		uint32_t idNodo;
		uint32_t puerto;
		uint32_t largoIp;
		char* ip;
		uint32_t largoArchivoTransformacion;
		char* archivoTransformacion;
		uint32_t largoArchivoRedLocal;
		char* archivoRedLocal;
	}t_reduccionLocalMaster;

/* Utilizada para enviar la etapa de reduccion global*/
	typedef struct{
		uint32_t idNodo;
		uint32_t puerto;
		uint32_t encargado;
		uint32_t largoIp;
		char* ip;
		uint32_t largoArchivoRedLocal;
		char* archivoRedLocal;
		uint32_t largoArchivoRedGlobal;
		char* archivoRedGlobal;
	}t_reduccionGlobalMaster;


	typedef struct{
		//int socketWorker;
		uint32_t bloqueATransformar;
		//uint32_t bytesOcupados;
		uint32_t etapa;
		uint32_t largoRutaArchivo;
		char* rutaArchivoTemporal;
		uint32_t largoArchivoTransformador;
		char* archivoTransformador;
	}t_transformacionWorker;

	typedef struct{
		int largoRutaTemporal;
		char* rutaTemporal;
	}t_respuestaTransformacion;

	typedef struct{
		int idNodo;
		int cantidadTransformaciones;
	}t_transformacionesNodo;

	typedef struct{
		uint32_t etapa;
		uint32_t largoArchivoReductorLocal;
		char* archivoReductorLocal;
		uint32_t cantidadTransformaciones;
		t_list* temporalesTranformacion;
	}t_redLocalesWorker;

	typedef struct{
		uint32_t largoRutaTemporalTransformacion;
		char* rutaTemporalTransformacion;
	}t_temporalesTransformacionWorker;

t_list* listaTransformaciones;
t_list* listaRedLocales;
t_list* listaRedGloblales;

t_transformacionesNodo* nodosTransformacion;

char* transformador;
char* reductor;
char* archivoAprocesar;
char* direccionDeResultado;
int socketYama;



int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);
int file_exists (char * fileName);
void crearListas();
void destruirListas();
void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado);
int conectarseAYama(int puerto,char* ip);
int conectarseAWorker(int, char*);
void mandarRutaArchivoAYama(int socketYama, char* archivoAprocesar);

void recibirPlanificacionDeYama(int socketYama);
void deserializarPlanificacion(void*);
void deserializarTransformaciones(int , void*, int*);
void deserializarReduccionesLocales(int , void* , int* );
void deserializarReduccionesGlobales(int , void* , int*);

void operarEtapas();

void enviarAWorkers(char*,char*);

void enviarTransformacionAWorkers(char* , char* );

void enviarRedLocalesAWorker(t_reduccionGlobalMaster* );

void hiloConexionWorker(t_transformacionMaster*);

void* serializarTransformacionWorker(t_transformacionWorker* , int* );

void* serializarReduccionLocalWorker(t_redLocalesWorker* , int*);

int respuestaTransformacion(int);

void disminuirTransformacionesDeNodo(int);

int devolverTamanioArchivo(char*);
char* obtenerContenidoArchivo(char*);

void avisarAYama(t_transformacionMaster*);


#endif /* FUNCIONESMASTER_H_ */
