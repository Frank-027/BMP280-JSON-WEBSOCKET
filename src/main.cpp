#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Wire.h> // I2C Lib
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "secrets.h"  

#define TIME_INTERVAL 5000 // Interval for sensor readings in milliseconds

#define BMP_ADDR        0x76 // Address of BMP280 sensor to be used with I2C

Adafruit_BMP280 bmp;  // bmp object
WebSocketsServer webSocket(81); // Create a WebSocket server on port 81

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void setup() {
  Serial.begin( 115200 );
  while ( !Serial ) {
    delay( 10 );
  }
  Serial.println( "Startup ESP32 BME-280" );
  Serial.println( "--------------------------" );

  // Connect to Wi-Fi
  Serial.print( "Connecting to Wi-Fi" );
  WiFi.begin( SSID, PASSWORD );
  while ( WiFi.status() != WL_CONNECTED ) {
    delay( 500 );
    Serial.print( "." );
  }
  Serial.println( "Connected to Wi-Fi" );
  Serial.print( "IP Address: " );
  Serial.println( WiFi.localIP() );

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  bool status = bmp.begin( BMP_ADDR );
  if ( !status ) {
    Serial.println( "Could not find BME280 sensor");
    return;
  }

  // Optional: configure oversampling and filter
  // to obtain more accurate sensor readings
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  Serial.println( "Setup Complete------------" );
  Serial.println( "--------------------------" );
}

void loop() {
    // WebSocket-server continu verwerken
    webSocket.loop();

    // Sensor om de 5 seconden uitlezen
    static unsigned long previousMillis = 0;

    if (millis() - previousMillis >= TIME_INTERVAL) {

        previousMillis = millis();

        float temperature = bmp.readTemperature();
        float pressure = bmp.readPressure() / 100.0;

        Serial.print("Temperature: ");
        Serial.println(temperature);

        Serial.print("Pressure: ");
        Serial.println(pressure);

        String json = "{ \"temperature\": " +
                      String(temperature, 2) +
                      ", \"pressure\": " +
                      String(pressure, 2) + " }";

        webSocket.broadcastTXT(json);

        Serial.println("============================");
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] client %u disconnected\n", num);
      break;
    case WStype_CONNECTED:
        Serial.printf("[WSc] client %u connected\n", num);
        Serial.printf( "[WSc] IP-address: %s\n", webSocket.remoteIP(num).toString().c_str() );
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] client %u sent Text: %s\n", num, payload);
      break;
    default:
      Serial.printf("[WSc] client %u sent unknown type: %d\n", num, type);
      break;
  }
}
