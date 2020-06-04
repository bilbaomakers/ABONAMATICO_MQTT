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

    bool invertir;

public:

    enum Estado_Salida {

		EDB_IDLE,
		EDB_PULSADO,
        EDB_HOLD,
		EDB_DETECTADO_CAMBIO,
        		
	}Estado_Salida;


    unsigned long debouncetime;
    unsigned long holdtime;

    Pulsador(int pin, int modo, unsigned long DebounceTimeMS, unsigned long HoldTimeMS, bool invert);
    ~Pulsador(){};

    int LeeEstado();

    void Run();
    

};
