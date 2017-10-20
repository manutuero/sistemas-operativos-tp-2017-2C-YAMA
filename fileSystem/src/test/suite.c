#include "suite.h"
#include "../funcionesFileSystem.h"
#include <CUnit/Basic.h>

int correrTests() {
	// Inicializa un registro de suites vacío.
	CU_initialize_registry();

	// Crea una nueva suite de tests, sin tomar funciones de inicializacion ni limpieza.
	CU_pSuite suiteFs = CU_add_suite("Suite fileSystem", NULL, NULL);

	// Agrego un caso de prueba a la suite.
	CU_add_test(suiteFs, "noExistePath",
			test_existePathDirectorio_noExistePath);
	CU_add_test(suiteFs, "existePath", test_existePathDirectorio_existePath);
	CU_add_test(suiteFs, "devuelveFalse",
			test_obtenerNodoMAsLibre_devuelveFalse);
	CU_add_test(suiteFs, "devuelveTrue", test_obtenerNodoMAsLibre_devuelveTrue);

	// Settea la librería de modo tal que muestre la mayor cantidad de información posible (con el flag CU_BRM_VERBOSE).
	CU_basic_set_mode(CU_BRM_VERBOSE);

	// Corre los tests.
	CU_basic_run_tests();

	/* Destruye todas las estructuras y los tests creados,
	 * libera la memoria utilizada por la librería (para evitar memory leaks).
	 * CUnit nos pide que lo llamemos y que sea lo último que hagamos. */
	CU_cleanup_registry();

	// Devuelve un código al finalizar la ejecución, que puede ser de error o de ejecución correcta.
	return CU_get_error();
}

/* Test cases */
void test_existePathDirectorio_noExistePath(void) {
	CU_ASSERT_FALSE(existePathDirectorio("/path/que/no/existe"));
}

void test_existePathDirectorio_existePath(void) {
	CU_ASSERT_TRUE(existePathDirectorio("/root"));
}

void test_obtenerNodoMAsLibre_devuelveFalse(void) {
	t_nodo *nodo = malloc(sizeof(t_nodo));
	nodo->socketDescriptor = 1;
	nodo->idNodo = 1;
	nodo->bloquesTotales = 5;
	nodo->bloquesLibres = 5;
	nodo->puertoWorker = 8888;
	nodo->bitmap = persistirBitmap(nodo->idNodo, nodo->bloquesTotales);
	nodo->ip = "127.0.0.1";
	agregarNodo(nodo);

	nodo = malloc(sizeof(t_nodo));
	nodo->socketDescriptor = 2;
	nodo->idNodo = 2;
	nodo->bloquesTotales = 10;
	nodo->bloquesLibres = 10;
	nodo->puertoWorker = 9999;
	nodo->bitmap = persistirBitmap(nodo->idNodo, nodo->bloquesTotales);
	nodo->ip = "127.0.0.1";
	agregarNodo(nodo);

	CU_ASSERT_NOT_EQUAL(obtenerNodoMasLibre(nodos), 1);

	// Limpia lo que genero en Fs de linux. Hardcodeadisimo.
	char *comando = string_new();
	string_append(&comando, "rm /home/utnso/thePonchos/metadata/bitmaps/1.dat");
	system(comando);

	comando = string_new();
	string_append(&comando, "rm /home/utnso/thePonchos/metadata/bitmaps/2.dat");
	system(comando);
}

void test_obtenerNodoMAsLibre_devuelveTrue(void) {
	t_nodo *nodo = malloc(sizeof(t_nodo));
	nodo->socketDescriptor = 1;
	nodo->idNodo = 1;
	nodo->bloquesTotales = 5;
	nodo->bloquesLibres = 5;
	nodo->puertoWorker = 8888;
	nodo->bitmap = persistirBitmap(nodo->idNodo, nodo->bloquesTotales);
	nodo->ip = "127.0.0.1";
	agregarNodo(nodo);

	nodo = malloc(sizeof(t_nodo));
	nodo->socketDescriptor = 2;
	nodo->idNodo = 2;
	nodo->bloquesTotales = 10;
	nodo->bloquesLibres = 10;
	nodo->puertoWorker = 9999;
	nodo->bitmap = persistirBitmap(nodo->idNodo, nodo->bloquesTotales);
	nodo->ip = "127.0.0.1";
	agregarNodo(nodo);

	CU_ASSERT_EQUAL(obtenerNodoMasLibre(nodos), 2);

	// Limpia lo que genero en Fs de linux. Hardcodeadisimo.
	char *comando = string_new();
	string_append(&comando, "rm /home/utnso/thePonchos/metadata/bitmaps/1.dat");
	system(comando);

	comando = string_new();
	string_append(&comando, "rm /home/utnso/thePonchos/metadata/bitmaps/2.dat");
	system(comando);
}
