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
	cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

	// espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
	this_thread::sleep_for(duracion_produ);

	const int num_ingrediente = aleatorio<0,NUM_FUMADORES-1>();

	// informa de que ha terminado de producir
	cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

	return num_ingrediente;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador) {
	// calcular milisegundos aleatorios de duración de la acción de fumar)
	chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

	// informa de que comienza a fumar

	cout << "Fumador " << num_fumador << " :"
	     << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)"
	     << endl;

	// espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
	this_thread::sleep_for( duracion_fumar );

	// informa de que ha terminado de fumar

	cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente."
	     << endl;
}

/*********************************************************************
*                             MONITOR                                *
**********************************************************************/

class Estanco : public HoareMonitor{
private:
	int ingMostrador; //Ingrediente actual en el ingMostrador
	int compras[NUM_FUMADORES];
	CondVar recogida,
	        fumando[NUM_FUMADORES],
	        dormitar[NUM_FUMADORES];
public:
	Estanco();
	void ponerIngrediente(int ingrediente);
	void esperarRecogidaIngrediente();
	void obtenerIngrediente(int ingrediente);
	void dormir(int fum);

	int getContador(int i);
};

Estanco::Estanco(){
	ingMostrador = -1;

	for(int i = 0; i < NUM_FUMADORES; i++)
		compras[i] = 0;

	recogida = newCondVar();

	for(int i = 0; i < NUM_FUMADORES; i++){
		fumando[i] = newCondVar();
		dormitar[i] = newCondVar();
	}
}

void Estanco::ponerIngrediente(int ingrediente) {
	ingMostrador =  ingrediente;

	mtx.lock();
	cout << "Puesto en venta el ingrediente " << ingMostrador << endl << flush;
	mtx.unlock();

	if(compras[ingrediente] == 3) {
		mtx.lock();
		cout << "\tEl estanquero despierta a el fumador " << ingrediente << endl;
		mtx.unlock();

		dormitar[ingrediente].signal();
	}

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

	compras[ingrediente]++;

	recogida.signal();
}

void Estanco::dormir(int fum) {
	mtx.lock();
	cout << "\tFumador " << fum << " se ha ido a dormir" << endl;
	mtx.unlock();

	dormitar[fum].wait();
}

int Estanco::getContador(int i){
	return compras[i];
}

/*********************************************************************
*              FUNCIONES HEBRA ESTANQUERO Y FUMADORA                 *
**********************************************************************/

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> monitor) {
	int ingrediente;

	while(true) {
		ingrediente = producir_ingrediente();
		monitor->ponerIngrediente(ingrediente);
		monitor->esperarRecogidaIngrediente();
	}
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(MRef<Estanco> monitor, int num_fumador) {
	while(true) {
		monitor->obtenerIngrediente(num_fumador);

		if(monitor->getContador(num_fumador) == 3) {
			monitor->dormir(num_fumador);
		}
		else
			fumar(num_fumador);
	}
}

//----------------------------------------------------------------------

int main() {
	cout << "**************************************************" << endl
	     << "|             Problema de los fumadores          |" << endl
	     << "**************************************************" << endl;

	MRef<Estanco> monitor = Create<Estanco>();

	thread estanquero(funcion_hebra_estanquero, monitor);

	thread fumadores[NUM_FUMADORES];
	for (int i = 0; i < NUM_FUMADORES; i++)
		fumadores[i] = thread(funcion_hebra_fumador, monitor, i);

	estanquero.join();
	for(int i = 0; i < NUM_FUMADORES; i++)
		fumadores[i].join();
}
