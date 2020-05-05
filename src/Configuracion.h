
// Conexiones Del ESP8266

// SALIDAS
#define PINLED D7

// ENTRADAS
#define PINHOME D6

// STEPPER
#define PASOS_MOTOR 200         // Pasos por vuelta
#define MICROPASOS 16           // Micropasos del driver
#define DIR_MOTOR D0            // Pin de direccion
#define STEP_MOTOR D1           // Pin de pasos
#define ENABLE_MOTOR D2         // Pin de enable

// Y la geometria de la maquina en mm
#define PASOTRANSMISION 3           // Paso del husillo de la transmision en mm (mm por vuelta)
#define POSABIERTO 300              // Posicion maxima abierto (cambio jeringuilla)
#define POSMAX 290                  // Posicin de la jeringuilla llena
#define POSMIN 160                  // Posicion de la jeringuilla vacia

// CARGA DE LA BATERIA
#define THIBERNADO  60              // Tiempo para despertar y comprobar la bateria (minutos)

// Hola local con respecto a UTC
#define HORA_LOCAL 2

// Para el nombre del fichero de configuracion de comunicaciones
#define FICHERO_CONFIG_COM "/AbonaMaticoCom.json"

// Para el nombre del fichero de configuracion del proyecto
#define FICHERO_CONFIG_PRJ "/AbonaMaticoCfg.json"

