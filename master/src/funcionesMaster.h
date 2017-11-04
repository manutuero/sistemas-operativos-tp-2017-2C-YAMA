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

//MACROS
#define TRANSFORMACION 1
#define REDUCCIONLOCAL 2
#define REDUCCIONGLOBAL 3
#define ERRORTRANSFORMACION -1
#define ERRORREDUCCIONLOCAL -2
#define ERRORREDUCCIONGLOBAL -3
#define ALMACENAFINAL 9
#define ERRORALMACENADOFINAL -4

//MACROS YAMA
#define PLANIFICACION 5
//define RUTAARCHIVO 4
//serializarRutaArchivo de utils crea un header con id = 5. Habria que sacarlo


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

/* Estructuras de conexion con Worker */
	typedef struct{
		uint32_t bloqueATransformar;
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
		uint32_t largoRutaArchivoReductorLocal;
		char* rutaArchivoReductorLocal;
		uint32_t largoArchivoReductor;
		char* archivoReductor;
		uint32_t cantidadTransformaciones;
		t_list* temporalesTranformacion;
	}t_redLocalesWorker;


	typedef struct{
		uint32_t etapa;
		uint32_t largoRutaArchivoTemporal;
		char* rutaArchivoTemporal;
		uint32_t largoArchivoReductor;
		char* archivoReductor;
		uint32_t cantidadNodos;
		t_list* nodosAConectar;
	}t_reduccionGlobalWorker;

	typedef struct{
		uint32_t largoRutaTemporalTransformacion;
		char* rutaTemporalTransformacion;
	}t_temporalesTransformacionWorker;

	typedef struct{
		uint32_t puerto;
		uint32_t largoIp;
		char* ip;
		uint32_t largoRutaArchivoReduccionLocal;
		char* rutaArchivoReduccionLocal;
	}t_datosNodoAEncargado;


/* 		Variables Globales  	 */

//listas globales
t_list* listaTransformaciones;
t_list* listaRedLocales;
t_list* listaRedGloblales;

//semaforos
pthread_mutex_t mutexMaximasTareas;
pthread_mutex_t mutexTotalFallos;
pthread_mutex_t mutexTotalTransformaciones;
pthread_mutex_t mutexTotalReduccionesLocales;
pthread_mutex_t mutexTotalReduccionesGlobales;

//rutas de archivos por parametros
char* transformador;
char* reductor;
char* archivoAprocesar;
char* direccionDeResultado;
//variables
t_transformacionesNodo* nodosTransformacion;
t_log* masterLogger;
char* ipYama;
int puertoYama, socketYama;
int transformacionesPendientes, fallos;
int cantidadTareasCorriendo, maximoTareasCorriendo;
/* FIRMAS DE FUNCIONES  */

int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);
int file_exists (char * fileName);
void crearListas();
void destruirListas();
void inicializarMutex();
void crearLogger();
t_config* cargarArchivoDeConfiguracion();

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

void enviarReduccionGlobalAWorkerEncargado();

void hiloConexionWorker(t_transformacionMaster*);

void* serializarTransformacionWorker(t_transformacionWorker* , int* );

void* serializarReduccionLocalWorker(t_redLocalesWorker* , int*);

void* serializarReduccionGlobalWorker(t_reduccionGlobalWorker* redGlobalWorker,int* largoBuffer);

int respuestaTransformacion(int);

void disminuirTransformacionesDeNodo(int);

int devolverTamanioArchivo(char*);
char* obtenerContenidoArchivo(char*);

void avisarAYama(t_transformacionMaster*,t_header);
void avisarAYamaRedLocal(t_redLocalesWorker,t_header);
void avisarAYamaRedGlobal(t_reduccionGlobalWorker,t_header);
void avisarAlmacenadoFinal();

void metricas();

#endif /* FUNCIONESMASTER_H_ */
