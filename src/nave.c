/*!
   \file nave.c
   \brief Codigo correspondiente a un proceso nave.
   \author Daniel Mateo Moreno
   \author Diego Troyano Lopez
   \date 15/04/2019
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

#include <mapa.h>
#include "manejadores.h"

struct mq_attr attributes = { /* Atributos de un mensaje */
  .mq_flags = 0,
  .mq_maxmsg = 10,
  .mq_curmsgs = 0,
  .mq_msgsize = sizeof(tipo_mensaje)
};

/*!
   \brief Funcion principal de un proceso nave.
   \return int terminacion del proceso
*/
int main(int argc, char const *argv[]) {
/****************************************************DECLARACION DE VARIABLES**************************************************************/
  struct sigaction act; /* Mascara de seniales */
  int numeroNave, numeroEquipo;
  simular = 1; /* Condicion de los bucles para la simulacion activa */
  int i;
  char instruccion[MSGSIZE];
  tipo_mensaje msg;
  char mover[] = "MOVER_ALEATORIO";
  char atacar[] = "ATACAR";
  tipo_nave nave;
  int fd_mapa;
  tipo_mapa *mem_mapa;
  sem_t *sem_naves[N_NAVES*N_EQUIPOS]; /* Semaforos de coordinacion entre jefes y naves */
  sem_t *sem_cola=NULL;
  char nombreSem[11]; /* Nombre de los semaforos jefe-nave */
/********************************************************************************************************************************************/

  if(argc!=3){
    fprintf(stderr, "ERROR DE ARGUMENTOS. SE PIDE: ./nave nEquipo nNave\n");
    exit(EXIT_FAILURE);
  }
  numeroNave = atoi(argv[2]);
  numeroEquipo = atoi(argv[1]);

/********************************************************SEMAFOROS***************************************************************************/
  for(i=0;i<N_NAVES*N_EQUIPOS;i++){  /* Abrimos los semaforos de la gestion jefe-naves */
    sprintf(nombreSem, "sem_naves%d", i);
    if((sem_naves[i] = sem_open(nombreSem, O_EXCL, S_IRUSR | S_IWUSR, 0))==SEM_FAILED){
      fprintf(stderr, "ERROR AL CREAR EL SEMAFORO JEFE-NAVE %d. ERROR: %s\n", i, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  if((sem_cola = sem_open(SEM_COLA, O_EXCL, S_IRUSR | S_IWUSR, 1))==SEM_FAILED){ /* Creamos el semaforo de la gestion naves-simulador */
    fprintf(stderr, "ERROR AL CREAR EL SEMAFORO DE LA COLA. ERROR: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
/********************************************************************************************************************************************/

/****************************************************MEMORIA COMPARTIDA**********************************************************************/
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

/*****************************************************COLA DE MENSAJES***********************************************************************/
  cola = mq_open(COLA, O_RDWR, S_IRUSR | S_IWUSR, &attributes); /* Abrimos la cola de mensajes */
  if(cola==(mqd_t)-1){
    fprintf(stderr, "ERROR ABRIENDO LA COLA DE MENSAJES. ERROR: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
/*******************************************************************************************************************************************/

  fprintf(stdout, "++Proceso nave %d del equipo %d creado++\n", numeroNave, numeroEquipo);

/*********************************************CAPTURA DE SENIALES***************************************************************************/
  sigemptyset(&(act.sa_mask)); /* Declaramos la mascara de seniales */
  act.sa_flags = 0;

  act.sa_handler = SIG_IGN; /* Capturamos la senial SIGINT cuando se realiza un Control+C y la ignoramos */
	if(sigaction(SIGINT, &act, NULL)<0){
		fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGINT: %s\n", strerror(errno));
	}

  act.sa_handler = manejador_SIGTERM; /* Capturamos la senial SIGTERM enviada por el jefe */
	if(sigaction(SIGTERM, &act, NULL)<0){
		fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGTERM: %s\n", strerror(errno));
	}
/*******************************************************************************************************************************************/

/*********************************************BUCLE SIMULACION******************************************************************************/
  while(simular){ /* Bucle que no acaba hasta un ganador o Control+C */

    memset(instruccion, 0, MSGSIZE);
    read(STDIN_FILENO, instruccion, sizeof(instruccion)); /* Leemos la instruccion de la tuberia */
    if(strcmp("ATACAR", instruccion)==0){ /* Comprueba si la instruccion enviada por el simulador es ATACAR */
        nave = mapa_get_nave(mem_mapa, numeroEquipo-1, numeroNave-(N_NAVES)*(numeroEquipo-1)-1); /* Nave actual */
        msg.nave[0] = nave;
        nave = localizar_nave(mem_mapa, nave); /* Localizamos una nave a la que atacar */
        msg.nave[1] = nave;
        msg.nave[1].vida = msg.nave[1].vida - ATAQUE_DANO; /* Restamos el danio a la posible nave atacada */
        strcpy(msg.msg, atacar);
        sem_wait(sem_cola); /* Esperan las naves a que el simulador gestione una instruccion ya enviada en la cola */
      	if(mq_send(cola, (char*)&msg, sizeof(msg), 1)==-1){ /* Enviamos la accion al simulador por la cola */
		        fprintf(stderr, "ERROR EN EL ENVIO DEL MENSAJE ATACAR: %s\n", strerror(errno));
            exit(EXIT_SUCCESS);
        }
        memset(msg.msg, 0, MSGSIZE);
        sem_post(sem_naves[numeroNave-1]); /* Da paso a la escritura en la tuberia de la segunda instruccion de la nave */
    }
    else if(strcmp("MOVER_ALEATORIO", instruccion)==0){ /* Comprueba si la instruccion enviada por el simulador es MOVER_ALEATORIO */
        nave = mapa_get_nave(mem_mapa, numeroEquipo-1, numeroNave-(N_NAVES)*(numeroEquipo-1)-1);  /* Nave actual */
        msg.nave[0] = nave;
        mover_aleatorio(&nave); /* Obtenemos la nave tras mover aleatoriamente */
        if(mapa_is_casilla_vacia(mem_mapa, nave.posy, nave.posx)){ /* Comprueba que la casilla sea vacia */
          msg.nave[1] = nave;
          strcpy(msg.msg, mover);
          sem_wait(sem_cola); /* Esperan las naves a que el simulador gestione una instruccion ya enviada en la cola */
          if(mq_send(cola, (char*)&msg, sizeof(msg), 1)==-1){ /* Enviamos la accion al simulador por la cola */
              fprintf(stderr, "ERROR EN EL ENVIO DEL MENSAJE MOVER_ALEATORIO: %s\n", strerror(errno));
              exit(EXIT_SUCCESS);
          }
          memset(msg.msg, 0, MSGSIZE);
        }
        sem_post(sem_naves[numeroNave-1]); /* Da paso a la escritura en la tuberia de la segunda instruccion de la nave */
    }
    else if(strcmp("DESTRUIR", instruccion)==0){ /* Comprueba si la instruccion enviada por el simulador es DESTRUIR */
        simular = 0; /* No repite el bucle. Acaba. */
    }
    else;

  }
/********************************************************************************************************************************************/

/****************************************LIBERACION Y FINALIZACION NAVE**********************************************************************/
  for(i=0;i<N_NAVES*N_EQUIPOS;i++){ /* Bucle de cierre de semaforos */
    sem_close(sem_naves[i]);
  }

  sem_close(sem_cola); /* Cerramos el semaforo */
  munmap(mem_mapa, sizeof(mem_mapa)); /* Cerramos la memoria compartida */

  exit(EXIT_SUCCESS);
/********************************************************************************************************************************************/
}
