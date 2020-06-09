/*!
   \file jefe.c
   \brief Codigo correspondiente a un proceso jefe, coordinador de los procesos naves.
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

/*!
   \brief Funcion principal de un proceso jefe.
   \return int terminacion del proceso
*/
int main(int argc, char const *argv[]) {
/****************************************************DECLARACION DE VARIABLES**************************************************************/
  int fd[2]; // Descriptor de tuberia
  int fdx[N_NAVES]; // Array de descriptores de las tuberias
  int pipe_status; // Estado tuberia
  int i, j, k; // Variable para iterar
  char nNave[] = "  ";  // Numero de nave en el equipo
  pid_t pid; // Id del proceso
  struct sigaction act; /* Mascara de seniales */
  char instruccion[MSGSIZE]; /* Instruccion transmitida por el simulador */
  int naveADestruir; /* Numero de nave que va a ser destruida */
  simular = 1; /* Condicion de los bucles para la simulacion activa */
  int num_aleat; /* Numero aleatorio indicando la accion a tomar */
  char destruir[] = "DESTRUIR"; /* Accion enviada DESTRUIR */
  char atacar[] = "ATACAR"; /* Accion enviada ATACAR */
  char mover[] = "MOVER_ALEATORIO"; /* Accion enviada MOVER_ALEATORIO */
  sem_t *sem_naves[N_NAVES*N_EQUIPOS]; /* Semaforos de coordinacion entre jefes y naves */
  char nombreSem[11]; /* Nombre de los semaforos jefe-nave */
/******************************************************************************************************************************************/

  if(argc!=3){
    fprintf(stderr, "ERROR DE ARGUMENTOS. SE PIDE: ./jefe nEquipo nNave\n");
    exit(EXIT_FAILURE);
  }

  fprintf(stdout, "--Proceso jefe del equipo %d creado--\n", atoi(argv[1]));

/********************************************************SEMAFOROS***************************************************************************/
  for(i=0;i<N_NAVES*N_EQUIPOS;i++){ /* Abrimos los semaforos de la gestion jefe-naves */
    sprintf(nombreSem, "sem_naves%d", i);
    if((sem_naves[i] = sem_open(nombreSem, O_EXCL, S_IRUSR | S_IWUSR, 0))==SEM_FAILED){
      fprintf(stderr, "ERROR AL CREAR EL SEMAFORO JEFE-NAVE %d. ERROR: %s\n", i, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
/********************************************************************************************************************************************/

/*****************************************************CREACION DE NAVES********************************************************************/
  j = atoi(argv[2]);

  for(i=0;i<N_NAVES;i++){ /* Bucle de creacion de naves */

		pipe_status = pipe(fd); /* Abrimos una tubería */
		if(pipe_status==-1){
			fprintf(stderr, "ERROR AL ABRIR LA TUBERIA: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

    fdx[i] = fd[1]; /* Guardamos los primeros descritores de las tuberias en un array de descriptores */

		pid = fork(); /* Creamos un proceso nave */
		if(pid<0){
			fprintf(stderr, "ERROR EN EL FORK: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		else if(pid==0){

			close(fd[1]);
      if(dup2(fd[0], STDIN_FILENO)==-1){ /* Hacemos que el fichero de entrada de la nave sea el segundo descriptor de la tuberia */
        fprintf(stderr, "ERROR EN EL DUP2: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }

      sprintf(nNave, "%d", j+i);
      close(fd[0]);
			execl("./nave", "./nave", argv[1], nNave, NULL);  /* Enviamos los hijos naves a su correspondiente codigo */

		}
		else{
      close(fd[0]);
		}

	}
/******************************************************************************************************************************************/

/*********************************************CAPTURA DE SENIALES**************************************************************************/
  sigemptyset(&(act.sa_mask)); /* Declaramos la mascara de seniales */
  act.sa_flags = 0;

  act.sa_handler = SIG_IGN; /* Capturamos la senial SIGINT cuando se realiza un Control+C y la ignoramos */
	if(sigaction(SIGINT, &act, NULL)<0){
		fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGINT: %s\n", strerror(errno));
	}

  act.sa_handler = SIG_IGN; /* Capturamos la senial SIGTERM cuando se envia a todos los hijos y al jefe */
  if(sigaction(SIGTERM, &act, NULL)<0){
    fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGINT: %s\n", strerror(errno));
  }
/******************************************************************************************************************************************/

/*********************************************BUCLE SIMULACION*****************************************************************************/
  while(simular){ /* Bucle que no acaba hasta recibir la instruccion FIN */

    memset(instruccion, 0, MSGSIZE);
    read(STDIN_FILENO, instruccion, MSGSIZE); /* Leemos la instruccion de la tuberia */
    if(strcmp("FIN", instruccion)==0){ /* Comprueba si la instruccion enviada por el simulador es FIN */
      simular = 0;  /* No repite el bucle. Acaba. */
      kill(0, SIGTERM); /* Enviamos la senial de interrupcion a todas las naves */
    }
    else if(strcmp("TURNO", instruccion)==0){ /* Comprueba si la instruccion enviada por el simulador es TURNO */
      for(k=0;k<2;k++){ /* Bucle para enviar dos instrucciones a cada nave */
        num_aleat = generador_accion_aleatoria(); /* Generamos un tipo de accion */
        if(num_aleat==ATACAR){ /* La accion es atacar */
          for(i=0; i<N_NAVES; i++){ /* Enviamos la accion ATACAR a las naves */
            write(fdx[i], atacar, sizeof(atacar)); /* Escribimos la instruccion en la tuberia de cada nave */
            sem_wait(sem_naves[i+j-1]); /* Esperamos a que la instruccion haya sido leida y enviada al simulador por esa nave */
          }
        }
        else if(num_aleat==MOVER_ALEATORIO){ /* La accion es mover_aleatorio */
          for(i=0; i<N_NAVES; i++){ /* Enviamos la accion MOVER_ALEATORIO a las naves */
            write(fdx[i], mover, sizeof(mover)); /* Escribimos la instruccion en la tuberia de cada nave */
            sem_wait(sem_naves[i+j-1]); /* Esperamos a que la instruccion haya sido leida y enviada al simulador por esa nave */
          }
        }
        else;
      }
    }
    else if(strcmp(instruccion, "DESTRUIR")>0) { /* Comprueba si la instruccion enviada por el simulador es DESTRUIR */
      naveADestruir = (int)instruccion[9];
      write(fdx[naveADestruir], destruir, sizeof(destruir)); /* Enviamos la accion DESTRUIR a la nave indicada */
    }
    else{

    }

  }
/*****************************************************************************************************************************************/

/*****************************************RECOGIDA Y FINALIZACION DE NAVES****************************************************************/
  for(i=0;i<N_NAVES;i++){ // Esperamos a que los procesos nave terminen
    close(fdx[i]);
    wait(NULL);
		fprintf(stdout, "Un proceso nave del equipo %d recogido\n", atoi(argv[1]));
	}
/*****************************************************************************************************************************************/

/****************************************LIBERACION Y FINALIZACION JEFE*******************************************************************/
  for(i=0;i<N_NAVES*N_EQUIPOS;i++){ /* Bucle de cierre de los semaforos */
    sem_close(sem_naves[i]);
  }

  exit(EXIT_SUCCESS); /* Acaba el proceso jefe */
/*****************************************************************************************************************************************/
}
