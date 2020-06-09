/*!
   \file simulador.h
   \brief Codigo correspondiente a las declaraciones de las estructuras y variables globales de la simulacion.
   \author Daniel Mateo Moreno
   \author Diego Troyano Lopez
   \date 27/04/2019
*/

#ifndef SRC_SIMULADOR_H_
#define SRC_SIMULADOR_H_

#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>
#include <mqueue.h>

#define N_EQUIPOS 3 // Número de equipos
#define N_NAVES 3 // Número de naves por equipo

/*** SCREEN ***/
extern char symbol_equipos[N_EQUIPOS]; // Símbolos de los diferentes equipos en el mapa (mirar mapa.c)
#define MAPA_MAXX 20 // Número de columnas del mapa
#define MAPA_MAXY 20 // Número de filas del mapa
#define SCREEN_REFRESH 10000 // Frequencia de refresco del mapa en el monitor
#define SYMB_VACIO '.' // Símbolo para casilla vacia
#define SYMB_TOCADO '%' // Símbolo para tocado
#define SYMB_DESTRUIDO 'X' // Símbolo para destruido
#define SYMB_AGUA 'w' // Símbolo para agua

/*** SIMULACION ***/
#define VIDA_MAX 50 // Vida inicial de una nave
#define ATAQUE_ALCANCE 20 // Distancia máxima de un ataque
#define ATAQUE_DANO 10 // Daño de un ataque
#define MOVER_ALCANCE 1 // Máximo de casillas a mover
#define TURNO_SECS 5 // Segundos que dura un turno


/*** MAPA ***/
// Información de nave
typedef struct {
	int vida; // Vida que le queda a la nave
	int posx; // Columna en el mapa
	int posy; // Fila en el mapa
	int equipo; // Equipo de la nave
	int numNave; // Numero de la nave en el equipo
	bool viva; // Si la nave está viva o ha sido destruida
} tipo_nave;

// Información de una casilla en el mapa
typedef struct {
	char simbolo; // Símbolo que se mostrará en la pantalla para esta casilla
	int equipo; // Si está vacia = -1. Si no, número de equipo de la nave que está en la casilla
	int numNave; // Número de nave en el equipo de la nave que está en la casilla
} tipo_casilla;


typedef struct {
	tipo_nave info_naves[N_EQUIPOS][N_NAVES];
	tipo_casilla casillas[MAPA_MAXY][MAPA_MAXX];
	int num_naves[N_EQUIPOS]; // Número de naves vivas en un equipo
} tipo_mapa;


#define SHM_MAP_NAME "/shm_naves" /* Memoria compartida del proyecto */
#define COLA "/Cola" /* Cola de mensajes del proyecto */
#define ATACAR 0 /* Accion de atacar */
#define MOVER_ALEATORIO 1 /* Accion de moverse a una casilla aleatoria */
#define MSGSIZE 20 /* Tamanio maximo de un mensaje en la cola */
mqd_t cola;
int simular; /* Condicion de los bucles para seguir la simulacion. Si se hace 0, los procesos acaban */
int alarma; /* Flag de la alarma del proceso simulador */

// Informacion de un mensaje enviado en la cola
typedef struct{
	tipo_nave nave[2]; // Nave que va a realizar la accion y la modificada/atacada
	char msg[MSGSIZE]; // Mensaje (MOVER_ALEATORIO/ATACAR)
} tipo_mensaje;

#define SEM_COLA "sem_cola" // Semaforo que regula el transito de mensajes entre nave y simulador
#define SEM_MONITOR "sem_monitor" // Semaforo de espera al inicio entre simulador y monitor

#endif /* SRC_SIMULADOR_H_ */
