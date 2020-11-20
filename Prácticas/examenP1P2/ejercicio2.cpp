#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include "HoareMonitor.h"
#include "Semaphore.h"

using namespace std ;
using namespace HM;


//**********************************************************************
// variables compartidas

const int num_lectores   = 5,
	       num_escritores = 3;

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
// Monitor hoare Lec_Esc
//----------------------------------------------------------------------

class Lec_Esc : public HoareMonitor {
private:
	int     n_lec,
	        espera_pareja;
	bool    escrib;
	CondVar lectura,
	        escritura;

public:
	Lec_Esc();
	void ini_lectura();
	void fin_lectura();
	void ini_escritura();
	void fin_escritura();
};

Lec_Esc::Lec_Esc(){
	n_lec         = 0;
	espera_pareja = 0;
	escrib        = false;

	lectura   = newCondVar();
	escritura = newCondVar();
}

void Lec_Esc::ini_lectura(){
	if (escrib){
		lectura.wait();
	}

	n_lec++;
	lectura.signal();
}

void Lec_Esc::fin_lectura(){
	n_lec--;

	if (n_lec == 0){
		escritura.signal();
	}
}

void Lec_Esc::ini_escritura(){
	if (escrib || n_lec > 0){
		escritura.wait();
	}

	escrib = true;

	if(espera_pareja == 0){
		espera_pareja++;
	} else{
		espera_pareja = 0;
	}
}

void Lec_Esc::fin_escritura(){
	escrib = false;

	if(espera_pareja > 0)
		escritura.signal();
	else if (lectura.get_nwt() != 0){
		lectura.signal();
	}
	else{
		escritura.signal();
	}
}


// Función que simula la acción de escribir, como un retardo
// aleatorio de la hebra

void escribir( int n_escritor ) {
	// calcular milisegundos aleatorios de duración de la acción de escribir)
	chrono::milliseconds duracion_escr( aleatorio<10,100>() );

	// informa de que comienza a escribir
	mtx.lock();
	cout << "Escritor " << n_escritor << " : empieza a escribir (" << duracion_escr.count() << " milisegundos)" << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_escr' milisegundos
	this_thread::sleep_for( duracion_escr );

	// informa de que ha terminado de producir
	mtx.lock();
	cout << "Escritor " << n_escritor << " : termina de escribir " << endl;
	mtx.unlock();
}

// Función que ejecuta la hebra del escritor
void funcion_hebra_escritor( MRef<Lec_Esc> monitor, int n_escritor ) {
	while( true ){
		chrono::milliseconds espera( aleatorio<10,100>() );
		this_thread::sleep_for( espera );

		monitor->ini_escritura();
		escribir(n_escritor);
		monitor->fin_escritura();
	}
}

// Función que simula la acción de leer, como un retardo
// aleatorio de la hebra
void leer( int n_lector ) {
	// calcular milisegundos aleatorios de duración de la acción de escribir)
	chrono::milliseconds duracion_lect( aleatorio<10,100>() );

	// informa de que comienza a escribir
	mtx.lock();
	cout << "Lector   " << n_lector << " : empieza a leer (" << duracion_lect.count() << " milisegundos)" << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_escr' milisegundos
	this_thread::sleep_for( duracion_lect );

	// informa de que ha terminado de producir
	mtx.lock();
	cout << "Lector   " << n_lector << " : termina de leer " << endl;
	mtx.unlock();
}


// Función que ejecuta la hebra del fumador
void  funcion_hebra_lector( MRef<Lec_Esc> monitor, int n_lector ) {
	while( true ) {
		chrono::milliseconds espera( aleatorio<100,200>() );
		this_thread::sleep_for( espera );

		monitor->ini_lectura();
		leer(n_lector);
		monitor->fin_lectura();
	}
}

int main() {
	cout << "----------------------------------------------------" << endl
	     << " Problema de los lectores-escritores con monitor SU " << endl
	     << "----------------------------------------------------" << endl
	     << flush ;

	MRef<Lec_Esc> monitor = Create<Lec_Esc>( );
	thread lectores[num_lectores];
	thread escritores[num_escritores];


	for (int i = 0; i < num_lectores; i++){
		lectores[i] = thread (funcion_hebra_lector, monitor, i);
	}

	for (int i = 0; i < num_escritores; i++){
		escritores[i] = thread (funcion_hebra_escritor, monitor, i);
	}


	for (int i = 0; i < num_lectores; i++){
		lectores[i].join();
	}

	for (int i = 0; i < num_escritores; i++){
		escritores[i].join();
	}

	cout << endl << endl;
}
