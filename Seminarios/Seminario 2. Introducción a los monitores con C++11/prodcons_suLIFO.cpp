#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <random>
#include "HoareMonitor.h"
#include "Semaphore.h"

using namespace std ;
using namespace HM ;

//-----------------------------------------------------------------------------
constexpr int num_items        = 40,
              num_productores  = 4,
              num_consumidores = 4;

mutex mtx ;

unsigned cont_prod[num_items],
         cont_cons[num_items],
         producidos[num_productores];

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

int producir_dato(int i) {
	if(producidos[i] < (num_items / num_productores)){
		static int contador = 0 ;
		this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
		mtx.lock();
		cout << "producido: " << contador << endl << flush ;
		producidos[i]++;
		mtx.unlock();
		cont_prod[contador] ++ ;
		return contador++;
	}
}
//-----------------------------------------------------------------------------
void consumir_dato( unsigned dato )
{
	if ( num_items <= dato ) {
		cout << " dato === " << dato << ", num_items == " << num_items << endl ;
		assert( dato < num_items );
	}

	cont_cons[dato] ++ ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
	mtx.lock();
	cout << "                  consumido: " << dato << endl ;
	mtx.unlock();
}

//-----------------------------------------------------------------------------

void ini_contadores() {
	for( unsigned i = 0 ; i < num_items ; i++ ){
		cont_prod[i] = 0 ;
		cont_cons[i] = 0 ;
	}
}
//-----------------------------------------------------------------------------
void test_contadores()
{
	bool ok = true ;
	cout << "comprobando contadores ...." << flush ;

	for( unsigned i = 0 ; i < num_items ; i++ ) {
		if ( cont_prod[i] != 1 ) {
			cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
			ok = false ;
		}
		if ( cont_cons[i] != 1 ) {
			cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
			ok = false ;
		}
	}
	if (ok)
		cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}
//-----------------------------------------------------------------------------
class ProdConsSULIFO : public HoareMonitor {
private:
	static const int num_celdas_total = 10 ;
	int buffer[num_celdas_total], // buffer de tamaño fijo, con los datos
       primera_libre ;          // indice de celda de la próxima inserción
	CondVar ocupadas, libres ;

public:
	ProdConsSULIFO(  ) ;
	int  leer();
	void escribir( int valor );
};
//-----------------------------------------------------------------------------
ProdConsSULIFO::ProdConsSULIFO(  ) {
	primera_libre = 0 ;
	ocupadas      = newCondVar();
	libres        = newCondVar();
}
//-----------------------------------------------------------------------------
int ProdConsSULIFO::leer(  ) {
	// esperar bloqueado hasta que 0 < num_celdas_ocupadas
	while ( primera_libre == 0 )
		ocupadas.wait();

	// hacer la operación de lectura, actualizando estado del monitor
	assert(0 < primera_libre);
	const int valor = buffer[primera_libre - 1 ] ;
	primera_libre-- ;

	// señalar al productor que hay un hueco libre, por si está esperando
	libres.signal();

	// devolver valor
	return valor ;
}

//-----------------------------------------------------------------------------

void ProdConsSULIFO::escribir( int valor ) {
	// esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
	while (primera_libre == num_celdas_total)
		libres.wait();

	//cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
	assert( primera_libre < num_celdas_total );

	// hacer la operación de inserción, actualizando estado del monitor
	buffer[primera_libre] = valor ;
	primera_libre++ ;

	// señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
	ocupadas.signal();
}

//-----------------------------------------------------------------------------

void funcion_hebra_productora( MRef<ProdConsSULIFO> monitor, int hebra ) {
	for( unsigned i = 0 ; i < num_items/num_productores ; i++ ) {
		int valor = producir_dato(hebra) ;
		monitor->escribir( valor );
	}
}
//-----------------------------------------------------------------------------
void funcion_hebra_consumidora( MRef<ProdConsSULIFO> monitor, int hebra )
{
	for( unsigned i = 0 ; i < num_items/num_consumidores ; i++ ) {
		int valor = monitor->leer();
		consumir_dato( valor );
	}
}
//-----------------------------------------------------------------------------
int main() {
	cout << "-----------------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Multiples prod/cons, Monitor SU, buffer LIFO). " << endl
        << "------------------------------------------------------..........-------------------------" << endl
        << flush ;

	for(int i = 0; i < num_productores; i++){
		producidos[i] = 0;
	}

	MRef<ProdConsSULIFO> monitor = Create<ProdConsSULIFO>( );

	thread hebras_productoras[num_productores];
	for(int i = 0; i < num_productores; i++){
		hebras_productoras[i] = thread (funcion_hebra_productora, monitor, i);
	}

	thread hebras_consumidoras[num_consumidores];
	for(int i = 0; i < num_consumidores; i++){
		hebras_consumidoras[i] = thread ( funcion_hebra_consumidora, monitor, i);
	}

	for(int i = 0; i < num_productores; i++){
		hebras_productoras[i].join();
	}

	for(int i = 0; i < num_consumidores; i++){
		hebras_consumidoras[i].join();
	}

	test_contadores() ;
}
