
#include <AbonaMatico.h>
#include <Arduino.h>
#include <ArduinoJson.h>				// OJO: Tener instalada una version NO BETA (a dia de hoy la estable es la 5.13.4). Alguna pata han metido en la 6
#include <NTPClient.h>					// Para la gestion de la hora por NTP
#include <WiFiUdp.h>					// Para la conexion UDP con los servidores de hora.
#include <FS.h>							// Libreria Sistema de Ficheros
#include <Configuracion.h>				// Fichero de configuracion
//#include <A4988.h>					// Para el stepper
//#include <A4988_Stepper.h>				// La libreria Stepper que usa Tasmota
#include <FlexyStepper.h>
#include <Bounce2.h>

FlexyStepper stepper;
//Bounce HomeSWDebounced = Bounce();

AbonaMatico::AbonaMatico(String fich_config_AbonaMatico, NTPClient& ClienteNTP) : ClienteNTP(ClienteNTP) {

	sAbonaMatico = this;	// Apuntar el puntero sAbonamatico a esta instancia (para funciones estaticas)

	detachInterrupt(digitalPinToInterrupt(PINHOME));
	pinMode(PINHOME, INPUT_PULLUP);
	//HomeSWDebounced.attach(PINHOME);
	//HomeSWDebounced.interval(5);
	pinMode(ENABLE_MOTOR, OUTPUT);
	digitalWrite(ENABLE_MOTOR,0);
	pinMode(PINLED, OUTPUT);
	digitalWrite(PINLED,1);
	HardwareInfo = "AbonaMatico-1.0";
	ComOK = false;
	HayQueSalvar = false;
	mificheroconfig = fich_config_AbonaMatico;
    
	//stepper = FlexyStepper();

	// Estados iniciales
	Estado_Mecanica = EM_SIN_INICIAR;
	Estado_Comunicaciones = EC_SIN_CONEXION;
	Estado_Riegamatico = ER_SIN_CONEXION;

	//estado_home = HomeSWDebounced.read();

	this->LeeConfig();

	stepper.connectToPins(STEP_MOTOR, DIR_MOTOR);
	stepper.setTargetPositionToStop();
	stepper.setStepsPerRevolution(PASOS_MOTOR);
	//stepper.setSpeedInStepsPerSecond((float)200);
	//stepper.setSpeedInRevolutionsPerSecond(2);
	//stepper.setAccelerationInRevolutionsPerSecondPerSecond(1);
	//stepper.setStepsPerMillimeter((PASOS_MOTOR*MICROPASOS)/PASOTRANSMISION);	
	//stepper.setStepsPerMillimeter(200/3);
	//stepper.setSpeedInMillimetersPerSecond(1);
	//stepper.setAccelerationInMillimetersPerSecondPerSecond(1);
	
}

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
		jObj.set("PM", stepper.getCurrentPositionInRevolutions());		// Posicion del Motor
		jObj.set("SH", estado_home);									// Switch Home
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

boolean AbonaMatico::SalvaConfig(){
	

	File mificheroconfig_handler = SPIFFS.open(mificheroconfig, "w");

	if (!mificheroconfig_handler) {
		Serial.println("No se puede abrir el fichero de configuracion de mi proyecto");
		return false;
	}

	if (mificheroconfig_handler.print(MiEstadoJson(0))){

		return true;

	}

	else {

		return false;

	}

}

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

				Serial.print("Configuracion Abonamatico Leida: ");
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

// Funcion para inicializar la mecanica. Se puede forzar pero NO si estamos en estado de la mecanica activo antes forzar a vacio.
// Basicamente cambia estados, la funcion mecanicarun sabra que hacer
void AbonaMatico::IniciaMecanica(){

	//estado_home = HomeSWDebounced.read();

	if (Estado_Mecanica == EM_SIN_INICIAR){
		
		stepper.setCurrentPositionInRevolutions(0);
		Estado_Mecanica = EM_INICIALIZANDO_BAJANDO;
		this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
						
		if (stepper.motionComplete() && estado_home == LOW){

			// No ponemos a mover para abajo todo el recorrido, y ya el resto en el mecanicarun
			Serial.println ("INIT: Moviendo a HOME");
			//stepper.setTargetPositionInMillimeters(POSABIERTO);
			stepper.setSpeedInRevolutionsPerSecond(3);
			stepper.setTargetPositionInRevolutions(POSMAX/PASOTRANSMISION);
			
		}
		
	}

	else {

		Serial.print ("ATENCION: No se puede inicializar la mecanica en el estado actual: ");
		Serial.println(Estado_Mecanica);
		this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));

	}


}

// Aqui implementar todo el funcionamiento de la mecanica. Se lanza desde el runfast
void AbonaMatico::MecanicaRun(){

	//estado_home = HomeSWDebounced.read();

	// Sempre que se detecte el end switch a 1 parar el motor al instante ya veremos en otra parte que hacer segun el estado de la maquina.
	if (!stepper.motionComplete() && estado_home == HIGH){
		
		stepper.setTargetPositionToStop();
		
	}
	/*
	// Aqui la secuencia de la mecanica
	switch (Estado_Mecanica){

		case EM_INICIALIZANDO_BAJANDO:
			
			// Si estamos andando para abajo. No hacer nada

			// Si estamos en Home Parados. Andar para arriba el recorrido total
			if (stepper.motionComplete() && estado_home){

				// en este caso nos movemos hasta arriba del todo
				Serial.println ("INIT: Alcanzado Home, Subiendo a POSMAX");
				stepper.setCurrentPositionInRevolutions(0);
				stepper.setTargetPositionInMillimeters(POSABIERTO*-1);				
				Estado_Mecanica = EM_ERROR;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));

			}

			else if (stepper.motionComplete() && !estado_home){

				// en este caso nos movemos hasta arriba del todo
				Serial.println ("INIT: Error en la mecanica");		
				Estado_Mecanica = EM_INICIALIZANDO_SUBIENDO;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));

			}

		break;
		
		case EM_INICIALIZANDO_SUBIENDO:
									
			if (stepper.motionComplete() && !estado_home){

				// Cambiar la mecanica de estado
				Estado_Mecanica = EM_ABIERTA;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
				Serial.println ("INIT: Mecanica Inicializada correctamente");
				Serial.println ("INIT: Proceda a poner la jeringuilla y continuar");
				HayQueSalvar=true;

			}

			if (stepper.motionComplete() && estado_home){

				// Cambiar la mecanica de estado
				Estado_Mecanica = Tipo_Estado_Mecanica::EM_ERROR;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
				Serial.println ("INIT: Fallo en el Switch Home");
				HayQueSalvar=true;

			}
			

		break;
		
		default:

			stepper.setTargetPositionToStop();
			Estado_Mecanica = Tipo_Estado_Mecanica::EM_ERROR;

		break;
	}
	*/

}


// Esta funcion se lanza desde una Task y hace las "cosas periodicas de la clase". No debe atrancarse nunca tampoco por supuesto (ni esta ni ninguna)
void AbonaMatico::Run() {
	
	// UpTime Minutos
	t_uptime = 0;
	
	if (HayQueSalvar){

		SalvaConfig();
		HayQueSalvar = false;

	}

}


void AbonaMatico::RunFast(){

	//HomeSWDebounced.update();
	this->MecanicaRun();
	stepper.processMovement();
	
}
