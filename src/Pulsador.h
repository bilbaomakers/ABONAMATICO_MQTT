
#include "Arduino.h"

class Pulsador{


private:

 
    unsigned long millis_debounce_swhome;

    int pinswitch;

public:

    enum Estado_Debounce {

		EDB_IDLE,
		EDB_PULSADO,
		EDB_DETECTADO_CAMBIO,
		
	}estado_debounce_swhome;


    unsigned long debouncetime;

    Pulsador(int pin, int modo);
    ~Pulsador(){};

    int LeeEstado();

    void Run();
    

};
