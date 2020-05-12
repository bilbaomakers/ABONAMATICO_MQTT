
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

    Pulsador(int pin, int modo);
    ~Pulsador(){};

    int LeeEstado();

    void Run();
    

};
