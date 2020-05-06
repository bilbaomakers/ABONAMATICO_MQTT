
#include <Comunicaciones.h>
#include <Arduino.h>
#include <string>						// Para el manejo de cadenas
#include <AsyncMqttClient.h>			// Vamos a probar esta que es Asincrona: https://github.com/marvinroger/async-mqtt-client
#include <ArduinoJson.h>				// OJO: Tener instalada una version NO BETA (a dia de hoy la estable es la 5.13.4). Alguna pata han metido en la 6

// TAREAS:
// Quitar de aqui cualquier interaccion directa con nada, por ejemplo con el Serial
// hacerlo con callbacks


AsyncMqttClient ClienteMQTT;

Comunicaciones::Comunicaciones(){
     
    
}

Comunicaciones::~Comunicaciones(){


}


void Comunicaciones::SetEventoCallback(TipoCallbackEvento ref){

    MiCallbackEventos = (TipoCallbackEvento)ref;

}


void Comunicaciones::SetMqttServidor(char l_mqttserver[40]){

    strncpy(mqttserver, l_mqttserver, sizeof(l_mqttserver));
    this->Desonectar();
    
}


void Comunicaciones::SetMqttUsuario(char l_mqttusuario[19]){

    strncpy(mqttusuario, l_mqttusuario, sizeof(l_mqttusuario));
    this->Desonectar();

}


void Comunicaciones::SetMqttPassword(char l_mqttpassword[19]){

    strncpy(mqttpassword, l_mqttpassword, sizeof(l_mqttpassword));
    this->Desonectar();

}


void Comunicaciones::SetMqttTopic(char l_mqtttopic[33]){

    strncpy(mqtttopic, l_mqtttopic, sizeof(l_mqtttopic));
    this->FormaEstructuraTopics();
    this->Desonectar();

}


void Comunicaciones::SetMqttClientId(char l_mqttclientid[15]){

    strncpy(mqttclientid, l_mqttclientid, sizeof(l_mqttclientid));
    this->Desonectar();

}



void Comunicaciones::FormaEstructuraTopics(){

    cmndTopic = "cmnd/" + String(mqtttopic) + "/#";
    statTopic = "stat/" + String(mqtttopic);
    teleTopic = "tele/" + String(mqtttopic);
    lwtTopic = teleTopic + "/LWT";

}

bool Comunicaciones::IsConnected(){

    return ClienteMQTT.connected();

}

void Comunicaciones::Conectar(){

    ClienteMQTT = AsyncMqttClient();

    this->FormaEstructuraTopics();

    ClienteMQTT.setServer(mqttserver, 1883);
    ClienteMQTT.setCleanSession(true);
    ClienteMQTT.setClientId(mqttclientid);
    ClienteMQTT.setCredentials(mqttusuario,mqttpassword);
    ClienteMQTT.setKeepAlive(4);
    ClienteMQTT.setWill(lwtTopic.c_str(),2,true,"Offline");

    // Aqui vamos a explicar por que esto, que deberia ser lo "normal" no funciona y lo que hay que hacer    
    // Esto se llama "Voy a pasar a un objeto instanciado en esta clase una funcion callback miembro, ahi queda eso"
    /*
    ClienteMQTT.onConnect(onMqttConnect);
    ClienteMQTT.onDisconnect(onMqttDisconnect);
    ClienteMQTT.onMessage(onMqttMessage);
    ClienteMQTT.onPublish(onMqttPublish);
    */
  
    // Podriamos pensar que bueno, esto o algo parecido podria funcionar porque en realidad como callback al objeto MQTT
    // yo le tengo que pasar una funcion REAL, INSTANCIADA
    // ClienteMQTT.onConnect(this->onMqttConnect);

    // Y es correcto, pero se hace asi, con std:bind (que facil eh :D estaba fumao el que invento esto jaja)
    // lo de los placeholders hace referencia a los parametros de la funcion
    ClienteMQTT.onConnect(std::bind(&Comunicaciones::onMqttConnect, this, std::placeholders::_1));
    ClienteMQTT.onDisconnect(std::bind(&Comunicaciones::onMqttDisconnect, this, std::placeholders::_1));
    ClienteMQTT.onMessage(std::bind(&Comunicaciones::onMqttMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    ClienteMQTT.onPublish(std::bind(&Comunicaciones::onMqttPublish, this, std::placeholders::_1));



}

void Comunicaciones::Desonectar(){

    ClienteMQTT.disconnect();
    
}

void Comunicaciones::onMqttConnect(bool sessionPresent) {

    bool susflag = false;
	bool lwtflag = false;
	
    char Mensaje[100];

	// Suscribirse al topic de Entrada de Comandos
	if (ClienteMQTT.subscribe(cmndTopic.c_str(), 2)) {
	
		susflag = true;				

	}
		
	// Publicar un Online en el LWT
	if (ClienteMQTT.publish((teleTopic + "/LWT").c_str(), 2,true,"Online")){

		lwtflag = true;

	}



	if (!susflag || !lwtflag){

		// Si falla la suscripcion o el envio del Online malo kaka. Me desconecto para repetir el proceso.
        strcpy(Mensaje, "MQTT: Algo falla al suscribirme a los topics");
        MiCallbackEventos(EVENTO_DESCONECTADO, Mensaje);
		this->Desonectar();

	}

	else{

        strcpy(Mensaje, "MQTT: Conectado y suscrito correctamente");
        MiCallbackEventos(EVENTO_CONECTADO, Mensaje);

	}

}

void Comunicaciones::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

    char Razon_Desconexion[100];

    switch (reason){

        case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:

            strcpy(Razon_Desconexion, "Servidor MQTT no disponible");

        break;

        case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:

            strcpy(Razon_Desconexion, "Conexion MQTT Perdida");

        break;

        
        case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:

            strcpy(Razon_Desconexion, "MQTT: Acceso denegado");

        break;


        default:

            strcpy(Razon_Desconexion, "Otro error de conexion MQTT sin implementar");

        break;


    }

	MiCallbackEventos(EVENTO_DESCONECTADO, Razon_Desconexion);

}

void Comunicaciones::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  
		// Toda esta funcion me la dispara la libreria MQTT cuando recibe algo y me pasa todo lo de arriba
        // Aqui me la cocino yo a mi gusto, miro si es cmnd/#, y construyo un Json muy bonito con el comando
        // Despues a su vez yo paso este JSON mi comando MiCallbackMsgRecibido al main para que haga con ella lo que guste
        // Que generalmente sera enviarlo a la tarea de evaluacion de comandos para ver que hacer.
        String s_topic = String(topic);

		// Para que no casque si no viene payload. Asi todo OK al gestor de comandos le llega vacio como debe ser, el JSON lo pone bien.
		if (payload == NULL){

			payload = "NULL";

		}
	
		// Lo que viene en el char* payload viene de un buffer que trae KAKA, hay que limpiarlo (para eso nos pasan len y tal)
		char c_payload[len+1]; 										// Array para el payload y un hueco mas para el NULL del final
		strlcpy(c_payload, payload, len+1); 			            // Copiar del payload el tamaño justo. strcopy pone al final un NULL
		
		// Y ahora lo pasamos a String que esta limpito
		String s_payload = String(c_payload);

		// Sacamos el prefijo del topic, o sea lo que hay delante de la primera /
		int Indice1 = s_topic.indexOf("/");
		String Prefijo = s_topic.substring(0, Indice1);
		
		// Si el prefijo es cmnd se lo mandamos al manejador de comandos
		if (Prefijo == "cmnd") { 

			// Sacamos el "COMANDO" del topic, o sea lo que hay detras de la ultima /
			int Indice2 = s_topic.lastIndexOf("/");
			String Comando = s_topic.substring(Indice2 + 1);

			DynamicJsonBuffer jsonBuffer;
			JsonObject& ObjJson = jsonBuffer.createObject();
			ObjJson.set("COMANDO",Comando);
			ObjJson.set("PAYLOAD",s_payload);

			char JSONmessageBuffer[100];
			ObjJson.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
            
            MiCallbackEventos(Comunicaciones::EVENTO_MSG_RX, JSONmessageBuffer);
						
		}

		
}

void Comunicaciones::onMqttPublish(uint16_t packetId) {
  
	// Al publicar no hacemos nada de momento.

}
