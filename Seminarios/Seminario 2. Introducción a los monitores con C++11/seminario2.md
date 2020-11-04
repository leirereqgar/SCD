# Productores-Consumidores con Monitores SU

## Solución LIFO

Con el código escrito en `prodcons_suLIFO.cpp` el resultado es la siguiente traza del programa:

```
-----------------------------------------------------------------------------------------
Problema de los productores-consumidores (Multiples prod/cons, Monitor SU, buffer LIFO). 
------------------------------------------------------..........-------------------------
producido: 0
producido: 1
                  consumido: 0
producido: 2
producido: 3
producido: 4
                  consumido: 1
producido: 5
producido: 6
producido: 7
                  consumido: 4
                  consumido: 5
                  consumido: 3
                  consumido: 7
producido: 8
                  consumido: 2
producido: 9
producido: 10
                  consumido: 9
producido: 11
                  consumido: 10
                  consumido: 8
                  consumido: 6
producido: 12
---Se ha recortado la traza para mejor lectura---
producido: 35
                  consumido: 27
producido: 36
                  consumido: 32
producido: 37
                  consumido: 33
producido: 38
                  consumido: 35
                  consumido: 31
                  consumido: 37
producido: 39
                  consumido: 38
                  consumido: 39
                  consumido: 36
                  consumido: 34
                  consumido: 30
comprobando contadores ....
solución (aparentemente) correcta.
```

En un principio la solución parece ser correcta.

## Solución FIFO

Con el código aportado en `prodcons_suFIFO.cpp`  da como resultado la siguiente traza:

```
-----------------------------------------------------------------------------------------
Problema de los productores-consumidores (Multiples prod/cons, Monitor SU, buffer FIFO). 
------------------------------------------------------..........-------------------------
producido: 0
producido: 1
producido: 2
                  consumido: 0
producido: 3
producido: 4
producido: 5
                  consumido: 2
                  consumido: 1
producido: 6
                  consumido: 4
producido: 7
                  consumido: 3
                  consumido: 5
producido: 8
                  consumido: 6
producido: 9
producido: 10
---Se ha recortado la traza para mejorar la lectura---
producido: 36
producido: 37
                  consumido: 30
producido: 38
                  consumido: 35
                  consumido: 33
                  consumido: 34
producido: 39
                  consumido: 37
                  consumido: 36
                  consumido: 38
                  consumido: 39
comprobando contadores ....
solución (aparentemente) correcta.
```

Vemos que en un principio la solución es correcta

## While vs if

Si cambiásemos el `while` de las funciones por un `if` el programa no funcionaría porque como hay varios productores hay que regular que no puedan producir por encima del máximo de celdas, con solo un `if` solo se comprobaría una vez y seguirían produciendo. Y además se tienen que esperar a que las colas de condición `CondVar` estén llenas.

