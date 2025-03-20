#include <Wire.h>
#include <MPU6050.h>
#include <SoftwareSerial.h>

SoftwareSerial sim800(10, 11); // RX, TX (ajusta según tu conexión)
MPU6050 mpu;

String telefono = "+521234567890"; // 📞 Reemplaza con tu número de teléfono
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
}

void loop() {
    while (giroscopioEncendido) {
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // Detecta movimiento si hay aceleración alta
        if (abs(ax) > 1000 || abs(ay) > 1000 || abs(az) > 1000) {
            Serial.println("¡Movimiento detectado!");
            obtenerYEnviarUbicacion();
            delay(5000);  // Evita múltiples activaciones en poco tiempo
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
            enviarSMS(latitud, longitud);
        }
    }
}

void enviarSMS(String lat, String lon) {
    Serial.println("Enviando SMS...");

    String link = "https://maps.google.com/?q=" + lat + "," + lon;

    sim800.println("AT+CMGF=1");  // Modo texto
    delay(1000);
    sim800.print("AT+CMGS=\"");
    sim800.print(telefono);
    sim800.println("\"");
    delay(1000);
    sim800.print("Ubicación GSM: \n");
    sim800.print("Lat: " + lat + "\nLon: " + lon + "\n");
    sim800.print("Google Maps: " + link);
    delay(500);
    sim800.write(26);  // Ctrl+Z para enviar
    delay(3000);

    Serial.println("SMS enviado.");
}
