/*!
   \file manejadores.c
   \brief Codigo correspondiente a los manejadores utilizados en el proyecto y a otras funciones importantes.
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
#include <time.h>

#include <mapa.h>
#include "manejadores.h"

/***************************************MANEJADORES***********************************************/

void manejador_Control_C(int sig){
	fprintf(stdout, "\nEl simulador ha recibido la senial SIGINT. Terminando todos los procesos...\n");
	simular = 0;
}

void manejador_SIGTERM(int sig){
  fprintf(stdout, "Una nave ha recibido la senial SIGTERM. Terminando...\n");
  simular = 0;
}

void manejador_ALARM(int sig){
  fprintf(stdout, "\n\n");
  alarma = 1;
}

/*************************************************************************************************/

/*************************************OTRAS FUNCIONES*********************************************/

int generador_accion_aleatoria(){
	int num;
	srand(getpid()+time(NULL));
	num = rand()%2;
	return num;
}

void mover_aleatorio(tipo_nave *nave){
	int num, x, y, cond=1;

	x = nave->posx;
	y = nave->posy;

	srand(getpid()+time(NULL));

	while(cond){ /* Busca moverse en una direccion una casilla */
		num = rand()%4;

		if(num==0){
			if(x+1!=MAPA_MAXX){
				nave->posx++;
			}
		}
		else if(num==1){
			if(x-1!=-1){
				nave->posx--;
			}
		}
		else if(num==2){
			if(y+1!=MAPA_MAXX){
				nave->posy++;
			}
		}
		else{
			if(y-1!=-1){
				nave->posy--;
			}
		}

		if(x!=nave->posx || y!=nave->posy){ /* Si sigue en su posicion inicial, busca otra */
			cond = 0;
		}

	}

}

tipo_nave localizar_nave(tipo_mapa *mapa, tipo_nave nave){
	int i, j;
	tipo_nave naveL;

	for(i=nave.posy;i<MAPA_MAXY;i++){
		for(j=nave.posx;j<MAPA_MAXX;j++){
			if(mapa->casillas[j][i].equipo!=-1){
				if(mapa->info_naves[mapa->casillas[j][i].equipo][mapa->casillas[j][i].numNave].equipo!=nave.equipo && mapa->info_naves[mapa->casillas[j][i].equipo][mapa->casillas[j][i].numNave].viva==true){
					naveL = mapa->info_naves[mapa->casillas[j][i].equipo][mapa->casillas[j][i].numNave];
					return naveL;
				}
			}
		}
	}

	for(i=nave.posy;i>=0;i--){
		for(j=nave.posx;j>=0;j--){
			if(mapa->casillas[j][i].equipo!=-1){
				if(mapa->info_naves[mapa->casillas[j][i].equipo][mapa->casillas[j][i].numNave].equipo!=nave.equipo && mapa->info_naves[mapa->casillas[j][i].equipo][mapa->casillas[j][i].numNave].viva==true){
					naveL = mapa->info_naves[mapa->casillas[j][i].equipo][mapa->casillas[j][i].numNave];
					return naveL;
				}
			}
		}
	}

	naveL = nave;

	return naveL;
}

int there_is_a_winner(tipo_mapa *mapa){
	int cont=0, winner=-1, i;

	for(i=0;i<N_EQUIPOS;i++){
		if(mapa_get_num_naves(mapa, i)<=0){
			cont++;
		}
		else{
			winner = i;
		}
	}

	if(cont==2){
		return winner;
	}
	else if(cont==3){
		return -2;
	}
	else{
		return -1;
	}

}

void imprimir_ganador(int winner){
	fprintf(stdout, "***** EQUIPO GANADOR %c *****\n", symbol_equipos[winner]);
	fprintf(stdout, "TERMINA LA SIMULACION!\n");
}

void imprimir_ataque(tipo_mapa *mapa, tipo_nave nave1, tipo_nave nave2){
	char equipo1 = symbol_equipos[nave1.equipo];
	char equipo2 = symbol_equipos[nave2.equipo];
	int nNave1 = nave1.equipo*N_EQUIPOS + nave1.numNave;
	int nNave2 = nave2.equipo*N_EQUIPOS + nave2.numNave;
	int x1 = nave1.posx, x2 = nave2.posx;
	int y1 = nave1.posy, y2 = nave2.posy;
	fprintf(stdout," -> ACCION ATACAR [%c%d] %d:%d -> %d:%d [%c%d]\n", equipo1, nNave1, x1, y1, x2, y2, equipo2, nNave2);
}

void imprimir_mover(tipo_mapa *mapa, tipo_nave nave1, tipo_nave nave2){
	char equipo = symbol_equipos[nave1.equipo];
	int nNave = nave1.equipo*N_EQUIPOS + nave1.numNave;
	int x1 = nave1.posx, x2 = nave2.posx;
	int y1 = nave1.posy, y2 = nave2.posy;
	fprintf(stdout," -> ACCION MOVER [%c%d] %d:%d -> %d:%d\n", equipo, nNave, x1, y1, x2, y2);
}

/*************************************************************************************************/
