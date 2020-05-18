/*

# ABONAMATICO 1.0
# Inyector de Abono en el riego con capcidades MQTT
Desarrollado con Visual Code + PlatformIO en Framework Arduino
Implementa las comunicaciones WIFI y MQTT asi como la configuracion de las mismas via comandos
Implementa el envio de comandos via puerto serie o MQTT
Implementa el uso de tareas para multiproceso con la libreria TaskScheduler
Author: Diego Maroto - BilbaoMakers 2020 - info@bilbaomakers.org - dmarofer@diegomaroto.net
https://github.com/dmarofer/ABONAMATICO_MQTT
https://bilbaomakers.org/
Licencia: GNU General Public License v3.0 - https://www.gnu.org/licenses/gpl-3.0.html


*/

#include <Arduino.h>
#include <NTPClient.h>					// Para la gestion de la hora por NTP
#include <WiFiUdp.h>					// Para la conexion UDP con los servidores de hora.
#include <Configuracion.h>				// Fichero de configuracion
#include <FlexyStepper.h>
#include <AccelStepper.h>
#include <IndicadorLed.h>

class AbonaMatico
{


private:

	// Variables Privadas

    bool HayQueSalvar;															// Flag para indicar que hay que salvar en el ciclo run
	String mificheroconfig;														// Para el nombre del fichero de configuracion

	unsigned long t_uptime;														// Para el tiempo que llevamos en marcha

	int PasosPorMilimetro;														// Numero de pasos para mover un milimetro
	bool Frenando;																// Flag para saber que hemos dado la orden de parada y no repetirla

	NTPClient &ClienteNTP;														// Para almacenar Alias (referencia) al objeto tipo NTPClient para poder usar en la clase el que viene del Main
	IndicadorLed &LedEstado;													// Para el Led de Estado
	static AbonaMatico* sAbonaMatico;											// Un objeto para albergar puntero a la instancia del Abonamatico y manipularla desde dentro desde la interrupcion

	
	// Funciones privadas
	
	typedef void(*RespondeComandoCallback)(String comando, String respuesta);	// Definir como ha de ser la funcion de Callback (que le tengo que pasar y que devuelve)
	RespondeComandoCallback MiRespondeComandos = nullptr;						// Definir el objeto que va a contener la funcion que vendra de fuera AQUI en la clase.

	void MecanicaRun();															// Funcion para el gobierno de la mecanica
	void EncoderRun();															// Para la gestion del encoder
		

public:

	

    AbonaMatico(String fich_config_AbonaMatico, NTPClient& ClienteNTP, IndicadorLed& LedEstado);
    ~AbonaMatico() {};

    //  Variables Publicas
	String HardwareInfo;											// Identificador del HardWare y Software
	bool ComOK;														// Si la wifi y la conexion MQTT esta OK
	int PosicionMM;													// Posicion de la mecanica en mm

	// Para el estado de la mecanica
	enum Tipo_Estado_Mecanica {

		EM_SIN_INICIAR,					// Cuando la mecanica no esta en un estado conocido
		EM_INICIALIZANDO_BAJANDO,		// Cuando se esta calibrando buscando el home
		EM_INICIALIZANDO_SUBIENDO,		// Cuando estamos subiendo desde home
		EM_ABIERTA,						// En su posicion superior para el cambio de jeringuilla
		EM_ACTIVA_PARADA,				// En funcionamiento normal con el motor parado
		EM_ACTIVA_EN_MOVIMIENTO,		// En funcionamiento normal con el motor en movimiento
		EM_ERROR,						// Error en la mecanica

	}Estado_Mecanica;

	// Para el estado del Riegamatico
	enum Tipo_Estado_Riegamatico {

		ER_SIN_CONEXION,				// No hay conexion con el riegamatico
		ER_CONECTADO_NOREADY,			// Hay conexion con el riegamatico pero no esta en estado operativo
		ER_CONRECTADO_READY,			// hay conexion con el riegamatico y esta operativo

	}Estado_Riegamatico;


	// Funciones Publicas
	String MiEstadoJson(int categoria);								// Devuelve un JSON con los estados en un array de 100 chars (la libreria MQTT no puede con mas de 100)
	void TaskRun();													// Funcion periodica de Tareas (update lento)
    void RunFast();													// Funcion de vida para las tareas rapidas, como las del objeto stepper.
	
	void SetRespondeComandoCallback(RespondeComandoCallback ref);	// Definir la funcion para pasarnos la funcion de callback del enviamensajes
	boolean LeeConfig();
	boolean SalvaConfig();

	// Funciones publicas de comandos
	void Abonar();
	void ResetMecanica();
	void IniciaMecanica();
	void Recargar();

	// Funciones con RIEGAMATICO
	void SetEstadoRiegamatico (Tipo_Estado_Riegamatico (estado));
	
	// funciones publicas de configuracion
	void SetML();
		
    
};
