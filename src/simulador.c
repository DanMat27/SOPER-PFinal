/*!
   \file simulador.c
   \brief Codigo correspondiente al proceso simulador, encargado de coordinar la simulacion e inicializar recursos.
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
   \brief Funcion principal del proceso simulador.
   \return int terminacion del proceso
*/
int main() {
/****************************************************DECLARACION DE VARIABLES**************************************************************/
	pid_t pid; // Id del proceso
	int fd[2]; // Descriptor de tuberia
	int fdx[N_EQUIPOS]; // Array de descriptores de las tuberias
	int pipe_status; // Estado tuberia
	int i, j=1, k; // Variables para iterar
	int x=8, y=0; // Variables para coordenadas
	char nEquipo[] = " "; // Numero de equipo de un jefe
	char nNave[] = " "; // Numero de la primera nave de cada equipo
	char fin[] = "FIN"; /* Accion enviada FIN */
	char turno[] = "TURNO"; /* Accion enviada TURNO */
	char destruir[MSGSIZE]; /* Accion enviada DESTRUIR <NAVE> */
	int fd_mapa; // Descriptor de la memoria compartida
	int error; // Variable de control de errores
	tipo_mapa *mem_mapa=NULL; // Estructura para reservar memoria compartida
	tipo_nave nave; // Estructura con la info de una nave para el mapa
  tipo_mensaje msg; // Estructura con la info de un mensaje recibido de la cola
	struct sigaction act; /* Mascara de seniales */
	simular = 1; /* Condicion de los bucles para la simulacion activa */
	alarma = 0; /* Condicion cuando suena la alarma */
  sem_t *sem_monitor=NULL, *sem_cola=NULL; /* Semaforo de la cola */
  sem_t *sem_naves[N_NAVES*N_EQUIPOS]; /* Semaforos de coordinacion entre jefes y naves */
  char nombreSem[11]; /* Nombre de los semaforos jefe-nave */
  int winner=-1; /* Numero equipo ganador */
/********************************************************************************************************************************************/

/********************************************************SEMAFOROS***************************************************************************/
  if((sem_monitor = sem_open(SEM_MONITOR, O_EXCL, S_IRUSR | S_IWUSR, 0))==SEM_FAILED){ /* Creamos el semaforo de la gestion monitor-simulador */
    if((sem_monitor = sem_open(SEM_MONITOR, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0))==SEM_FAILED){
      fprintf(stderr, "ERROR AL CREAR EL SEMAFORO MONITOR-SIMULADOR. ERROR: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  if((sem_cola = sem_open(SEM_COLA, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1))==SEM_FAILED){ /* Creamos el semaforo de la gestion naves-simulador */
    fprintf(stderr, "ERROR AL CREAR EL SEMAFORO DE LA COLA. ERROR: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for(i=0;i<N_NAVES*N_EQUIPOS;i++){ /* Creamos los semaforos de la gestion jefe-naves */
    sprintf(nombreSem, "sem_naves%d", i);
    if((sem_naves[i] = sem_open(nombreSem, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0))==SEM_FAILED){
      fprintf(stderr, "ERROR AL CREAR EL SEMAFORO JEFE-NAVE %d. ERROR: %s\n", i, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
/********************************************************************************************************************************************/

/******************************************************MEMORIA COMPARTIDA********************************************************************/
	fd_mapa = shm_open(SHM_MAP_NAME, O_CREAT | O_RDWR , S_IRUSR | S_IWUSR); /* Creamos la memoria compartida con el estado del mapa */
	if(fd_mapa==-1){
	  fprintf(stderr, "ERROR DE RESERVA DE MEMORIA COMPARTIDA. ERROR: %s\n", strerror(errno));
	  exit(EXIT_FAILURE);
	}

	error = ftruncate(fd_mapa, sizeof(tipo_mapa)); /* Asignamos el tamanio a la memoria compartida */
  if(error==-1){
    fprintf(stderr, "ERROR AL REASIGNAR EL TAMANIO DE LA MEMORIA COMPARTIDA. ERROR: %s\n", strerror(errno));
    shm_unlink(SHM_MAP_NAME);
    exit(EXIT_FAILURE);
  }

	mem_mapa = (tipo_mapa*)mmap(NULL, sizeof(tipo_mapa), PROT_READ | PROT_WRITE, MAP_SHARED, fd_mapa, 0); /* Mapeamos el segmento de la memoria */
	if(mem_mapa == MAP_FAILED){
		 fprintf(stderr, "ERROR AL MAPEAR LA MEMORIA. ERROR: %s\n", strerror(errno));
		 shm_unlink(SHM_MAP_NAME);
		 exit(EXIT_FAILURE);
	}
	close(fd_mapa); /* Cerramos el descriptor de fichero de memoria compartida */

	for(i=0;i<MAPA_MAXX;i++){ /* Inicializamos las casillas del mapa */
		for(k=0;k<MAPA_MAXY;k++){
			mapa_clean_casilla(mem_mapa, k, i);
		}
	}

	for(i=0;i<N_EQUIPOS;i++){ /* Inicializamos las naves en el mapa */
		for(k=0;k<N_NAVES;k++){
			nave.vida = VIDA_MAX;
			nave.equipo = i;
			nave.numNave = k;
			nave.viva = true;
			nave.posx = x;
			nave.posy = y;
			mapa_set_nave(mem_mapa, nave);
			x++;
		}
		x=8;
		y = y + 8;
		mapa_set_num_naves(mem_mapa, i, N_NAVES);
	}

  sem_post(sem_monitor); /* Damos el paso al monitor */
/********************************************************************************************************************************************/

/*****************************************************COLA DE MENSAJES***********************************************************************/
	cola = mq_open(COLA, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes); /* Creamos la cola de mensajes */
  if(cola==(mqd_t)-1){
    fprintf(stderr, "ERROR CREANDO LA COLA DE MENSAJES. ERROR: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
/********************************************************************************************************************************************/

/*****************************************************CREACION DE JEFES**********************************************************************/
	for(i=0;i<N_EQUIPOS;i++){

		pipe_status = pipe(fd); /* Abrimos una tubería */
		if(pipe_status==-1){
			fprintf(stderr, "ERROR AL ABRIR LA TUBERIA: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		fdx[i] = fd[1]; /* Guardamos los primeros descritores de las tuberias en un array de descriptores */

		pid = fork(); /* Creamos un proceso jefe */
		if(pid<0){
			fprintf(stderr, "ERROR EN EL FORK: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		else if(pid==0){

			close(fd[1]);
			if(dup2(fd[0], STDIN_FILENO)==-1){ /* Hacemos que el fichero de entrada del jefe sea el segundo descriptor de la tuberia */
				fprintf(stderr, "ERROR EN EL DUP2: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			sprintf(nEquipo, "%d", i+1); /* Numero de equipo del nuevo jefe */
			sprintf(nNave, "%d", j); /* Numero de nave a partir del cual comienzan los ids de sus naves */
			close(fd[0]);
			execl("./jefe", "./jefe", nEquipo, nNave, NULL); /* Enviamos los hijos jefes a su correspondiente codigo */

		}
		else{
			close(fd[0]);
			j += N_NAVES; /* Gestion del numero de la nave */
		}

	}
/********************************************************************************************************************************************/

/*********************************************CAPTURA DE SENIALES****************************************************************************/
	sigemptyset(&(act.sa_mask)); /* Declaramos la mascara de seniales */
	act.sa_flags = 0;

	act.sa_handler = manejador_Control_C; /* Capturamos la senial SIGINT cuando se realiza un Control+C */
	if(sigaction(SIGINT, &act, NULL)<0){
		fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGINT: %s\n", strerror(errno));
	}

	act.sa_handler = SIG_IGN; /* Capturamos la senial SIGTERM enviada por los hijos jefe */
	if(sigaction(SIGTERM, &act, NULL)<0){
		fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGINT: %s\n", strerror(errno));
	}

	act.sa_handler = manejador_ALARM; /* Capturamos la senial SIGALARM de la alarma de nuevo turno */
	if(sigaction(SIGALRM, &act, NULL)<0){
		fprintf(stderr, "ERROR EN CAPTURA DE LA SENIAL SIGINT: %s\n", strerror(errno));
	}
/********************************************************************************************************************************************/

/*********************************************BUCLE SIMULACION******************************************************************************/
	alarm(TURNO_SECS); /* Ponemos la alarma 5 segundos cada nuevo turno */
  memset(msg.msg, 0, MSGSIZE);

	while(simular){ /* Bucle que no acaba hasta un ganador o Control+C */

		if(alarma==1){
      mapa_restore(mem_mapa); /* Refrescamos el mapa */
      winner = there_is_a_winner(mem_mapa); /* Comprobamos si hay un equipo ganador */
      if(winner!=-1 && winner!=-2){ /* Si es -1 no hay ganador aun. Si es -2, no quedan naves vivas */
        imprimir_ganador(winner); /* Imprimimos el mensaje de equipo ganador */
        simular = 0;
        alarm(TURNO_SECS); /* Volvemos a poner la alarma */
      }
      else if(winner==-2){
        fprintf(stdout, "*****SE DESTRUYERON TODOS LOS EQUIPOS A LA VEZ!*****\n");
        fprintf(stdout, "TERMINA LA SIMULACION\n");
        simular = 0;
        alarm(TURNO_SECS); /* Volvemos a poner la alarma */
      }
      else{
        alarma = 0;
        for(i=0; i<N_EQUIPOS; i++){ /* Avisamos a los jefes para que terminen enviando la instruccion TURNO por la tuberia */
          write(fdx[i], turno, sizeof(turno)); /* Escribimos la instruccion en las tuberias con los jefes */
        }
        alarm(TURNO_SECS); /* Ponemos la alarma 5 segundos cada nuevo turno */
      }
		}

		if(mq_receive(cola, (char*)&msg, sizeof(msg), 0)==-1){ /* Recibimos las instrucciones de las naves */
			if (errno == EINTR) { /* Evitamos el problema de la interrupcion consecuente de la alarma */

			}
      else{
        fprintf(stderr, "ERROR AL RECIBIR EL MENSAJE: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
		}
    else{
      if(msg.msg[0]=='A'){ /* Comprobamos si la instruccion del mensaje es ATACAR */
        imprimir_ataque(mem_mapa, msg.nave[0], msg.nave[1]);  /* Imprimimos el mensaje de la accion atacar */
        if(mapa_get_distancia(mem_mapa, msg.nave[0].posy, msg.nave[0].posx, msg.nave[1].posy, msg.nave[1].posx)<=ATAQUE_ALCANCE){ /* Comprobamos si el ataque esta al alcance de la nave que dispara */
          if(!mapa_is_casilla_vacia(mem_mapa, msg.nave[1].posy, msg.nave[1].posx)){ /* Comprobamos que la casilla a la que dispara no esté vacia */
              mapa_send_misil(mem_mapa, msg.nave[0].posy, msg.nave[0].posx, msg.nave[1].posy, msg.nave[1].posx); /* Disparamos el misil al objetivo */
              if(mapa_get_nave(mem_mapa, msg.nave[1].equipo, msg.nave[1].numNave).viva){ /* Comprobamos que la nave atacada aun esta viva */
                if(msg.nave[1].vida<=0){  /* Comprobamos si la nave atacada va a ser destruida con el ataque */
                  fprintf(stdout," - NAVE [%c%d] DESTRUIDA -\n", symbol_equipos[msg.nave[1].equipo], msg.nave[1].equipo*N_EQUIPOS + msg.nave[1].numNave);
                  msg.nave[1].viva = false;
                  mapa_set_nave(mem_mapa, msg.nave[1]);  /* Modificamos la nave destruida en el mapa */
                  mapa_clean_casilla(mem_mapa, msg.nave[1].posy, msg.nave[1].posx);
                  mapa_set_symbol(mem_mapa, msg.nave[1].posy, msg.nave[1].posx, SYMB_DESTRUIDO); /* Damos el simbolo destruido a la casilla de la nave derribada */
                  mapa_set_num_naves(mem_mapa, msg.nave[1].equipo, mem_mapa->num_naves[msg.nave[1].equipo]-1); /* Disminuimos el numero de naves vivas del mapa */
                  write(fdx[msg.nave[1].equipo], destruir, sizeof(destruir)); /* Enviamos DESTRUIR al jefe de su equipo */
                  memset(destruir, 0, MSGSIZE);
                }
                else{
                  fprintf(stdout," - NAVE [%c%d] TOCADA. VIDA: %d -\n", symbol_equipos[msg.nave[1].equipo], msg.nave[1].equipo*N_EQUIPOS + msg.nave[1].numNave, msg.nave[1].vida);
                  mapa_set_nave(mem_mapa, msg.nave[1]); /* Modificamos la nave atacada en el mapa */
                  mapa_set_symbol(mem_mapa, msg.nave[1].posy, msg.nave[1].posx, SYMB_TOCADO); /* Damos el simbolo tocado a la casilla de la nave tocada */
                }
              }
          }
          else{ /* Si esta vacia, es AGUA */
            mapa_send_misil(mem_mapa, msg.nave[0].posy, msg.nave[0].posx, msg.nave[1].posy, msg.nave[1].posx);
            mapa_set_symbol(mem_mapa, msg.nave[1].posy, msg.nave[1].posx, SYMB_AGUA);
          }
        }
        memset(msg.msg, 0, MSGSIZE);
        usleep(100000);
        sem_post(sem_cola); /* Dejamos que una nave envie el siguiente mensaje al simulador */
      }
      else if(msg.msg[0]=='M'){ /* Comprobamos si la instruccion del mensaje es  MOVER_ALEATORIO */
        if(mapa_is_casilla_vacia(mem_mapa, msg.nave[1].posy, msg.nave[1].posx)){
          imprimir_mover(mem_mapa,msg.nave[0], msg.nave[1]); /* Imprimimos el mensaje de la accion mover_aletorio */
          mapa_set_nave(mem_mapa, msg.nave[1]); /* Modificamos la nave en el mapa */
          mapa_clean_casilla(mem_mapa, msg.nave[0].posy, msg.nave[0].posx); /* Modificamos la casilla de la que se mueve la nave en el mapa */
        }
        memset(msg.msg, 0, MSGSIZE);
        usleep(100000);
        sem_post(sem_cola); /* Dejamos que una nave envie el siguiente mensaje al simulador */
      }
    }

	}
/********************************************************************************************************************************************/

/*****************************************RECOGIDA Y FINALIZACION DE JEFES*******************************************************************/
	for(i=0; i<N_EQUIPOS; i++){ /* Avisamos a los jefes para que terminen enviando la instruccion FIN por la tuberia */
		write(fdx[i], fin, sizeof(fin));
	}

	for(i=0;i<N_EQUIPOS;i++){ // Esperamos a que los procesos jefe terminen
		close(fdx[i]);
    wait(NULL);
		fprintf(stdout, "Un proceso jefe recogido\n");
	}
/********************************************************************************************************************************************/

/****************************************LIBERACION Y FINALIZACION SIMULADOR*****************************************************************/
	munmap(mem_mapa, sizeof(mem_mapa)); /* Cerramos la memoria compartida */
	shm_unlink(SHM_MAP_NAME); /* Liberamos la memoria compartida */
  sem_close(sem_monitor); /* Cerramos el semaforo */
  sem_unlink(SEM_MONITOR); /* Liberamos el semaforo */
  sem_close(sem_cola); /* Cerramos el semaforo */
  sem_unlink(SEM_COLA); /* Liberamos el semaforo */
	mq_close(cola); /* Cerramos la cola de mensajes */
	mq_unlink(COLA); /* Borramos la cola de mensajes */

  for(i=0;i<N_NAVES*N_EQUIPOS;i++){ /* Bucle de cierre de semaforos */
    sem_close(sem_naves[i]);
    sprintf(nombreSem, "sem_naves%d", i);
    sem_unlink(nombreSem);
  }

	fprintf(stdout, "Proceso simulador terminado\n");
  exit(EXIT_SUCCESS); /* Termina el proceso simulador */
/********************************************************************************************************************************************/
}
