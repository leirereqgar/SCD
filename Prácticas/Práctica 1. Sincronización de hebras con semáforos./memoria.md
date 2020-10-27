# Práctica 1: Sincronización de hebras con semáforos. Problemas del productor-consumidor y estaquero-fumadores.

## 1 Productor-consumidor

### Variables

He usado las siguientes variables:

- `buffer`: vector de 10 elementos que guarda los datos que se producen antes de ser consumidos.
- `n`: el índice que marca que dato se va a consumir.
- `mtx`: un cerrojo para la sección crítica y la salida por pantalla ininterrumpida.

### Semáforos

Se han empleado dos semáforos:

- `escribe`: inicializado con el tamaño del buffer. Como el buffer empieza vacío podrá producir hasta 10 elementos sin ningún impedimento.
  - Con `sem_wait` se espera hasta que haya espacio en el buffer para producir más datos.
  - Con `sem_signal` se avisa de que se han producido datos.
- `lee`: inicializado a 0 porque no hay ningún dato en el buffer para que pueda consumir.
  - `sem_wait` mientras espera que haya datos en el buffer.
  - `sem_signal` cuando ya haya consumido un dato para avisar de que hay espacio en el buffer.

### Versión LIFO

En esta versión el índice `n` será el del último que se ha producido y será el primero en consumirse; para ello se aumenta cuando se producen datos y se reduce cuando se consumen:

```c++
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 40 ,   // número de items
	       tam_vec   = 10 ;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos

int buffer[tam_vec];
int indice = 0; //Llevar la cuenta del último

Semaphore escribe = tam_vec,
	       lee     = 0;
mutex mtx;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio() {
	static default_random_engine generador( (random_device())() );
	static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
	return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato() {
	static int contador = 0 ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	mtx.lock();
	cout << "producido: " << contador << endl << flush ;
	mtx.unlock();

	cont_prod[contador]++ ;
	return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato ) {
	assert( dato < num_items );
	cont_cons[dato] ++ ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	mtx.lock();
	cout << "                  consumido: " << dato << endl ;
	mtx.unlock();
}


//----------------------------------------------------------------------

void test_contadores() {
	bool ok = true ;
	cout << "comprobando contadores ...." ;

	for(unsigned i = 0; i < num_items; i++){
		if (cont_prod[i] != 1){
			cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
			ok = false ;
		}
		if (cont_cons[i] != 1){
			cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
			ok = false ;
		}
	}
	if (ok)
		cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora() {
	for(unsigned i = 0; i < num_items; i++){
		int dato = producir_dato() ;

		sem_wait(escribe);
		mtx.lock();
		buffer[indice] = dato;
		indice++;
		mtx.unlock();
		sem_signal(lee);
	}
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora() {
	for( unsigned i = 0; i < num_items; i++){
		int dato ;

		sem_wait(lee);
		mtx.lock();
		indice--;
		dato = buffer[indice];
		mtx.unlock();
		consumir_dato(dato);
		sem_signal(escribe);
	}
}
//----------------------------------------------------------------------

int main() {
	cout << "--------------------------------------------------------"  << endl
	     << "Problema de los productores-consumidores (solución LIFO)." << endl
	     << "--------------------------------------------------------"  << endl
	     << flush ;

	thread hebra_productora (funcion_hebra_productora),
	       hebra_consumidora(funcion_hebra_consumidora);

	hebra_productora.join();
	hebra_consumidora.join();

	test_contadores();
}
```

### Versión FIFO

En esta versión el índice `n` estará en orden de entrada, los que primero se produjeron serán los primeros en consumirse.

```c++
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 40 ,   // número de items
	       tam_vec   = 10 ;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos

int buffer[tam_vec];
int indice = 0; //Llevar la cuenta del último

Semaphore escribe = tam_vec,
	       lee     = 0;
mutex mtx;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio() {
	static default_random_engine generador( (random_device())() );
	static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
	return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato() {
	static int contador = 0 ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	mtx.lock();
	cout << "producido: " << contador << endl << flush ;
	mtx.unlock();

	cont_prod[contador]++ ;
	return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato ) {
	assert( dato < num_items );
	cont_cons[dato] ++ ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	mtx.lock();
	cout << "                  consumido: " << dato << endl ;
	mtx.unlock();
}


//----------------------------------------------------------------------

void test_contadores() {
	bool ok = true ;
	cout << "comprobando contadores ...." ;

	for(unsigned i = 0; i < num_items; i++){
		if (cont_prod[i] != 1){
			cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
			ok = false ;
		}
		if (cont_cons[i] != 1){
			cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
			ok = false ;
		}
	}
	if (ok)
		cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora() {
	for(unsigned i = 0; i < num_items; i++){
		int dato = producir_dato() ;

		sem_wait(escribe);
		mtx.lock();
		indice = i % tam_vec;
		buffer[indice] = dato;
		mtx.unlock();
		sem_signal(lee);
	}
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora() {
	for( unsigned i = 0; i < num_items; i++){
		int dato ;

		sem_wait(lee);
		mtx.lock();
		indice = i % tam_vec;
		dato = buffer[indice];
		mtx.unlock();
		sem_signal(escribe);
		consumir_dato(dato);
	}
}
//----------------------------------------------------------------------

int main() {
	cout << "--------------------------------------------------------"  << endl
	     << "Problema de los productores-consumidores (solución LIFO)." << endl
	     << "--------------------------------------------------------"  << endl
	     << flush ;

	thread hebra_productora (funcion_hebra_productora),
	       hebra_consumidora(funcion_hebra_consumidora);

	hebra_productora.join();
	hebra_consumidora.join();

	test_contadores();
}
```

## 2 Problema de los fumadores

### Semáforos

- `mostrador`: semáforo que controla si está o no disponible el estanquero para comprar los ingredientes. Se inicializa a 1 para que el primer ingrediente se pueda poner en venta.
- `ingredientes`: vector de 3 mostradores, todos inicializados a 0 porque no hay nada en el mostrador para comprar.
- `mtx`: cerrojo para la salida de texto.

```c++
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int NUM_FUMADORES = 3;
Semaphore mostrador = 1;
Semaphore ingredientes[3] = {0,0,0}; //Controlar si los fumadores tienen su ingrediente o no
mutex mtx;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio() {
	static default_random_engine generador( (random_device())() );
	static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
	return distribucion_uniforme( generador );
}

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente(){
	// calcular milisegundos aleatorios de duración de la acción de fumar)
	chrono::milliseconds duracion_produ(aleatorio<10,100>());

	// informa de que comienza a producir
	cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

	// espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
	this_thread::sleep_for(duracion_produ);

	const int num_ingrediente = aleatorio<0,NUM_FUMADORES-1>();

	// informa de que ha terminado de producir
	cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

	return num_ingrediente;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero() {
	while(true) {
		sem_wait(mostrador);

		int producido = producir_ingrediente();

		mtx.lock();
		cout << "El estanquero produce el item " << producido << endl;
		mtx.unlock();

		sem_signal(ingredientes[producido]);
	}
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador) {
	// calcular milisegundos aleatorios de duración de la acción de fumar)
	chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

	// informa de que comienza a fumar

	cout << "Fumador " << num_fumador << " :"
	     << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

	// espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
	this_thread::sleep_for( duracion_fumar );

	// informa de que ha terminado de fumar

	cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(int num_fumador) {
	while(true) {
		sem_wait(ingredientes[num_fumador]);

		mtx.lock();
		cout << "Fumador " << num_fumador << " compra su ingrediente" << endl;
		mtx.unlock();

		sem_signal(mostrador);

		fumar(num_fumador);
	}
}

//----------------------------------------------------------------------

int main() {
	cout << "**************************************************" << endl
	     << "|             Problema de los fumadores          |" << endl
	     << "**************************************************" << endl;

	thread estanquero(funcion_hebra_estanquero);

	vector<thread> fumadores(NUM_FUMADORES);
	for (int i = 0; i < NUM_FUMADORES; i++)
		fumadores[i] = thread(funcion_hebra_fumador, i);

	estanquero.join();

	for(int i = 0; i < NUM_FUMADORES; i++)
		fumadores[i].join();
}
```