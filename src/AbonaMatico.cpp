
#include <AbonaMatico.h>
#include <Arduino.h>
#include <ArduinoJson.h>				// OJO: Tener instalada una version NO BETA (a dia de hoy la estable es la 5.13.4). Alguna pata han metido en la 6
#include <NTPClient.h>					// Para la gestion de la hora por NTP
#include <WiFiUdp.h>					// Para la conexion UDP con los servidores de hora.
#include <FS.h>							// Libreria Sistema de Ficheros
#include <Configuracion.h>				// Fichero de configuracion
#include <FlexyStepper.h>
#include <Pulsador.h>

FlexyStepper stepper;
Pulsador SwitchHome (PINHOME, INPUT_PULLUP);


AbonaMatico::AbonaMatico(String fich_config_AbonaMatico, NTPClient& ClienteNTP) : ClienteNTP(ClienteNTP) {

	sAbonaMatico = this;	// Apuntar el puntero sAbonamatico a esta instancia (para funciones estaticas)

	SwitchHome.debouncetime = DEBOUNCESWHOME;

	pinMode(ENABLE_MOTOR, OUTPUT);
	digitalWrite(ENABLE_MOTOR,0);
	pinMode(PINLED, OUTPUT);
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

	// STEPPER
	stepper = FlexyStepper();

	stepper.connectToPins(STEP_MOTOR, DIR_MOTOR);
	stepper.setTargetPositionToStop();
	stepper.setStepsPerRevolution(PASOS_MOTOR);
	stepper.setStepsPerMillimeter((PASOS_MOTOR*MICROPASOS)/PASOTRANSMISION);	

	//stepper.setSpeedInStepsPerSecond((float)200);
	//stepper.setSpeedInRevolutionsPerSecond(2);
	//stepper.setAccelerationInRevolutionsPerSecondPerSecond(1);
	//stepper.setStepsPerMillimeter(200/3);
	//stepper.setSpeedInMillimetersPerSecond(1);
	//stepper.setAccelerationInMillimetersPerSecondPerSecond(1);
	
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
		jObj.set("PM", stepper.getCurrentPositionInRevolutions());		// Posicion del Motor
		jObj.set("SH", (unsigned int)SwitchHome.LeeEstado());			// Switch Home
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

	// Solo se puede iniciar la mecanica si esta en este estado
	if (Estado_Mecanica == EM_SIN_INICIAR){
		
		// Resetear posicion y cambiado estado a BAJANDO (que es el siguiente estado)
		
		Estado_Mecanica = EM_INICIALIZANDO_BAJANDO;
		this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
						
		// Si el motor esta parado y el SW HOME en IDLE
		if (stepper.motionComplete() && SwitchHome.LeeEstado() == Pulsador::EDB_IDLE){

			// No ponemos a mover para abajo todo el recorrido, y ya el resto en el mecanicarun
			Serial.println ("INIT: Moviendo a HOME");
			//stepper.setTargetPositionInMillimeters(POSABIERTO);

			stepper.setSpeedInMillimetersPerSecond(1);
			stepper.setAccelerationInMillimetersPerSecondPerSecond(1);
			stepper.setCurrentPositionInMillimeters(0);
			stepper.setTargetPositionInMillimeters(POSMAX*-1);
			
		}

		// Si no es que estamos parados y abajo ya, asi que la maquina de estado hara su trabajo para esta situacion
		
	}

	// Si la mecanica no esta en el estado correcto para inicializar entonces no hacer nada, solo avisar.
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
	if (!stepper.motionComplete() && SwitchHome.LeeEstado() == Pulsador::EDB_PULSADO){
		
		stepper.setTargetPositionToStop();
		
	}
	
	// Aqui la secuencia de la mecanica
	switch (Estado_Mecanica){

		case EM_INICIALIZANDO_BAJANDO:
					
			// Si el motor esta parado
			if (stepper.motionComplete()){

				// Miramos el estado del switch de home
				switch (SwitchHome.LeeEstado()){

					// Si no esta pulsado algo malo ha pasado en la mecanica
					case Pulsador::EDB_IDLE:

						// en este caso nos movemos hasta arriba del todo
						Serial.println ("INIT: Error en la mecanica");		
						Estado_Mecanica = EM_ERROR;
						this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
						break;
										
					// En otro caso para arriba (incluso en Pulsador::EDB_DETECTADO_CAMBIO)
					default:
				
						Serial.println ("INIT: Alcanzado Home, Subiendo a POSMAX");
						stepper.setCurrentPositionInRevolutions(0);
						stepper.setTargetPositionInMillimeters(POSMAX);				
						Estado_Mecanica = EM_INICIALIZANDO_SUBIENDO;
						this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
						break;

				}


			}

		break;

		// En caso de subiendo .....		
		case EM_INICIALIZANDO_SUBIENDO:

			// Si esta parado el motor porque ya hemos llegado a POSMAX									
			if (stepper.motionComplete()){

				// Cambiar la mecanica de estado
				Estado_Mecanica = EM_ABIERTA;
				this->MiRespondeComandos("IniciaMecanica", String(Estado_Mecanica));
				Serial.println ("INIT: Mecanica Inicializada correctamente");
				Serial.println ("INIT: Proceda a poner la jeringuilla y continuar");
				HayQueSalvar=true;

			}

		break;
		
		// Por si las moscas porque no hemos implementado aun todos los casos
		default:

			stepper.setTargetPositionToStop();
			Estado_Mecanica = Tipo_Estado_Mecanica::EM_ERROR;
			//Serial.println ("ERROR: Estado de la mecanica no implementado");

		break;
	}
	

}

// Esta funcion se lanza desde una Task y hace las "cosas periodicas lentas de la clase". No debe atrancarse nunca tampoco por supuesto (ni esta ni ninguna)
void AbonaMatico::Run() {
	
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
	this->MecanicaRun();
	stepper.processMovement();

}
