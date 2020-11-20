#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

const int NUM_FUMADORES = 3;
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
	mtx.lock();
	cout << "Suministradora : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
	this_thread::sleep_for(duracion_produ);

	const int num_ingrediente = aleatorio<0,NUM_FUMADORES-1>();

	// informa de que ha terminado de producir
	mtx.lock();
	cout << "Suministradora : termina de producir ingrediente " << num_ingrediente << endl;
	mtx.unlock();

	return num_ingrediente;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador) {
	// calcular milisegundos aleatorios de duración de la acción de fumar)
	chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

	// informa de que comienza a fumar

	mtx.lock();
	cout << "Fumador " << num_fumador << " :"
	     << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)"
	     << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
	this_thread::sleep_for( duracion_fumar );

	// informa de que ha terminado de fumar

	mtx.lock();
	cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente."
	     << endl;
	mtx.unlock();
}

/*********************************************************************
*                      MONITOR  ESTANCO                              *
**********************************************************************/

class Estanco : public HoareMonitor{
private:
	int ingMostrador; //Ingrediente actual en el ingMostrador
	CondVar recogida,
	        fumando[NUM_FUMADORES];
public:
	Estanco();
	void ponerIngrediente(int ingrediente);
	void esperarRecogidaIngrediente();
	void obtenerIngrediente(int ingrediente);
};

Estanco::Estanco(){
	ingMostrador = -1;
	recogida = newCondVar();

	for(int i = 0; i < NUM_FUMADORES; i++){
		fumando[i] = newCondVar();
	}
}

void Estanco::ponerIngrediente(int ingrediente) {
	ingMostrador =  ingrediente;

	mtx.lock();
	cout << "Puesto en venta el ingrediente " << ingMostrador << endl << flush;
	mtx.unlock();

	fumando[ingrediente].signal();
}

void Estanco:: esperarRecogidaIngrediente() {
	if(ingMostrador != -1)
		recogida.wait();
}

void Estanco::obtenerIngrediente(int ingrediente) {
	if(ingMostrador != ingrediente)
		fumando[ingrediente].wait();
	ingMostrador = -1;

	mtx.lock();
	cout << "Ingrediente " << ingrediente << " recogido" << endl << flush;
	mtx.unlock();

	recogida.signal();
}

/*********************************************************************
*                      MONITOR  ESTANCO                              *
**********************************************************************/
class Productor : public HoareMonitor{
private:
	static const int num_celdas_total = 5;
	int buffer[num_celdas_total], // buffer de tamaño fijo, con los datos
       primera_libre,          // indice de celda de la próxima inserción
       primera_ocupada,
       celdas_ocupadas;
	CondVar ocupadas, libres ;
public:
	Productor();
	int  cogerIngrediente();
	void ponerIngrediente(int valor);
};

Productor::Productor() {
	primera_libre = 0;
	ocupadas      = newCondVar();
	libres        = newCondVar();
}

//-----------------------------------------------------------------------------

int Productor::cogerIngrediente() {
	while (celdas_ocupadas == 0 )
		ocupadas.wait();

	assert(0 < celdas_ocupadas);
	const int ing = buffer[primera_ocupada];
	primera_ocupada = (primera_ocupada + 1) % num_celdas_total;
	celdas_ocupadas--;

	libres.signal();

	return ing;
}

//-----------------------------------------------------------------------------

void Productor::ponerIngrediente(int ingrediente) {
	while (celdas_ocupadas == num_celdas_total)
		libres.wait();

	assert(celdas_ocupadas < num_celdas_total );
	buffer[primera_libre] = ingrediente;
	primera_libre = (primera_libre + 1) % num_celdas_total;
	celdas_ocupadas++;

	ocupadas.signal();
}

/*********************************************************************
*     FUNCIONES HEBRA ESTANQUERO, FUMADORA Y SUMINISTRADORA          *
**********************************************************************/

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> mostrador, MRef<Productor> almacen) {
	int ingrediente;

	while(true) {
		ingrediente = almacen->cogerIngrediente();
		mostrador->ponerIngrediente(ingrediente);
		mostrador->esperarRecogidaIngrediente();
	}
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador) {
	while(true) {
		monitor->obtenerIngrediente(num_fumador);
		fumar(num_fumador);
	}
}

void funcion_hebra_suministrador(MRef<Productor> almacen) {
	int ingrediente = 0;

	while(true) {
		ingrediente = producir_ingrediente();
		almacen->ponerIngrediente(ingrediente);
	}
}

//----------------------------------------------------------------------

int main() {
	cout << "**************************************************" << endl
	     << "|             Problema de los fumadores          |" << endl
	     << "**************************************************" << endl;

	MRef<Estanco> mostrador = Create<Estanco>();
	MRef<Productor> almacen = Create<Productor>();

	thread estanquero(funcion_hebra_estanquero, mostrador, almacen);

	thread fumadores[NUM_FUMADORES];
	for (int i = 0; i < NUM_FUMADORES; i++)
		fumadores[i] = thread(funcion_hebra_fumador, mostrador, i);

	thread suministradora(funcion_hebra_suministrador, almacen);

	suministradora.join();
	estanquero.join();
	for(int i = 0; i < NUM_FUMADORES; i++)
		fumadores[i].join();
}
