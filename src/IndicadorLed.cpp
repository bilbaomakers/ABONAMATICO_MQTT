
#include <IndicadorLed.h>
#include "Arduino.h"



IndicadorLed::IndicadorLed(int l_pin, bool t_Invertir){

    this->pin=l_pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW ^ Invertir);
    EstadoLed = LED_APAGADO;
    TiempoON = 200;
    TiempoOFF = 1000;
    NumeroPulsos = 1;
    CuentaMillis1=millis();
    CuentaCiclos = 0;
    Invertir = t_Invertir;

}

void IndicadorLed::Pulso(unsigned int l_TiempoON, unsigned int l_TiempoOFF, unsigned int l_NumeroPulsos){

    this->TiempoON=l_TiempoON;
    this->TiempoOFF=l_TiempoOFF;
    this->NumeroPulsos=l_NumeroPulsos;
    CuentaCiclos = 0;
    EstadoLed=LED_PULSO_OFF;

}


void IndicadorLed::Encender(){

    digitalWrite(pin, HIGH ^ Invertir);
    EstadoLed = LED_ENCENDIDO;

}

void IndicadorLed::Apagar(){

    digitalWrite(pin, LOW ^ Invertir);
    EstadoLed = LED_APAGADO;

}

void IndicadorLed::RunFast(){


    switch (EstadoLed){
    
        case LED_PULSO_ON:

            //Si llevamos Encendidos mas del tiempo encendido
            if ((millis() - CuentaMillis1) > TiempoON){

                // Apagar, actualizar el cuentatiempos y el estado y contar un ciclo hecho
                digitalWrite(pin, LOW ^ Invertir);
                CuentaMillis1 = millis();
                EstadoLed = LED_PULSO_OFF;
                if (NumeroPulsos != 0){ CuentaCiclos++;}
             
            }

            
        break;

        case LED_PULSO_OFF:

            // Si llevamos apagados mas del tiempo de apagado
            if ((millis() - CuentaMillis1) > TiempoOFF){

                // Si quedan pulsos por hacer
                if (CuentaCiclos < NumeroPulsos || NumeroPulsos == 0){
                    
                    
                    digitalWrite(pin, HIGH ^ Invertir);      // ENCENDER
                    CuentaMillis1 = millis();                // ACTUALIZAR EL TIEMPO DEL CONTADOR
                    EstadoLed = LED_PULSO_ON;                // PASAR A ESTE ESTADO QUE HARA OTRO CICLO

                }

                else{

                    CuentaMillis1 = millis();       // ACTUALIZAR EL TIEMPO DEL CONTADOR                    
                    EstadoLed=LED_APAGADO;

                }
                
             
            }
            
        break;

        default:
        break;

    
    }



}