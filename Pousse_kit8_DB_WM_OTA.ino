#include <Arduino.h>     //WM
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>     //WM    
#include "DHT.h"
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RemoteDebug.h>

SSD1306Wire display(0x3c, D4, D5, GEOMETRY_128_32); // ADDRESS, SDA, SCL  -  If not, they can be specified manually.

#define DHTTYPE DHT11   // DHT 11

/*Put your SSID & Password*/
//const char* ssid = "SSID";  // Enter SSID here
//const char* password = "PWD";  //Enter Password here

/* DHT Sensor*/
uint8_t DHTPin = D3;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

float Temperature;
float Humidity;
float ConsigneT = 24;
float ConsigneH = 80;

RemoteDebug Debug;
ESP8266WebServer server(80);
WiFiManager wm;

void handle_root();  //WM
void noderesetwifi();  //WM


void setup() {

  Serial.begin(115200);
  delay(100);
  Serial.println("");
  pinMode(DHTPin, INPUT);

  dht.begin();

  wm.autoConnect("Regul_AP", "youpi");
  //    ou
  //      Serial.println("Connecting to ");
  //      Serial.println(ssid);
  //      //connect to your local wi-fi network
  //      WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  // initialisation de la librairie de debug
  Debug.begin("Node");

  if (MDNS.begin("Node")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.on("/+", []() {
    ConsigneT = ConsigneT + 1;
    server.send(200, "text/html", SendHTML(Temperature, Humidity, ConsigneT));
  });
  server.on("/-", []() {
    ConsigneT = ConsigneT - 1;
    server.send(200, "text/html", SendHTML(Temperature, Humidity, ConsigneT));
  });
  server.on("/resetwifi", noderesetwifi); //WM

  server.begin();
  Serial.println("HTTP server started");

  // Initialising the UI will init the display too.
  display.init();
  display.setFont(ArialMT_Plain_10);
  //display.setFont(ArialMT_Plain_16);
  display.flipScreenVertically();

  ArduinoOTA.setHostname("Node"); // on donne une petit nom a notre module
  ArduinoOTA.begin(); // initialisation de l'OTA


}
void loop() {
  afficheregul();
  MDNS.update();
  ArduinoOTA.handle();
  Debug.handle();
  server.handleClient();
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML(Temperature, Humidity, ConsigneT));
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void afficheregul() {
  display.clear();
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Temperature = Temperature - 2.5;
  Humidity = dht.readHumidity(); // Gets the values of the humidity
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  String disp = "T:";
  if (isnan(Temperature)) {disp += "N/A";} else {disp += (float)Temperature;}
  disp += "°C ";
  display.drawString(0, 0, disp);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  disp = "H:";
  if (isnan(Humidity)) {disp += "N/A";} else {disp += (int)Humidity;}
  disp += "%";
  display.drawString(127, 0, disp);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  disp = "Consigne:";
  disp += (int)ConsigneT;
  disp += "°C";
  display.drawString(0, 10, disp);
  disp = "Hygro:";
  disp += (int)ConsigneH;
  disp += "%";
  display.drawString(0, 20, disp);
  // write the buffer to the display
  display.display();

}

String SendHTML(float Temperaturestat, float Humiditystat, float ConsigneTstat) {
  Debug.println("request received");
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP8266 Weather Report</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP8266 Temp/Hygro</h1>\n";
  ptr += "<p>Temperature: ";
  if (isnan(Temperaturestat)) {ptr += "N/A";} else {ptr += (float)Temperaturestat;}
  ptr += "&deg;C</p>";
  ptr += "<p>Humidity: ";
  if (isnan(Humiditystat)) {
    ptr += "N/A";
  }
  else {
    ptr += (int)Humiditystat;
  }
  ptr += "%</p>";

  ptr += "<h1>REGULATION:</h1>\n";
  ptr += "<h1>";
  ptr += (int)ConsigneTstat;
  ptr += "&deg;C</h1>\n";
  ptr += "<p><a href=\"/+\"\"><button style=\"height: 50px; width: 100px\"> + </button></a>  ";
  ptr += "<a href=\"/-\"\"><button style=\"height: 50px; width: 100px\"> - </button></a></p>";
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

// Handle root url (/setup/)
void noderesetwifi() {
  //reset saved settings
  wm.resetSettings();
  ESP.restart();
}
