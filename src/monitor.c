/*!
   \file monitor.c
   \brief Codigo correspondiente al proceso monitor que imprime el estado del mapa.
   \author Daniel Mateo Moreno
   \author Diego Troyano Lopez
   \date 27/04/2019
*/

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include <simulador.h>
#include <gamescreen.h>
#include <mapa.h>
#include "manejadores.h"


void mapa_print(tipo_mapa *mapa)
{
	int i,j;

	for(j=0;j<MAPA_MAXY;j++) {
		for(i=0;i<MAPA_MAXX;i++) {
			tipo_casilla cas=mapa_get_casilla(mapa,j, i);
			//printf("%c",cas.simbolo);
			screen_addch(j, i, cas.simbolo);
		}
		printf("\n");
	}
	screen_refresh();
}


int main() {
	int fd_mapa; /* Descriptor de memoria */
	tipo_mapa *mem_mapa; /* Memoria compartida con el mapa */
	simular = 1; /* Condicion de los bucles para la simulacion activa */
	struct sigaction act; /* Mascara de seniales */
	sem_t *sem_monitor=NULL; /* Semaforo que espera al simulador */

/********************************************************SEMAFOROS***************************************************************************/
	if((sem_monitor = sem_open(SEM_MONITOR, O_EXCL, S_IRUSR | S_IWUSR, 0))==SEM_FAILED){ /* Creamos el semaforo de la gestion monitor-simulador */
	  if((sem_monitor = sem_open(SEM_MONITOR, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0))==SEM_FAILED){
	    fprintf(stderr, "ERROR AL CREAR EL SEMAFORO MONITOR-SIMULADOR. ERROR: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	  }
	}
/********************************************************************************************************************************************/

	sem_wait(sem_monitor); /* Espera a que el simulador cree el mapa */

/******************************************************MEMORIA COMPARTIDA********************************************************************/
	fd_mapa = shm_open(SHM_MAP_NAME, O_RDWR , S_IRUSR | S_IWUSR); /* Abrimos la memoria compartida con el estado del mapa */
	if(fd_mapa==-1){
		fprintf(stderr, "ERROR DE RESERVA DE MEMORIA COMPARTIDA. ERROR: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	mem_mapa = (tipo_mapa*)mmap(NULL, sizeof(tipo_mapa), PROT_READ | PROT_WRITE, MAP_SHARED, fd_mapa, 0); /* Mapeamos el segmento de la memoria */
	if(mem_mapa == MAP_FAILED){
		 fprintf(stderr, "ERROR AL MAPEAR LA MEMORIA. ERROR: %s\n", strerror(errno));
		 shm_unlink(SHM_MAP_NAME);
		 exit(EXIT_FAILURE);
	}
	close(fd_mapa); /* Cerramos el descriptor de fichero de memoria compartida */
/********************************************************************************************************************************************/

/**********************************************************SENIALES**************************************************************************/
	sigemptyset(&(act.sa_mask)); /* Declaramos la mascara de seniales */
	act.sa_flags = 0;

	act.sa_handler = manejador_Control_C; /* Capturamos la senial SIGINT cuando se realiza un Control+C */
	if(sigaction(SIGINT, &act, NULL)<0){
		fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGINT: %s\n", strerror(errno));
	}
/********************************************************************************************************************************************/

/*********************************************BUCLE SIMULACION*******************************************************************************/
	while(simular){ /* No acaba hasta recibir un control+C */
		screen_init();
		mapa_print(mem_mapa); /* Imprime el estado del mapa */
		usleep(SCREEN_REFRESH);
		screen_end();
	}
/********************************************************************************************************************************************/

/****************************************LIBERACION Y FINALIZACION MONITOR*******************************************************************/
	munmap(mem_mapa, sizeof(mem_mapa)); /* Liberamos la memoria compartida */

	exit(EXIT_SUCCESS);
/********************************************************************************************************************************************/
}
