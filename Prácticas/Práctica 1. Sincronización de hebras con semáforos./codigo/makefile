HOME = .
BIN  = $(HOME)/bin
INC  = $(HOME)/include
SRC  = $(HOME)/src

CXXFLAGS = -std=c++11 -O2 -Wall -I$(INC) -pthread

all: bin prodcons-LIFO prodcons-FIFO fumadores

bin: 
	@mkdir -p $(BIN)

prodcons-LIFO: $(SRC)/prodcons-LIFO.cpp $(SRC)/Semaphore.cpp
	@printf "Creando productor consumidor LIFO\n"
	@$(CXX) $(CXXFLAGS) -o $(BIN)/prodcons-LIFO $(SRC)/prodcons-LIFO.cpp $(SRC)/Semaphore.cpp

prodcons-FIFO: $(SRC)/prodcons-FIFO.cpp $(SRC)/Semaphore.cpp
	@printf "Creando productor consumidor FIFO\n"
	@$(CXX) $(CXXFLAGS) -o $(BIN)/prodcons-FIFO $(SRC)/prodcons-FIFO.cpp $(SRC)/Semaphore.cpp
	
fumadores: $(SRC)/fumadores.cpp $(SRC)/Semaphore.cpp
	@printf "Creando problema fumadores\n"
	@$(CXX) $(CXXFLAGS) -o $(BIN)/fumadores $(SRC)/fumadores.cpp $(SRC)/Semaphore.cpp
