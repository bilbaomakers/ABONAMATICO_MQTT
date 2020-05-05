#include <Arduino.h>
#include <NTPClient.h>					// Para la gestion de la hora por NTP
#include <WiFiUdp.h>					// Para la conexion UDP con los servidores de hora.
//#include <A4988.h>						// Para el stepper
#include <Configuracion.h>				// Fichero de configuracion
#include <FlexyStepper.h>

class AbonaMatico
{


private:
    
    bool HayQueSalvar;
	String mificheroconfig;

	unsigned long t_uptime;						// Para el tiempo que llevamos en marcha

    typedef void(*RespondeComandoCallback)(String comando, String respuesta);			// Definir como ha de ser la funcion de Callback (que le tengo que pasar y que devuelve)
	RespondeComandoCallback MiRespondeComandos = nullptr;								// Definir el objeto que va a contener la funcion que vendra de fuera AQUI en la clase.

	// Para almacenar Alias (referencia) al objeto tipo NTPClient para poder usar en la clase el que viene del Main
    NTPClient &ClienteNTP;

	ICACHE_RAM_ATTR static void ISRHomeSwitch();		 		// Funcion servicio interrupcion switch home
	static AbonaMatico* sAbonaMatico;			// Un objeto para albergar puntero a la instancia del Abonamatico y manipularla desde dentro desde la interrupcion


	// para el estado del switch home
	bool estado_home;
    

	// Funciones privadas

	
	void MecanicaRun();														// Funcion para el gobierno de la mecanica

public:

    AbonaMatico(String fich_config_AbonaMatico, NTPClient& ClienteNTP);
    ~AbonaMatico() {};

    //  Variables Publicas
	String HardwareInfo;											// Identificador del HardWare y Software
	bool ComOK;														// Si la wifi y la conexion MQTT esta OK
	
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


	// Para el estado de las comunicaciones
	enum Tipo_Estado_Comunicaciones {

		EC_SIN_CONEXION,				// No conectado a ninguna red
		EC_SOLO_WIFI,					// Conectado solo a la wifi
		EC_CONECTADO,					// Conectado a la wifi y al broker mqtt

	}Estado_Comunicaciones;


	// Para el estado del Riegamatico
	enum Tipo_Estado_Riegamatico {

		ER_SIN_CONEXION,				// No hay conexion con el riegamatico
		ER_OFFLINE,						// Hay conexion con el riegamatico pero no esta en estado operativo
		ER_ACTIVO,						// hay conexion con el riegamatico y esta operativo

	}Estado_Riegamatico;


	// Funciones Publicas
	String MiEstadoJson(int categoria);								// Devuelve un JSON con los estados en un array de 100 chars (la libreria MQTT no puede con mas de 100)
	void Run();														// Actualiza las propiedades de estado de este objeto en funcion del estado de motores y sensores
    void RunFast();													// Funcion para las tareas rapidas, como las del objeto stepper.
	
	void SetRespondeComandoCallback(RespondeComandoCallback ref);	// Definir la funcion para pasarnos la funcion de callback del enviamensajes
	boolean LeeConfig();
	boolean SalvaConfig();

	void HomeInterrupt(); 											// Funcion publica para el manejo de la interrupcion

	// Funciones publicas de comandos
	void Abonar();
	void IniciaMecanica();
	void Recargar();
	
	// funciones publicas de configuracion
	void SetML();
		
    
};
