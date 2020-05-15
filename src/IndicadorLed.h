#include "Arduino.h"

class IndicadorLed{


private:

    int pin;
    unsigned int TiempoON;
    unsigned int TiempoOFF;
    unsigned int NumeroPulsos;

    unsigned long CuentaMillis1;
    unsigned int CuentaCiclos;

    bool Invertir;


public:

    enum TipoEstadoLed {

        LED_APAGADO,
        LED_ENCENDIDO,
        LED_PULSO_ON,
        LED_PULSO_OFF,

    }EstadoLed;

    IndicadorLed(int l_pin, bool l_Invertir);
    ~IndicadorLed(){};

    void Pulso(unsigned int l_TiempoON, unsigned int l_TiempoOFF, unsigned int l_NumeroPulsos);
    void Encender();
    void Apagar();

    void RunFast();

};



