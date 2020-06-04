
#include <IndicadorLed.h>
#include "Arduino.h"


IndicadorLed::IndicadorLed(int l_pin, bool t_Invertir){

    this->pin=l_pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW ^ Invertir);
    EstadoLed = LED_APAGADO;
    EstadoLedCiclos = LED_APAGADO;
    TiempoONPulso = 200;
    TiempoOFFPulso = 1000;
    NumeroPulsos = 1;
    TiempoONCiclo = 200;
    TiempoOFFCiclo = 1000;
    TiempoCiclo = 5000;
    NumeroPulsosCiclo = 1;
    CuentaMillisPulso=millis();
    CuentaMillisCiclo=millis();
    CuentaPulsos = 0;
    Invertir = t_Invertir;

}

void IndicadorLed::Pulsos(unsigned int l_TiempoONPulso, unsigned int l_TiempoOFFPulso, unsigned int l_NumeroPulsos){

    this->TiempoONPulso=l_TiempoONPulso;
    this->TiempoOFFPulso=l_TiempoOFFPulso;
    this->NumeroPulsos=l_NumeroPulsos;
    CuentaPulsos = 0;
    digitalWrite(pin, LOW ^ Invertir);
    EstadoLed=LED_PULSO_OFF;

}

void IndicadorLed::Ciclo(unsigned int l_TiempoONCiclo, unsigned int l_TiempoOFFCiclo, unsigned int l_TiempoCiclo, unsigned int l_NumeroPulsos){


    TiempoONCiclo = l_TiempoONCiclo;
    TiempoOFFCiclo = l_TiempoOFFCiclo;
    TiempoCiclo = l_TiempoCiclo;
    NumeroPulsosCiclo = l_NumeroPulsos;
    EstadoLed = LED_CICLO;
    EstadoLedCiclos = LED_CICLO;
    CuentaMillisCiclo = millis();

}

void IndicadorLed::Encender(){

    digitalWrite(pin, HIGH ^ Invertir);
    EstadoLed = LED_ENCENDIDO;
    EstadoLedCiclos = LED_ENCENDIDO;

}

void IndicadorLed::Apagar(){

    digitalWrite(pin, LOW ^ Invertir);
    EstadoLed = LED_APAGADO;
    EstadoLedCiclos = LED_APAGADO;

}

void IndicadorLed::RunFast(){


    switch (EstadoLed){
    
        case LED_PULSO_ON:

            //Si llevamos Encendidos mas del tiempo encendido
            if ((millis() - CuentaMillisPulso) > TiempoONPulso){

                // Apagar, actualizar el cuentatiempos y el estado y contar un ciclo hecho
                digitalWrite(pin, LOW ^ Invertir);
                CuentaMillisPulso = millis();
                EstadoLed = LED_PULSO_OFF;
                if (NumeroPulsos != 0){ CuentaPulsos++;}
             
            }

            
        break;

        case LED_PULSO_OFF:

            // Si llevamos apagados mas del tiempo de apagado
            if ((millis() - CuentaMillisPulso) > TiempoOFFPulso){

                // Si quedan pulsos por hacer
                if (CuentaPulsos < NumeroPulsos || NumeroPulsos == 0){
                    
                    
                    digitalWrite(pin, HIGH ^ Invertir);      // ENCENDER
                    CuentaMillisPulso = millis();                // ACTUALIZAR EL TIEMPO DEL CONTADOR
                    EstadoLed = LED_PULSO_ON;                // PASAR A ESTE ESTADO QUE HARA OTRO CICLO

                }

                else{

                    CuentaMillisPulso = millis();       // ACTUALIZAR EL TIEMPO DEL CONTADOR                    
                   
                    EstadoLed=EstadoLedCiclos;      // Volver el led al estado que estaba antes de los pulsos

                    switch (EstadoLedCiclos){       
                    
                        case LED_APAGADO:

                            digitalWrite(pin, LOW ^ Invertir);      // APAGAR

                        break;

                        case LED_ENCENDIDO:

                            digitalWrite(pin, HIGH ^ Invertir);      // ENCENDER

                        break;

                        default:
                        break;

                    }

                }
                
             
            }
            
        break;

        case LED_CICLO:

        if ((millis() - CuentaMillisCiclo) > TiempoCiclo){

            this->Pulsos(TiempoONCiclo, TiempoOFFCiclo, NumeroPulsosCiclo);
            CuentaMillisCiclo = millis();

        }


        break;

        default:
        break;

    
    }



}