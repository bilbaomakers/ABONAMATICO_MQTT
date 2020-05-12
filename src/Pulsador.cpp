#include <Pulsador.h>
#include "Arduino.h"


Pulsador::Pulsador(int pin, int modo){
   	
    pinswitch = pin;
	pinMode(pinswitch, modo);
	estado_debounce_swhome = EDB_IDLE;
	millis_debounce_swhome = 0;
    debouncetime = 50;

}


int Pulsador::LeeEstado (){

    return estado_debounce_swhome;

}


void Pulsador::Run(){

   	// Leo fisicamente el switch
	int l_lecturaswhome = digitalRead(pinswitch);

	// Si he leido un estado distinto de que se supone que tenia el Switch (el estado del switch es una enum, 0,1,2).... 
	// Estoy comparando un bool con un int pero teoricamente si es 2 pues tampoco es ni 0 ni 1, sera suficientemente listo?
	if (l_lecturaswhome != estado_debounce_swhome){

		//Si no esta en DETECTADO_CAMBIO, pues a detectectado 
		if (l_lecturaswhome != EDB_DETECTADO_CAMBIO){

			millis_debounce_swhome = millis();
			estado_debounce_swhome = EDB_DETECTADO_CAMBIO;

		}
			
		// Y si esta en DETECTADO cambio y ha pasado el tiempo de debounce
		else if ((millis() - millis_debounce_swhome) > debouncetime){

			

			// Aqui ya no es tan listo, a la hora de asignar no le gustan los tipos asi que
			switch (l_lecturaswhome){

				case LOW:

					estado_debounce_swhome = EDB_IDLE;
					break;

				case HIGH:

					estado_debounce_swhome = EDB_PULSADO;
					break;

			}
			
		}

		// Por aqui podriamos implementar otros tiempos de pulsacion de switch

	}

	// Si leo el mismo estado que el que tenia el boton entonces paso del tema.

}