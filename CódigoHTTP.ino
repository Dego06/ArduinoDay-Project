#include <Wire.h>
#include <MPU6050.h>
#include <SoftwareSerial.h>

SoftwareSerial sim800(10, 11); // RX, TX (ajusta según tu conexión)
MPU6050 mpu;

String serverURL = "http://tuservidor.com/endpoint"; // Reemplaza con la URL de tu servidor
bool giroscopioEncendido = true;  // Estado del giroscopio

void setup() {
    Serial.begin(115200);
    sim800.begin(9600);
    Wire.begin();

    Serial.println("Inicializando MPU6050...");
    mpu.initialize();

    if (mpu.testConnection()) {
        Serial.println("MPU6050 conectado correctamente.");
    } else {
        Serial.println("Error al conectar el MPU6050.");
    }

    // Inicializar el módulo SIM800C
    inicializarSIM800();
}

void loop() {
    // Verificar si hay comandos desde el servidor
    verificarComandosServidor();

    // Si el giroscopio está encendido, monitorear el movimiento
    if (giroscopioEncendido) {
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // Detectar movimiento si hay aceleración alta
        if (abs(ax) > 1000 || abs(ay) > 1000 || abs(az) > 1000) {
            Serial.println("¡Movimiento detectado!");
            obtenerYEnviarUbicacion();
            delay(5000);  // Evita múltiples activaciones en poco tiempo
        }
    }
}

// Inicializar el módulo SIM800C
void inicializarSIM800() {
    Serial.println("Inicializando SIM800C...");
    sim800.println("AT");
    delay(1000);
    sim800.println("AT+CFUN=1");  // Activar el módulo
    delay(1000);
    sim800.println("AT+CREG?");   // Verificar registro en la red
    delay(1000);
    sim800.println("AT+CGATT=1"); // Adjuntar a la red GPRS
    delay(1000);
    sim800.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); // Configurar conexión GPRS
    delay(1000);
    sim800.println("AT+SAPBR=3,1,\"APN\",\"tuapn\"");    // Reemplaza "tuapn" con el APN de tu operador
    delay(1000);
    sim800.println("AT+SAPBR=1,1"); // Abrir conexión GPRS
    delay(2000);
}

// Verificar comandos desde el servidor
void verificarComandosServidor() {
    sim800.println("AT+HTTPTERM"); // Terminar cualquier conexión HTTP previa
    delay(1000);
    sim800.println("AT+HTTPINIT"); // Inicializar HTTP
    delay(1000);
    sim800.println("AT+HTTPPARA=\"URL\",\"http://tuservidor.com/comandos\""); // URL para recibir comandos
    delay(1000);
    sim800.println("AT+HTTPACTION=0"); // Realizar una solicitud GET
    delay(5000);

    if (sim800.available()) {
        String respuesta = "";
        while (sim800.available()) {
            char c = sim800.read();
            respuesta += c;
        }
        Serial.println("Respuesta del servidor: " + respuesta);

        // Procesar la respuesta para activar/desactivar el giroscopio
        if (respuesta.indexOf("ENCENDER") != -1) {
            encenderApagarGiroscopio(true);
        } else if (respuesta.indexOf("APAGAR") != -1) {
            encenderApagarGiroscopio(false);
        }
    }
}

// Función para activar/desactivar el giroscopio
void encenderApagarGiroscopio(bool estado) {
    giroscopioEncendido = estado;
    if (estado) {
        Serial.println("Giroscopio ENCENDIDO");
    } else {
        Serial.println("Giroscopio APAGADO");
    }
}

// Obtener y enviar la ubicación al servidor
void obtenerYEnviarUbicacion() {
    Serial.println("Solicitando ubicación...");
    sim800.println("AT+CIPGSMLOC=1,1");
    delay(5000);

    if (sim800.available()) {
        String respuesta = "";
        while (sim800.available()) {
            char c = sim800.read();
            respuesta += c;
        }
        Serial.println("Ubicación recibida: " + respuesta);

        int inicio = respuesta.indexOf(":") + 2;
        int coma1 = respuesta.indexOf(",", inicio);
        int coma2 = respuesta.indexOf(",", coma1 + 1);

        if (inicio > 1 && coma1 > 1 && coma2 > 1) {
            String latitud = respuesta.substring(inicio, coma1);
            String longitud = respuesta.substring(coma1 + 1, coma2);
            enviarDatosServidor(latitud, longitud);
        }
    }
}

// Enviar datos al servidor HTTP
void enviarDatosServidor(String lat, String lon) {
    Serial.println("Enviando datos al servidor...");

    // Construir el cuerpo de la solicitud POST
    String postData = "latitud=" + lat + "&longitud=" + lon;

    // Configurar la solicitud HTTP
    sim800.println("AT+HTTPTERM"); // Terminar cualquier conexión HTTP previa
    delay(1000);
    sim800.println("AT+HTTPINIT"); // Inicializar HTTP
    delay(1000);
    sim800.println("AT+HTTPPARA=\"URL\",\"" + serverURL + "\""); // URL del servidor
    delay(1000);
    sim800.println("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\""); // Tipo de contenido
    delay(1000);
    sim800.println("AT+HTTPDATA=" + String(postData.length()) + ",10000"); // Enviar datos
    delay(1000);
    sim800.print(postData); // Enviar el cuerpo de la solicitud
    delay(1000);
    sim800.println("AT+HTTPACTION=1"); // Realizar una solicitud POST
    delay(5000);

    if (sim800.available()) {
        String respuesta = "";
        while (sim800.available()) {
            char c = sim800.read();
            respuesta += c;
        }
        Serial.println("Respuesta del servidor: " + respuesta);
    }
}
