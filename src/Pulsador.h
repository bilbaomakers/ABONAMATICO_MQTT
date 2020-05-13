/* 

# Pulsador.h V 1.0

Libreria de clase Pulsador para implementar la lectura de un pulsador o switch en entornos Arduino con configuracion de tiempo de debounce
Author: Diego Maroto - BilbaoMakers 2020 - info@bilbaomakers.org - dmarofer@diegomaroto.net
https://bilbaomakers.org/

Licencia: GNU General Public License v3.0 - https://www.gnu.org/licenses/gpl-3.0.html

*/

#include "Arduino.h"

class Pulsador{


private:

 
    unsigned long millis_debounce;

    int pinswitch;

public:

    enum Estado_Debounce {

		EDB_IDLE,
		EDB_PULSADO,
		EDB_DETECTADO_CAMBIO,
		
	}estado_debounce, estado_debounce_anterior;


    unsigned long debouncetime;

    Pulsador(int pin, int modo, unsigned long DebounceTimeMS);
    ~Pulsador(){};

    int LeeEstado();

    void Run();
    

};
