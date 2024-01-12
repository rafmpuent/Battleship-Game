#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

String jugador1;
String jugador2;
bool namesReceive;

WebServer server(80);

void handleRoot() {
  String html = "<html><body>";
  html += "<form method='POST' action='/wifi'>";
  html += "Red Wi-Fi: <input type='text' name='ssid'><br>";
  html += "Contraseña: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Conectar'>";
  html += "</form>";
  html += "<form method='POST' action='/app'>";
  html += "Jugador1: <input type='text' name='jugador1'><br>";
  html += "Jugador2: <input type='text' name='jugador2'><br>";
  html += "<input type='submit' value='Enviar Datos'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleApp() {
  jugador1 = server.arg("jugador1");
  jugador2 = server.arg("jugador2");
  Serial.print("Jugador1: ");
  Serial.println(jugador1);
  Serial.print("Jugador2: ");
  Serial.println(jugador2);
  namesReceive = true;
}

void initAP(const char* apSsid,const char* apPassword) { // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);

  server.on("/", handleRoot);
  server.on("/app", handleApp);

  server.begin();
  Serial.print("Ip de esp32...");
  Serial.println(WiFi.softAPIP());
  //Serial.println(WiFi.localIP());
  Serial.println("Servidor web iniciado");
}

int loopAP() {
  server.handleClient();
}