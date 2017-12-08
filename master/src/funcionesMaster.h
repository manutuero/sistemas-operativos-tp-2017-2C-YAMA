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
#include <commons/temporal.h>
#include <time.h>

//MACROS

#define TRANSFORMACIONOKYAMA 12
#define REDUCCIONLOCALOKYAMA 16
#define REDUCCIONGLOBALOKYAMA 20
#define ERRORTRANSFORMACION 103
#define ERRORREDUCCION 104
#define ERRORREDUCCIONGLOBAL 105
#define ERRORALMACENADOFINAL 107
#define ERRORALMACENADOFINALYAMA 108

//MACROS YAMA
#define PEDIDOTRANSFORMACION 6
#define ENVIOPLANIFICACION 9
#define INICIOREDUCCIONLOCAL 13
#define INICIOREDUCCIONGLOBAL 17
#define ALMACENAFINALYAMA 21
#define ALMACENADOFINALOKYAMA 29
#define REPLANIFICACION 24
#define ABORTARJOB 101

//MACROS WORKER
#define PEDIDOTRANSFORMACIONWORKER 10 //11
#define PEDIDOREDLOCALWORKER 14
#define PEDIDOREDGLOBALWORKER 18

#define TRANSFORMACIONOKWORKER 11 //11
#define REDUCCIONLOCALOKWORKER 15
#define REDUCCIONGLOBALOKWORKER 19
#define ORDENALMACENADOFINAL 22
#define ALMACENADOFINALOK 28
#define TRANSFORMACIONOK

//define RUTAARCHIVO 4
//serializarRutaArchivo de utils crea un header con id = 5. Habria que sacarlo


	typedef struct{
		uint32_t largoArchivo;
		char* nombreArchivo;
		uint32_t largoArchivo2;
		char* nombreArchivoGuardadoFinal;
	}t_pedidoTransformacion;

	typedef struct{
		uint32_t largoRutaTemporalArchivo;
		char* nombreArchivoTemporal;
		uint32_t largoRutaArchivoFinal;
		char* nombreArchivoArchivoFinal;
	}t_infoGuardadoFinal;

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
		uint32_t bytesOcupados; //agregar
		uint32_t bloqueATransformar;
		uint32_t largoRutaArchivo;
		char* rutaArchivoTemporal;
		uint32_t largoArchivoTransformador;
		char* archivoTransformador;
	}t_infoTransformacion;

	typedef struct{
		int largoRutaTemporal;
		char* rutaTemporal;
	}t_respuestaTransformacion;

	typedef struct{
		int idNodo;
		int cantidadTransformaciones;
	}t_transformacionesNodo;

	typedef struct{
		uint32_t largoRutaArchivoReducidoLocal;
		char* rutaArchivoReducidoLocal;
		uint32_t largoArchivoReductor;
		char* archivoReductor;
		uint32_t cantidadTransformaciones;
		t_list* temporalesTranformacion;
	}t_infoReduccionesLocales;

	typedef struct{
		uint32_t largoRutaTemporalTransformacion;
		char* rutaTemporalTransformacion;
	}t_temporalesTransformacionWorker;

	typedef struct{
		uint32_t largoRutaTemporal;
		char* rutaTemporalTransformacion;
		uint32_t largoRutaArchivoAProcesar;
		char* rutaArchivoAProcesar;
		uint32_t largoRutaArchivoDestino;
		char* rutaArchivoDestino;
	}t_falloTransformacion;

	typedef struct{
		uint32_t largoRutaArchivoTemporalLocal;
		char* rutaArchivoTemporalLocal;
		uint32_t largoRutaArchivoTemporalGlobal;
		char* rutaArchivoTemporalGlobal;
		uint32_t largoArchivoReductor;
		char* archivoReductor;
		uint32_t cantidadNodos;
		t_list* nodosAConectar;
	}t_infoReduccionGlobal;

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
t_list* archivosTranformacionOk;

//semaforos
pthread_mutex_t mutexMaximasTareas;
pthread_mutex_t mutexTotalFallos;
pthread_mutex_t mutexTotalTransformaciones;
pthread_mutex_t mutexTotalReduccionesLocales;
pthread_mutex_t mutexTotalReduccionesGlobales;
pthread_mutex_t mutexConexionWorker;
pthread_mutex_t mutexTiempoTransformaciones;
pthread_mutex_t mutexTiempoReducciones;

//rutas de archivos por parametros
char* transformador;
char* reductor;
char* archivoAprocesar;
char* direccionDeResultado;
//variables
t_transformacionesNodo* nodosTransformacion;
t_log* masterLogger;
time_t tiempo;
char* ipYama;
int puertoYama, socketYama, idMaster;
int transformacionesPendientes, reduccionesLocalesPendientes, fallos;
int cantidadTareasCorriendoTransformacion, maximoTareasCorriendoTransformacion;
int cantidadTareasCorriendoRedLocal, maximoTareasCorriendoRedLocal;
double tiempoTotalTransformaciones, tiempoTotalRedLocales, tiempoTotalRedGlobal;
int reduccionGlobalRealizada, cantidadTransformacionesRealizadas, cantidadReduccionesLocalesRealizadas;
/* FIRMAS DE FUNCIONES  */

int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);
int file_exists (char * fileName);
void crearListas();
void destruirListas();
void limpiarListas();
void liberarTransformaciones(void*);
void liberarReduccionesLocales(void*);
void liberarReduccionesGlobales(void*);
void inicializarMutex();
void crearLogger();
t_config* cargarArchivoDeConfiguracion();

void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado);
int conectarseAYama(int puerto,char* ip);
int conectarseAWorker(int, char*);
void mandarArchivosAYama(int socketYama, char* archivoAprocesar);

void* serializarArchivos(int*);

void recibirPlanificacionDeYama(int socketYama);
void deserializarPlanificacion(void*);
void deserializarTransformaciones(int , void*, int*);
void deserializarReduccionesLocales(int , void* , int* );
void deserializarReduccionesGlobales(int , void* , int*);
void cargarNodosTransformacion();

void operarEtapas();

void replanificarTransformaciones();

void enviarAWorkers(char*,char*);

void enviarTransformacionAWorkers(char* , char* );

void enviarRedLocalesAWorker(t_reduccionGlobalMaster* );

void enviarReduccionGlobalAWorkerEncargado();

void hiloConexionWorker(t_transformacionMaster*);

void enviarFalloTransformacionAYama(t_transformacionMaster*,t_header*);

void* serializarTransformacionWorker(t_infoTransformacion* , int* );

void* serializarReduccionLocalWorker(t_infoReduccionesLocales* , int*);

void* serializarReduccionGlobalWorker(t_infoReduccionGlobal* redGlobalWorker,int* largoBuffer);

void* serializarInfoGuardadoFinal(t_infoGuardadoFinal*,int*);

int respuestaWorker(int);

void disminuirTransformacionesDeNodo(int);

int transformacionExistente(char*);

void borrarTemporalesDeNodo(int);

int devolverTamanioArchivo(char*);
char* obtenerContenidoArchivo(char*);

void avisarAYama(t_transformacionMaster*,t_header);
void avisarAYamaRedLocal(t_infoReduccionesLocales,t_header);
void avisarAYamaRedGlobal(t_infoReduccionGlobal,t_header);
void avisarAlmacenadoFinal();

void metricas(double);

#endif /* FUNCIONESMASTER_H_ */
