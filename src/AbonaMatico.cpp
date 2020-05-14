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

#include <AbonaMatico.h>
#include <Arduino.h>
#include <ArduinoJson.h>				// OJO: Tener instalada una version NO BETA (a dia de hoy la estable es la 5.13.4). Alguna pata han metido en la 6
#include <NTPClient.h>					// Para la gestion de la hora por NTP
#include <WiFiUdp.h>					// Para la conexion UDP con los servidores de hora.
#include <FS.h>							// Libreria Sistema de Ficheros
#include <Configuracion.h>				// Fichero de configuracion
#include <FlexyStepper.h>
#include <Pulsador.h>

// El Objeto para el stepper
FlexyStepper stepper;

// El Objeto para los switches
Pulsador SwitchHome (PINHOME, INPUT_PULLUP, DEBOUNCESWHOME, false);
Pulsador EncoderPush (ENCODER_PUSH_PIN, INPUT_PULLUP, DEBOUNCESWUSER, true);

// Constructor de la  Clase
AbonaMatico::AbonaMatico(String fich_config_AbonaMatico, NTPClient& ClienteNTP) : ClienteNTP(ClienteNTP) {

	sAbonaMatico = this;	// Apuntar el puntero sAbonamatico a esta instancia (para funciones estaticas)

	pinMode(PINLED, OUTPUT);					// el LED
	digitalWrite(PINLED,1);
	
	HardwareInfo = "AbonaMatico-1.0";
	ComOK = false;
	HayQueSalvar = false;
	mificheroconfig = fich_config_AbonaMatico;
    
	// Estados iniciales
	Estado_Mecanica = EM_SIN_INICIAR;
	Estado_Comunicaciones = EC_SIN_CONEXION;
	Estado_Riegamatico = ER_SIN_CONEXION;

	// Mi Configuracion
	this->LeeConfig();

	// Stepper
	
	stepper.connectToPins(STEP_MOTOR ,DIR_MOTOR);
	stepper.setSpeedInStepsPerSecond(VMAX_MOTOR);
	stepper.setAccelerationInStepsPerSecondPerSecond(PASOS_MOTOR);
	stepper.setCurrentPositionInSteps(0);

	pinMode(ENABLE_MOTOR, OUTPUT);					// Enable del motor
		
	PosicionMM = 0;
	PasosPorMilimetro = PASOS_MOTOR / PASOTRANSMISION;

	Frenando = false;
		
}

// Callback para la funcion que pasa las respuestas a main
void AbonaMatico::SetRespondeComandoCallback(RespondeComandoCallback ref) {

	MiRespondeComandos = (RespondeComandoCallback)ref;

}

// Metodo que devuelve un JSON con el estado
String AbonaMatico::MiEstadoJson(int categoria) {

	DynamicJsonBuffer jBuffer;
	JsonObject& jObj = jBuffer.createObject();

	struct rst_info *rtc_info = system_get_rst_info(); 

	// Dependiendo del numero de categoria en la llamada devolver unas cosas u otras
	switch (categoria)
	{

	case 0:

		// Configuracion
		jObj.set("HI", HardwareInfo);							
		jObj.set("EM", String(Estado_Mecanica));
		break;


	case 1:

		// Esto llena de objetos de tipo "pareja propiedad valor"
		jObj.set("TIME", ClienteNTP.getFormattedTime());				// HORA
		jObj.set("HI", HardwareInfo);									// Info del Hardware
		jObj.set("EM", (unsigned int)Estado_Mecanica);					// Estado de la Mecanica
		jObj.set("EC", (unsigned int)Estado_Comunicaciones);			// Estado de las Comunicaciones
		jObj.set("ER", (unsigned int)Estado_Riegamatico);				// Estado del Riegamatico
		jObj.set("PM", PosicionMM);										// Posicion de la mecanica
		jObj.set("SH", (unsigned int)SwitchHome.LeeEstado());			// Switch Home
		jObj.set("SU", (unsigned int)EncoderPush.LeeEstado());			// Switch User (Pulsador del Encoder)
		jObj.set("MR", !stepper.motionComplete());						// Motor Running
		jObj.set("RC", rtc_info->reason);								// Reset Cause (1=WTD reset)

		break;

	case 2:

		jObj.set("INFO2", "INFO2");							
		
		break;

	default:

		jObj.set("NOINFO", "NOINFO");						// MAL LLAMADO

		break;
	}


	// Crear un buffer (aray de 100 char) donde almacenar la cadena de texto del JSON
	char JSONmessageBuffer[200];

	// Tirar al buffer la cadena de objetos serializada en JSON con la propiedad printTo del objeto de arriba
	jObj.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

	// devolver el char array con el JSON
	return JSONmessageBuffer;
	
}

// Funcion para salvar la configuracion en el fichero
boolean AbonaMatico::SalvaConfig(){
	

	File mificheroconfig_handler = SPIFFS.open(mificheroconfig, "w");

	if (!mificheroconfig_handler) {
		Serial.println("ABM: No se puede abrir el fichero de configuracion de mi proyecto");
		return false;
	}

	if (mificheroconfig_handler.print(MiEstadoJson(0))){

		return true;

	}

	else {

		return false;

	}

}

// Funcion para leer la configuracion desde el fichero
boolean AbonaMatico::LeeConfig(){

	// Sacar del fichero de configuracion, si existe, las configuraciones permanentes
	if (SPIFFS.exists(mificheroconfig)) {

		File mificheroconfig_handler = SPIFFS.open(mificheroconfig, "r");
		if (mificheroconfig_handler) {
			size_t size = mificheroconfig_handler.size();
			// Declarar un buffer para almacenar el contenido del fichero
			std::unique_ptr<char[]> buf(new char[size]);
			// Leer el fichero al buffer
			mificheroconfig_handler.readBytes(buf.get(), size);
			DynamicJsonBuffer jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(buf.get());
			if (json.success()) {

				Serial.print("ABM: Configuracion Abonamatico Leida: ");
				json.printTo(Serial);
				Serial.println("");

				// Dar valor a las variables desde el JSON de configuracion
				// Estado_Mecanica = (Tipo_Estado_Mecanica)json.get<int>("EM");
				
				return true;

			}

			return false;

		}

		return false;

	}

	return false;

}

// Funcion para poner la mecanica en estado 0
void AbonaMatico::ResetMecanica(){


	Estado_Mecanica = EM_SIN_INICIAR;
	this->MiRespondeComandos("ResetMecanica", String(Estado_Mecanica));


}

// Funcion para disparar el ciclo de inicio de la mecanica. La mecanica tiene que estar en estado 0
void AbonaMatico::IniciaMecanica(){


	//estado_home = HomeSWDebounced.read();

	// Solo se puede iniciar la mecanica si esta en este estado
	if (Estado_Mecanica == EM_SIN_INICIAR){
		
		// Resetear posicion y cambiado estado a BAJANDO (que es el siguiente estado)
		
		Estado_Mecanica = EM_INICIALIZANDO_BAJANDO;
		HayQueSalvar = true;
		this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
						
		// Si el motor esta parado y el SW HOME en IDLE
		if (stepper.motionComplete() && SwitchHome.LeeEstado() == Pulsador::EDB_IDLE){

			// No ponemos a mover para abajo todo el recorrido, y ya el resto en el mecanicarun
			Serial.println ("ABM: Moviendo a HOME");

			//AccelStepper
			stepper.setSpeedInStepsPerSecond(VMAX_MOTOR/2);
			stepper.setAccelerationInStepsPerSecondPerSecond(VMAX_MOTOR/2);
			stepper.setCurrentPositionInSteps(0);
			digitalWrite(ENABLE_MOTOR,0);
			Frenando=false;
			stepper.setTargetPositionInSteps(POSMAX * PasosPorMilimetro * -1);
			
		}

		// Si no es que estamos parados y abajo ya, asi que la maquina de estado hara su trabajo para esta situacion
		
	}

	// Si la mecanica no esta en el estado correcto para inicializar entonces no hacer nada, solo avisar.
	else {

		Serial.print ("ABM: ERROR, No se puede inicializar la mecanica en el estado actual: ");
		Serial.println(Estado_Mecanica);
		this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));

	}



}

// Actualizar la variable interna con el estado del riegamatico. Desde el MAIN que es donde se procesa la telemetria
void AbonaMatico::SetEstadoRiegamatico (Tipo_Estado_Riegamatico (estado)){

	Estado_Riegamatico = estado;

}

// Maquina de estado de la mecanica
void AbonaMatico::MecanicaRun(){


	// Actualizar la posicion en mm
	PosicionMM = (stepper.getCurrentPositionInSteps()/PASOS_MOTOR)*PASOTRANSMISION ;
	
	// Aqui la secuencia de la mecanica
	switch (Estado_Mecanica){

		case EM_INICIALIZANDO_BAJANDO:

			if (!stepper.motionComplete() && SwitchHome.LeeEstado() == Pulsador::EDB_PULSADO && !Frenando){
				
			Serial.println("ABM: Detectado Home. Parando motor.");
			stepper.setTargetPositionToStop();
			Frenando = true;
		
			}

			// Si el motor esta parado
			if (stepper.motionComplete()){

				Serial.println("ABM: Motor Parado en Home");
				
				Serial.println ("ABM: Alcanzado Home, Subiendo a POSABIERTO");
				
				//AccelStepper
				stepper.setSpeedInStepsPerSecond(VMAX_MOTOR);
				stepper.setAccelerationInStepsPerSecondPerSecond(VMAX_MOTOR);
				stepper.setCurrentPositionInSteps(0);
				digitalWrite(ENABLE_MOTOR,0);
				Frenando = false;
				stepper.setTargetPositionInSteps(POSABIERTO * PasosPorMilimetro);

				Estado_Mecanica = EM_INICIALIZANDO_SUBIENDO;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
				break;

			}

		break;

		// En caso de subiendo .....		
		case EM_INICIALIZANDO_SUBIENDO:

			// Si esta parado el motor porque ya hemos llegado a POSABIERTO									
			if (stepper.motionComplete()){

				// Cambiar la mecanica de estado
				digitalWrite(ENABLE_MOTOR,1);
				Estado_Mecanica = EM_ABIERTA;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
				Serial.println ("ABM: Mecanica Inicializada correctamente");
				Serial.println ("ABM: Proceda a poner la jeringuilla y continuar");
				HayQueSalvar=true;

			}

		break;
		

		case EM_ABIERTA:

			if (EncoderPush.LeeEstado() == Pulsador::EDB_PULSADO){

				Serial.println ("ABM: Jeringuilla Puesta Bajando a POSMAX");

				//AccelStepper
				stepper.setSpeedInStepsPerSecond(VMAX_MOTOR/2);
				stepper.setAccelerationInStepsPerSecondPerSecond(VMAX_MOTOR/2);
				digitalWrite(ENABLE_MOTOR,0);
				Frenando=false;
				stepper.setTargetPositionInSteps(POSMAX * PasosPorMilimetro);
				Estado_Mecanica = EM_ACTIVA_EN_MOVIMIENTO;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
				break;

			}
		
		break;

		case EM_ACTIVA_EN_MOVIMIENTO:

			if(stepper.motionComplete()){

				Estado_Mecanica=EM_ACTIVA_PARADA;
				HayQueSalvar=true;
				digitalWrite(ENABLE_MOTOR,1);
				Serial.println("ABM: Movimiento completado. Mecanica OK");

			}

		break;

		case EM_ACTIVA_PARADA:

			if(!stepper.motionComplete()){

				Estado_Mecanica=EM_ACTIVA_EN_MOVIMIENTO;
				Serial.println("ABM: Iniciando Movimiento");
				
			}			

		break;

		case EM_SIN_INICIAR:

			// Aqui de momento no hacer nada tranquilitos, pero es para que no salte default en este caso.

		break;


		// Por si las moscas porque no hemos implementado aun todos los casos o si pasa algo raro
		default:

			stepper.setTargetPositionToStop();
			Frenando=true;
			Estado_Mecanica = Tipo_Estado_Mecanica::EM_ERROR;
			Serial.println ("ABM: ERROR Estado de la mecanica no implementado.");

		break;
	}
	

}

// Esta funcion se lanza desde una Task y hace las "cosas periodicas lentas de la clase". No debe atrancarse nunca tampoco por supuesto (ni esta ni ninguna)
void AbonaMatico::TaskRun() {
	
	// UpTime Minutos
	t_uptime = 0;
	
	if (HayQueSalvar){

		SalvaConfig();
		HayQueSalvar = false;

	}

}

// Funcion de vida, lo mas rapido posible
void AbonaMatico::RunFast(){

	SwitchHome.Run();
	ESP.wdtFeed();
	EncoderPush.Run();
	ESP.wdtFeed();
	this->MecanicaRun();
	ESP.wdtFeed();
	stepper.processMovement();
	ESP.wdtFeed();

}
