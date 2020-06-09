/*!
   \file manejadores.h
   \brief Codigo correspondiente a las declaraciones de los manejadores utilizados en el proyecto y a otras funciones importantes.
   \author Daniel Mateo Moreno
   \author Diego Troyano Lopez
   \date 27/04/2019
*/

#ifndef SRC_MANEJADORES_H_
#define SRC_MANEJADORES_H_


/*!
   \brief Manejador del simulador cuando se realiza Control+C (senial SIGINT).
	        Modifica la condicion del bucle para que acabe el proceso simulador.
          Envia el mensaje de FIN a los jefes.
   \param sig Senial
   \return void
*/
void manejador_Control_C(int sig);


/*!
   \brief Manejador de la nave cuando recibe la senial SIGTERM.
	        Modifica la condicion de los bucles para que acaben todos los procesos nave.
   \param sig Senial
   \return void
*/
void manejador_SIGTERM(int sig);

/*!
   \brief Manejador del simulador cuando la alarma se activa.
	        Modifica la condicion de alarma para que avise a los jefes.
   \param sig Senial
   \return void
*/
void manejador_ALARM(int sig);

/*!
   \brief Funcion que devuelve un entero aleatorio que se asocia a un tipo de accion que enviara un proceso jefe a sus naves.
   \return int Entero entre 0 y 1.
*/
int generador_accion_aleatoria();

/*!
   \brief Genera una coordenada X-Y contigua aleatoria a la que podria moverse la nave.
   \param nave Nave con la informacion de las nuevas coordenadas.
   \return void
*/
void mover_aleatorio(tipo_nave *nave);

/*!
   \brief Localiza una nave a rango de ataque a la que posiblemente se atacara.
   \param mapa Informacion del mapa.
   \param nave Nave inicial que quiere atacar.
   \return naveL Nave localizada.
*/
tipo_nave localizar_nave(tipo_mapa *mapa, tipo_nave nave);

/*!
   \brief Devuelve el equipo ganador de la partida.
   \param mapa Informacion del mapa.
   \return winner Numero equipo ganador, -2 si no quedan naves vivas y -1 si no hay ganador.
*/
int there_is_a_winner(tipo_mapa *mapa);

/*!
   \brief Imprime el equipo ganador por pantalla.
   \param winner Numero equipo ganador.
   \return void
*/
void imprimir_ganador(int winner);

/*!
   \brief Imprime la accion de atacar (una nave a otra) por pantalla.
   \param mapa Informacion del mapa.
   \param nave1 Nave que ataca.
   \param nave2 Nave que es atacada.
   \return void
*/
void imprimir_ataque(tipo_mapa *mapa, tipo_nave nave1, tipo_nave nave2);

/*!
   \brief Imprime la accion de mover una nave a otra casilla por pantalla.
   \param mapa Informacion del mapa.
   \param nave1 Nave antes de moverse.
   \param nave2 Nave tras haberse movido.
   \return void
*/
void imprimir_mover(tipo_mapa *mapa, tipo_nave nave1, tipo_nave nave2);

#endif /* SRC_MANEJADORES_H_ */
