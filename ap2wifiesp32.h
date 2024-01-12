#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "EEPROM.h"

int wifi;

WebServer server(80);

//Declaracion de funcion de C++ para su utilizacion en C.
void escribirStringEnEEPROM(String cadena, int direccion);

void handleRoot() {
  String html = "<html><body>";
  html += "<form method='POST' action='/wifi'>";
  html += "Red Wi-Fi: <input type='text' name='ssid'><br>";
  html += "Contraseña: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Conectar'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

//Function that handle the wifi through the app
void handleWifi() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  Serial.print("Conectando a la red Wi-Fi ");
  Serial.println(ssid);
  Serial.print("Clave Wi-Fi ");
  Serial.println(password);
  Serial.print("...");

  WiFi.disconnect(); // Desconectar la red Wi-Fi anterior, si se estaba conectado
  WiFi.begin(ssid.c_str(), password.c_str(),6);
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED and cnt < 8) {
    delay(1000);
    Serial.print(".");
    cnt++;
  }
  
if (WiFi.status() == WL_CONNECTED)
  {
  Serial.println("Conexión establecida");
  server.send(200, "text/plain", "Conexión establecida");
  if(wifi == 0){
    escribirStringEnEEPROM(ssid, 50);
    escribirStringEnEEPROM(password, 100);
    wifi++;
    }
  else if(wifi == 1){
    escribirStringEnEEPROM(ssid, 150);
    escribirStringEnEEPROM(password, 200);
    wifi++; //wifi = 2
    }
  }
else{
  Serial.println("Conexión no establecida");
  server.send(200, "text/plain", "Conexión no establecida");
  }
}

//Funcion para conectarse al wifi cuando se detecta el ssid y la contraseña en la memoria EEPROM
int handleWifiDefault(String ssid, String password){
  Serial.print("Conectando a la red Wi-Fi ");
  Serial.println(ssid);
  Serial.print("Clave Wi-Fi ");
  Serial.println(password);
  Serial.print("...");
  WiFi.begin(ssid, password, 6);
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED and cnt < 8) {
    delay(1000);
    Serial.print(".");
    cnt++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Conexión establecida");
  }
  else{
    Serial.println("Conexión no establecida");
    return -1;//Retorna -1 si no ha podido conectarse
  }
  return 0;
}

void initAP(const char* apSsid,const char* apPassword) { // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);

  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);

  server.begin();
  Serial.print("Ip de esp32...");
  Serial.println(WiFi.softAPIP());
  //Serial.println(WiFi.localIP());
  Serial.println("Servidor web iniciado");
}

int loopAP() {
  server.handleClient();
  return wifi;
}