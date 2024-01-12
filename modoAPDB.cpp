#include <Arduino.h>
#include "EEPROM.h"
#include "ap2wifiesp32.h"
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Definir la URL a la que realizar la solicitud HTTP GET
const char *url = "http://www.192.168.4.1.com";

#define API_KEY "AIzaSyAi6C7594ps6b-90bk1PpklZFwjqUxVsV4"
#define DATABASE_URL "https://battleship-4232b-default-rtdb.europe-west1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

//Escribir cadena en memoria eeprom
void escribirStringEnEEPROM(String cadena, int direccion) {
  int longitudCadena = cadena.length();
  for (int i = 0; i < longitudCadena; i++) {
    EEPROM.write(direccion + i, cadena[i]);
  }
  EEPROM.write(direccion + longitudCadena, '\0'); // Null-terminated string
  EEPROM.commit(); // Guardamos los cambios en la memoria EEPROM
}

//leer cadena en memoria eeprom
String leerStringDeEEPROM(int direccion) {
  String cadena = "";
  char caracter = EEPROM.read(direccion);
  int i = 0;
  while (caracter != '\0' && i < 100) {
    cadena += caracter;
    i++;
    caracter = EEPROM.read(direccion + i);
  }
  return cadena;
}

//Leer valor entero desde EEPROM.
int leerValorEnteroDesdeEEPROM(int direccion) {
  int valorLeido = 0;
  int tamanoValor = sizeof(valorLeido);
  
  for (int i = 0; i < tamanoValor; i++) {
    byte byteLeido = EEPROM.read(direccion + i);
    valorLeido |= (byteLeido << (i * 8));
  }
  
  return valorLeido;
}

bool wifiNotFound1;     //No se ha encontrado una red Wifi 1
bool wifiNotFound2;     //No se ha encontrado una red Wifi 1
bool wifiNotConnected1; //No se ha podido conectar a la red Wifi 1
bool wifiNotConnected2; //No se ha podido conectar a la red Wifi 2
bool wifiConnected1;    //Se ha podido conectar a la red Wifi 1
bool wifiConnected2;    //Se ha podido conectar a la red Wifi 2
bool setUpFireBase = true;     //Establecer FireBase
int aciertos1 = 0;
int aciertos2 = 0;
int fallos1 = 0;
int fallos2 = 0;
String jugador1 = "";
String jugador2 = "";

void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  int ssid1 = leerValorEnteroDesdeEEPROM(50);
  int ssid2 = leerValorEnteroDesdeEEPROM(150);
  //Las esp32 tienen un valor por defecto en las direcciones de memoria EEPROM, usualmente es el valor de 255
  //Para mi esp32 el valor por defecto que se encontro fueron los valores de -1 y 0.
  wifiNotFound1 = ssid1 == -1 || ssid1 == 255 || ssid1 == 0; //No se ha encontrado wifi1
  wifiNotFound2 = ssid2 == -1 || ssid2 == 255 || ssid2 == 0; //No se ha encontrado wifi2
  if(wifiNotFound1 && wifiNotFound2){
    Serial.println("Wifi1 no encontrado");
    Serial.println("Wifi2 no encontrado");
    //En caso de que no se haya encontrado una red wifi en la memoria EEPROM, se setea el punto AP y se espera ingresar el wifi desde la APP
    initAP("espAP","123456789");//nombre de wifi a generarse y contrasena
  }
  else{
    //En caso de que se haya encontrado un wifi en la memoria EEPROM, se conectara a este.
    if(!wifiNotFound1){
      Serial.println("Wifi1 Encontrado");
      int wifi1 = handleWifiDefault(leerStringDeEEPROM(50), leerStringDeEEPROM(100));
      if(wifi1 == 0) wifiConnected1 = true; //Si retorna 0 significa que si se pudo conectar a la red1.
      else if(wifi1 == -1){//No se pudo conectar a la red wifi 1
        wifiNotConnected1 = true; //No se pudo conectar a la red wifi 1
        if(!wifiNotFound2){ //Nos intentamos conectar a la red wifi 2 en caso de que se haya encontrado          
          Serial.println("Wifi2 Encontrado");
          int wifi2 = handleWifiDefault(leerStringDeEEPROM(150), leerStringDeEEPROM(200));
          if(wifi2 == -1) {
            wifiNotConnected2 = true;    //No se pudo conectar a la red wifi 2
            initAP("espAP","123456789"); //Si no se pudo conectar a la segunda red wifi
          }
        }
        else{ //En caso de que no se haya podido conectar a la red wifi 1 y no se haya encontrado una segunda red
          initAP("espAP","123456789");//nombre de wifi a generarse y contrasena
        }
      }
    }
    else if(!wifiNotFound2){ //Si ingresamos aqui significa que la primera red wifi no se ha encontrado
      Serial.println("Wifi2 Encontrado");
      int wifi2 = handleWifiDefault(leerStringDeEEPROM(150), leerStringDeEEPROM(200));
      if(wifi2 == 0) wifiConnected2 = true;
      else if(wifi2 == -1){
        //Si esta linea se ejecuta significa que no se pudo conectar a la red Wifi2 y que no se encontro una redWifi1
        wifiNotConnected2 = true;    //No se pudo conectar a la red wifi 2
        initAP("espAP","123456789");//nombre de wifi a generarse y contrasena
      }
    }
  }
  initAP("espAP","123456789");//nombre de wifi a generarse y contrasena
}

void setupFireBase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if(Firebase.signUp(&config, &auth, "", "")){
    signupOK = true;
  }else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  setUpFireBase = false;
}

void loop() {
  //Si no se ha encontrado una red Wifi en la memoria EEPROM, si se esperara cambios en la app
  if(wifiNotFound1 && wifiNotFound2){
    if(loopAP() == 2){ //Cuando loopAp retorna 2 significa que se ha conectado exitosamente a 2 redes wifi
      wifiNotFound1 = false; //Establecemos estas variables como falsas para que no se vuelva a ejecutar el loopAP
      wifiNotFound2 = false;
    }
  }
  else if(wifiNotConnected1 && wifiNotConnected2) { //Si no se pudo conectar ni a la red wifi1 ni a la red wifi2, se sobrescribe la primera red wifi con una red wifi que si se pueda conectar
    if(loopAP() == 1) { //loopAP retorna 1 cuando si se pudo conectar exitosamente a la red Wifi
      wifiNotConnected1 = false;//Establecemos estas variables como falsas para que no se vuelva a ejecutar el loopAP
      wifiNotConnected2 = false;
    }
  } 

  if((wifiConnected1 || wifiConnected2) && setUpFireBase) setupFireBase();
  
  if(Firebase.ready() && signupOK && (wifiConnected1 || wifiConnected2)){
    //Datos primer jugador
    //Enviamos el nombre del jugador1
    if(Firebase.RTDB.pushString(&fbdo, "Jugador1/Nombre", jugador1)){
      Serial.println(); Serial.print(jugador1);
      Serial.print(" - succesfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    //Enviamos los aciertos del jugador1
    if(Firebase.RTDB.pushInt(&fbdo, "Jugador1/Aciertos", aciertos1)){
      Serial.println(); Serial.print(aciertos1);
      Serial.print(" - succesfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    //Enviamos los disparos fallidos del jugador1
    if(Firebase.RTDB.pushInt(&fbdo, "Jugador1/Fallos", fallos1)){
      Serial.println(); Serial.print(fallos1);
      Serial.print(" - succesfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    //Datos segundo jugador
    //Enviamos el nombre del jugador1
    if(Firebase.RTDB.pushString(&fbdo, "Jugador2/Nombre", jugador2)){
      Serial.println(); Serial.print(jugador2);
      Serial.print(" - succesfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    //Enviamos los aciertos del jugador1
    if(Firebase.RTDB.pushInt(&fbdo, "Jugador2/Aciertos", aciertos2)){
      Serial.println(); Serial.print(aciertos2);
      Serial.print(" - succesfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    //Enviamos los disparos fallidos del jugador1
    if(Firebase.RTDB.pushInt(&fbdo, "Jugador2/Fallos", fallos2)){
      Serial.println(); Serial.print(fallos2);
      Serial.print(" - succesfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    signupOK = false;
  }

  HTTPClient http;

  Serial.print("Realizando solicitud HTTP GET a: ");
  Serial.println(url);

  // Iniciar la conexión
  if (http.begin(url)) {
    // Realizar la solicitud HTTP GET
    int httpCode = http.GET();

    // Verificar el código de respuesta
    if (httpCode > 0) {
      // Imprimir el código de respuesta
      Serial.print("Código de respuesta HTTP: ");
      Serial.println(httpCode);

      // Leer la respuesta del servidor
      String payload = http.getString();
      Serial.println("Respuesta del servidor:");
      Serial.println(payload);
    } else {
      Serial.println("Error en la solicitud HTTP");
    }

    // Cerrar la conexión
    http.end();
  } else {
    Serial.println("Error al iniciar la conexión HTTP");
  }

  // Esperar antes de realizar la siguiente solicitud
  delay(5000);
}