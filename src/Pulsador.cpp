/* 

# Pulsador.h V 1.0

Libreria de clase Pulsador para implementar la lectura de un pulsador o switch en entornos Arduino con configuracion de tiempo de debounce
Author: Diego Maroto - BilbaoMakers 2020 - info@bilbaomakers.org - dmarofer@diegomaroto.net
https://bilbaomakers.org/

Licencia: GNU General Public License v3.0 - https://www.gnu.org/licenses/gpl-3.0.html

*/

#include <Pulsador.h>
#include "Arduino.h"


Pulsador::Pulsador(int pin, int modo, unsigned long DebounceTimeMS, unsigned long HoldTimeMS, bool invert){
   	
    pinswitch = pin;
	pinMode(pinswitch, modo);
	Estado_Salida = EDB_IDLE;
	millis_debounce = 0;
	debouncetime = DebounceTimeMS;
	holdtime = HoldTimeMS;
	invertir=invert;

}


int Pulsador::LeeEstado (){

    return Estado_Salida;

}


void Pulsador::Run(){

   	// Leo fisicamente el switch
	int l_lecturaswhome = digitalRead(pinswitch) ^ invertir;

	// Si he leido un estado distinto de que se supone que tenia el Switch (el estado del switch es una enum, 0,1,2).... 
	// Estoy comparando un bool con un int pero teoricamente si es 2 pues tampoco es ni 0 ni 1, sera suficientemente listo?
	if (l_lecturaswhome != Estado_Salida && Estado_Salida != EDB_DETECTADO_CAMBIO && Estado_Salida != EDB_HOLD){

		millis_debounce = millis();
		Estado_Salida = EDB_DETECTADO_CAMBIO;

	}

	switch(Estado_Salida){	

		// Si esta en DETECTADO_CAMBIO (o sea Debouncing)
		case EDB_DETECTADO_CAMBIO:

			// Mirar si ha pasado el
			if ((millis() - millis_debounce) > debouncetime){
		
				// Aqui ya no es tan listo, a la hora de asignar no le gustan los tipos asi que
				switch (l_lecturaswhome){

					case LOW:

						Estado_Salida = EDB_IDLE;
						
						break;

					case HIGH:

						Estado_Salida = EDB_PULSADO;
						
						break;

				}

			}

		break;

		case EDB_PULSADO:

			if (l_lecturaswhome == HIGH && (millis() - millis_debounce) > holdtime && holdtime != 0){

				Estado_Salida = EDB_HOLD;

			}
			
		break;

		case EDB_HOLD:

			if (l_lecturaswhome == LOW && (millis() - millis_debounce) > holdtime){

				Estado_Salida = EDB_IDLE;

			}
	
		}


}