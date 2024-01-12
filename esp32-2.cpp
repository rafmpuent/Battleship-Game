#include <Arduino.h>
#include <cmath>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include "apwifiesp32.h"
#ifdef __AVR__
    #include <avr/power.h>
#endif

//Declaracion de funciones
int coordenadasAIndice(int x, int y);
void sendShipData();
void sendShootData();
void receiveShipData();
void receiveShootData();
int determineWinnerByProximity();
void madeInHeaven();

//Configuracion OLED 
Adafruit_SSD1306 display(-1);

#define DIS_WIDTH 128 // OLED display width, in pixels
#define DIS_HEIGHT 64 // OLED display height, in pixels

#define IMG_HEIGHT   128
#define IMG_WIDTH    64

#define OLED_RESET -1
#define ANCHO 128 //Ancho de la OLED
#define ALTO 64 //Alto de la OLED

Adafruit_SSD1306 oled(ANCHO, ALTO, &Wire, OLED_RESET); //Configuracion OLED

//Matriz Leds
#define PIN 12 //Se usa el pin 12 para la transmision de datos en la tira de leds
#define NUMLEDS 50 //Numero de leds WS2812B

//Variables por jugador
#define NUM_SHIPS 6 //Numero de barcos 
#define NUM_SHOOTS 12 //Numero de disparos

//Pines Joystick
#define SW_PIN 15
#define X_PIN 36
#define Y_PIN 39

#define PSTART 34
#define PORIENTATION 35 //2

//Enume para orientacion de barcos
enum Orientation {
  HORIZONTAL, //0
  VERTICAL    //1
};

int brillo = 2; //Brillo de los leds
Adafruit_NeoPixel tira = Adafruit_NeoPixel(NUMLEDS, PIN, NEO_GRB + NEO_KHZ800);

//Variables antirrebote
volatile int lastDebounceTime = 0;
volatile int debounceDelay = 25;

//Declaracion de variables de juego
volatile int barcoEnemigo; //Variable para almacenar el numero del barco enemigo
bool ShipsMode = false; //Bandera para saber si se esta en modo ubicacion de barcos
bool inicioJuego = false; //Bandera para saber si se ha iniciado el jueego
bool inicioGuerra = false; // Bandera para saber si inicio la guerra
volatile int aciertos1 = 0;
volatile int fallidos1 = 0;
int aciertos2 = 0;
int fallidos2 = 0;
volatile bool endGame = false; //Bandera para saber si el juego se ha acabado
volatile boolean unableShips = false;
volatile int barco = 0; //Numero de barco actual
volatile boolean shipHasSank = false;//Un barco ha sido hundido
volatile boolean shipHasNoSank = false;//Un barco no ha sido hundido
volatile int shoots1 = 0; //Numero de disparo actual
volatile int shoots2 = 0; //Numero de disparo del enemigo
volatile boolean player1Ready = false; //El jugador 1 ha terminado de ubicar sus barcos
volatile boolean player2Ready = false; //El jugador 2 ha terminado de ubicar sus barcos
volatile boolean turnoPlayer1 = false; //Turno del jugador 1 para realizar los disparos
volatile boolean turnoPlayer2 = false; //Turno del jugador 2 para realizar los disparos
volatile boolean sendShootInfo = false; //We send Shoot data.
String response = "Waiting for something to happen?";
//Arreglos para almacenar las distancias minimas entre los 6 barcos y los disparos
double proximity1[NUM_SHIPS] = {0, 0, 0, 0, 0, 0};
double proximity2[NUM_SHIPS] = {0, 0, 0, 0, 0, 0};
int ganador = 0; //Ganador del juego
volatile int jugador = 0; //Numero jugador
volatile int enemigo = 0; //Numero enemigo

//Colores Leds
uint32_t amarillo  = tira.Color(150, 150, 0);
uint32_t azul      = tira.Color(0, 0, 150);
uint32_t blanco    = tira.Color(150, 150, 150);
uint32_t rojo      = tira.Color(150, 0, 0);
uint32_t verde     = tira.Color(0, 150, 0);
uint32_t off       = tira.Color(0, 0, 0);

//Creamos una estructura para almacenar los datos de los barcos
typedef struct Ship {
  int n; //Numero de barco
  int x; //Coordenada X del barco en su ubicacion actual
  int y; //Coordenada Y del barco en su ubicacion actual
  int size; //Tamano del barco
  Orientation orientation; //Orientacion del barco
  bool isActive; //Indica si el barco ha sido puesto en pantalla
  bool isSank; //Indica si el barco ha sido hundido
}Ship_t;

//Creamos una estructura para los disparos
typedef struct Shoot {
  int n; //Numero de disparo
  int x; //Coordenada X del disparo en su ubicacion actual
  int y; ////Coordenada Y del disparo en su ubicacion actual
}Shoot_t;

//Disparos jugador
Shoot_t playerShoots[NUM_SHOOTS];

//Barcos jugador
Ship_t playerShips[NUM_SHIPS];

//Barcos enemigos
Ship_t enemyShips[NUM_SHIPS];

//Disparos enemigos 
Shoot_t enemyShoots[NUM_SHOOTS];

//Funciones OLED
// 'logo-espol', 64x64px
const unsigned char epd_bitmap_logo_espol [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0x0f, 0x00, 0x3c, 0x00, 0xf0, 0x01, 0xe0, 0x60, 0x3f, 0xc0, 0xff, 0x01, 0xfc, 0x07, 0xf0, 0x60, 
0x79, 0xe1, 0xe7, 0x83, 0x9e, 0x0e, 0x38, 0x60, 0x60, 0xe3, 0x81, 0x87, 0x07, 0x1c, 0x1c, 0x60, 
0xe1, 0xc7, 0x01, 0xc6, 0x03, 0x18, 0x0c, 0x60, 0xff, 0x8e, 0x00, 0xce, 0x03, 0x18, 0x0c, 0x60, 
0xff, 0x1c, 0x00, 0xce, 0x03, 0x18, 0x0c, 0x60, 0xe0, 0x38, 0x01, 0xce, 0x03, 0x18, 0x0c, 0x70, 
0xe0, 0x70, 0x01, 0x8f, 0x07, 0x1c, 0x1c, 0x30, 0x79, 0xe0, 0x07, 0x8f, 0x9e, 0x0f, 0x38, 0x3c, 
0x3f, 0xc0, 0x1f, 0x0f, 0xfc, 0x07, 0xf0, 0x1f, 0x0f, 0x00, 0x1c, 0x0e, 0xf0, 0x01, 0xc0, 0x07, 
0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Array de todos los bitmaps para conveniencia.
const int epd_bitmap_allArray_LEN = 1;
const unsigned char* epd_bitmap_allArray[1] = {
    epd_bitmap_logo_espol
};

void showWinner() {
  display.clearDisplay();
  display.setTextColor(WHITE);	// establece color al unico disponible (pantalla monocromo)
  if(ganador != -1) {
    display.setCursor(0, 0);		// ubica cursor en inicio de coordenadas
    display.print("WINNER:"); 	// escribe en pantalla el texto
    display.setCursor (60, 20); // ubica cursor en coordenas 10,30
    display.setTextSize(1);		  // establece tamano de texto en 2
    display.print(ganador);		  // escribe valor de millis() dividido por 1000
    display.display();			    // muestra en pantalla todo lo establecido anteriormente  
  }
  else{
    display.setCursor(15, 15); // ubica cursor en medio de la OLED
    display.setTextSize(2);		 // establece tamano de texto en 1
    display.print("EMPATE"); 	 // escribe en pantalla el texto
    display.display();			   // muestra en pantalla todo lo establecido anteriormente  
  }
  delay(500);
  display.clearDisplay();
  display.display();
}

//Funcion para la deteccion de colision
boolean detectCollision(){
  Ship_t *currentShip = &playerShips[barco]; //Barco actual
  for (int i = 0; i < barco; i++) {
    if (i != barco) {
      Ship_t *otherShip = &playerShips[i];
      for (int j = 0; j < otherShip->size; j++) { 
        int otherX = otherShip->x + (otherShip->orientation == HORIZONTAL ? j : 0);
        int otherY = otherShip->y + (otherShip->orientation == VERTICAL ? j : 0);
        for(int k = 0; k < currentShip->size; k++){
          int currentX = currentShip->x + (currentShip->orientation == HORIZONTAL ? k : 0);
          int currentY = currentShip->y + (currentShip->orientation == VERTICAL ? k : 0);
          if(otherX == currentX && otherY == currentY) {
            return true;
          };
        }
      }
    }
  }
  return false;
}

//Funciones para el encendido de leds
void showLed(int x, int y, uint32_t color) {
    int indice = coordenadasAIndice(x, y);
    tira.setPixelColor(indice, color);
}

/*Led unico para indicar el estado del jugador
Amarillo: Jugador en modo ubicacion de barcos
Verde: Turno del jugador para realizar el disparo
Rojo: Turno del rival para realizar el disparo
*/
void turnoLed(uint32_t color) {
  tira.setPixelColor(49, color);
  tira.show();
}

// Muestra al barco que ha sido hundido por un periodo de tiempo
void showShipSank(Ship_t *shipsArray) {
  if(shipHasSank) {
    Ship_t *enemyShip = &shipsArray[barcoEnemigo];
    for(int i = 0; i < enemyShip->size; i++) {
      int x = enemyShip->x + (enemyShip->orientation == HORIZONTAL ? i : 0);
      int y = enemyShip->y + (enemyShip->orientation == VERTICAL ? i : 0);
      showLed(x, y, rojo);
    }
    tira.show();
    player.play(3);
    delay(500); // Breve pausa antes de apagar el barco
    tira.clear();
    if(aciertos1 == 6) {
      Serial2.write("ea-"); //endGame due to aciertos //En caso de error moverlo a shipHasSank
      inicioJuego  = false; //Se deshabilita el modo de ubicación de disparos
      endGame      = true; //Se acabo el juego
    }
  }
  barcoEnemigo = -1; //Reiniciamos la cuenta de barco enemigo
  shipHasSank  = false; //Reiniciamos el mostrar que el barco se ha hundido.
}

//Mostramos que el disparo se ha fallado
void showShipNoSank(Shoot_t *shootsArray, int disparo){
  if(shipHasNoSank) {
    Shoot_t *currentShoot = &shootsArray[disparo]; //Para este punto el disparo ya habra sido aumentado, por lo que, el disparo fallido fue el previo
    showLed(currentShoot->x, currentShoot->y, azul); //Encendemos la posición del disparo fallido de color azul
    tira.show();
    delay(500); //Se apaga despues de medio segundo
    tira.clear();
    tira.show();
  }
  shipHasNoSank = false; //Reiniciamos el mostrar que el disparo se ha fallado.
}

//Verificar si el disparo ha impactado a uno de los barcos
bool checkHit(int x, int y, Ship_t *enemyShip) {
    for (int i = 0; i < enemyShip->size; i++) {
        int shipX = enemyShip->x + (enemyShip->orientation == HORIZONTAL ? i : 0);
        int shipY = enemyShip->y + (enemyShip->orientation == VERTICAL ? i : 0);
        if (x == shipX && y == shipY && enemyShip->isSank == false) {
            barcoEnemigo = enemyShip->n; //Barco al que hemos hundido
            enemyShip->isSank = true; //Si el disparo impacto el barco, lo hunde y ya no se lo vuelve a contar como acierto
            return true; // El disparo acertó al barco enemigo
        }
    }
    return false; // El disparo no acertó al barco enemigo
}

//Verificamos si el disparo ha hundido a alguno de los barcos
bool shipSank(Shoot_t *currentShoot, Ship_t *shipsArray) {
  // Verificar si el disparo acertó a algún barco enemigo
  for (int i = 0; i < NUM_SHIPS; i++) {
      Ship_t *enemyShip = &shipsArray[i];
      if (checkHit(currentShoot->x, currentShoot->y, enemyShip)) {
          return true; //Acerto a uno de los barcos
      }
  }
  return false; //No acerto a ningun barco
}

//Funcion de interrupcion que me permite cambiar al siguiente barco
void IRAM_ATTR changeShip() {
  unsigned long currentMillis = millis();
  int switchState = digitalRead(SW_PIN);
  if (currentMillis - lastDebounceTime > debounceDelay) { //Antirrebote
    if(switchState == HIGH){ //Se solto el pulsador
      //Modo ubicacion de barcos|Permite que no haya colision de barcos
      if(!detectCollision() && barco < NUM_SHIPS){ 
        barco++; //Cambio al siguiente barco
        if(barco < NUM_SHIPS) {
          Ship_t *currentShip   = &playerShips[barco]; //Accedo al siguiente barco
          currentShip->isActive = true; //Activo el siguiente barco
        }
      }
      //Si estamos en modo guerra y se ha colocado el ultimo barco
      if(inicioGuerra && turnoPlayer1 && player2Ready && shoots1 < NUM_SHOOTS){
        //Si acertamos a un barco incrementamos un acierto, caso contrario un disparo fallido
        Shoot_t *currentShoot = &playerShoots[shoots1]; //Puntero a la estructura de 12 disparos
        if(shipSank(currentShoot, enemyShips)) {
          aciertos1++;
          //Si el jugador ha derrumbado los 6 barcos del enemigo, se termina el juego 
          shipHasSank = true; //Un barco ha sido hundido
          }
        else {
          fallidos1++;
          shipHasNoSank = true; //Se ha fallado el disparo
        }
        turnoPlayer1 = false; //Cuando se realiza el disparo es turno del jugador 2
        turnoPlayer2 = true;
        sendShootInfo = true; //We send Shoot data.
        shoots1++; //Se pasa al siguiente disparo
      }
      else if(barco == NUM_SHIPS && !player1Ready){ //Esto se debe ejecutar una sola vez
        ShipsMode = false; //Ya no se permite el movimiento de barcos
        unableShips = true; //Señal para apagar los barcos
        inicioGuerra = true; //Empezamos el modo guerra
        if(player2Ready) {
          turnoPlayer1 = false;//Si cuando el jugador ubica su ultimo barco el jugador 2 ya ha ubicado sus barcos, entonces el jugador 2 empieza el modo disparo
          jugador = 2; //Este jugador será el jugador 1
          enemigo = 1; //El enemigo será el jugador 2
        } 
        else {
          turnoPlayer1 = true; //Caso contrario el jugador 1 inicia el turno en modo disparo
          jugador = 1; //Este jugador será el jugador 1
          enemigo = 2; //El enemigo será el jugador 2
        }
        player1Ready = true; //El jugador 1 ha terminado de ubicar sus barcos
        Serial2.write("f-"); //Enviamos señal de que el jugador ha finalizado de ubicar sus barcos
      }
    }
  }
  lastDebounceTime = currentMillis;
}

void IRAM_ATTR changeOrientation() { //No se considera el rebote porque no es algo tan critico como la colocacion de barcos
//En caso de querer, se puede volver a cambiar la orientacion del barco
  detachInterrupt(digitalPinToInterrupt(PORIENTATION));
  Ship_t *currentShip = &playerShips[barco]; //Accedo al siguiente barco
  switch(currentShip->orientation){
    case 0:
      currentShip->orientation = VERTICAL;
      break;
    case 1:
      currentShip->orientation = HORIZONTAL;
      break;
  }
  attachInterrupt(digitalPinToInterrupt(PORIENTATION), changeOrientation, RISING);
}

//Convierte las coordenadas 2D a un indice 1D el cual indica el numero de led a encender
int coordenadasAIndice(int x, int y){
    return y * 7 + x;
}

//OLED
void setupOLED(){
  Wire.begin();					// inicializa bus I2C
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);	// inicializa pantalla con direccion 0x3C
  oled.clearDisplay(); // limpia pantalla cada que se alimenta el microcontrolador
  oled.display(); //Muestra los cambios
}

void showShots(){
  display.clearDisplay();
  display.setTextColor(WHITE);		// establece color al unico disponible (pantalla monocromo)
  display.setCursor(0, 0);			// ubica cursor en inicio de coordenadas 0,0
  display.setTextSize(1);		// establece tamano de texto en 1
  display.print("Aciertos: "); 	// escribe en pantalla el texto
  display.print(aciertos1); 	// escribe en pantalla el texto
  display.setCursor (0, 20);		// ubica cursor en coordenas 10,30
  display.setTextSize(1);			// establece tamano de texto en 2
  display.print("Fallos: ");
  display.print(fallidos1); 	// escribe en pantalla el texto
  display.display();			// muestra en pantalla todo lo establecido anteriormente
}

void setupLedBoard() {
    tira.begin(); //inicializacion de la tira
    tira.setBrightness(brillo); //Brillo de los LEDS
    tira.show();  //Muestras datos en pixel
}

//Movimiento de los barcos en la matriz de leds.
void moveShips() {
  Ship_t *currentShip = &playerShips[barco]; //Puntero a la estructura de 6 barcos
  //Valores del joystick en ambos ejes
  int joystickX = analogRead(X_PIN); 
  int joystickY = analogRead(Y_PIN);
  //Movimiento
  if(joystickX < 1000) currentShip->y--;
  else if(joystickX > 3000) currentShip->y++;
  if(joystickY < 1000) currentShip->x++;
  else if(joystickY > 3000) currentShip->x--;
  //Limita los bordes de la matriz para que el usuario no se pueda salir de lugar
  currentShip->x = constrain(currentShip->x, 0, 7 - (currentShip->orientation == HORIZONTAL ? currentShip->size : 1));
  currentShip->y = constrain(currentShip->y, 0, 7 - (currentShip->orientation == VERTICAL ? currentShip->size : 1));
}

// Función para actualizar la matriz con la posición de los barcos
void updateMatrix() {
  tira.clear();
  // Dibujar los barcos del jugador
  for (int i = 0; i < NUM_SHIPS; i++) {
    Ship_t *currentShip = &playerShips[i];
    if(currentShip->isActive){
      //uint32_t color = (currentShip->isActive) ? blanco : off; // blanco si el barco está seleccionado, apagado si no
      for (int j = 0; j < currentShip->size; j++) {
        int x = currentShip->x + (currentShip->orientation == HORIZONTAL ? j : 0);
        int y = currentShip->y + (currentShip->orientation == VERTICAL ? j : 0);
        showLed(x, y, blanco);
      }
    }
  }
  tira.show();
}

//Movimiento de los disparos
void moveShoots() {
  Shoot_t *currentShoot = &playerShoots[shoots1]; //Puntero a la estructura de 12 disparos
  //Valores del joystick en ambos ejes
  int joystickX = analogRead(X_PIN); 
  int joystickY = analogRead(Y_PIN);
  //Movimiento
  if(joystickX < 1000) currentShoot->y--;
  else if(joystickX > 3000) currentShoot->y++;
  if(joystickY < 1000) currentShoot->x++;
  else if(joystickY > 3000) currentShoot->x--;
  //Limita los bordes de la matriz para que el usuario no se pueda salir de lugar
  currentShoot->x = constrain(currentShoot->x, 0, 6);
  currentShoot->y = constrain(currentShoot->y, 0, 6);
}

// Función para actualizar la matriz con la posición de los barcos
void updateMatrixShoots() {
  if(shoots1 < NUM_SHOOTS) {
    tira.clear();
    Shoot_t *currentShoot = &playerShoots[shoots1];
    showLed(currentShoot->x, currentShoot->y, amarillo);
    tira.show();
  }
}

//Inicia el juego y permite la ubicacion de los barcos
void startGame() {
  turnoLed(amarillo);
  moveShips();
  updateMatrix();
  delay(250); //Esperamos un cuarto de segundo entre barcos
}

void startWar(){
  showShipSank(enemyShips); //Si se ha hundido un barco, se muestra parpadeante.
  showShipNoSank(playerShoots, shoots1 - 1); //Si se ha fallado un disparo, se muestra de color azul.
  if(sendShootInfo) sendShootData();//Enviamos los datos del disparo y señal para que indicar que es su turno
  if(turnoPlayer1 && player2Ready) { //Si es turno del jugador y el otro jugador ya termino de ubicar todos sus barcos, se puede mover el disparo
    turnoLed(verde);
    moveShoots();
  }
  updateMatrixShoots();
  if(turnoPlayer2) turnoLed(rojo); //Si es turno del jugador 2, se prende el led de color rojo
  showShots(); //En cada iteracion se muestra los aciertos y los disparos fallidos
  delay(250); //Permite regular velocidad
}

void setup() {
  pinMode(SW_PIN, INPUT_PULLUP); //El pulsador del joystick se define como entrada
  pinMode(PSTART, INPUT_PULLUP); //El pulsador grande de inicio se define como entrada
  pinMode(PORIENTATION, INPUT_PULLUP); //Pulsador para el cambio de orientacion del barco
  setupLedBoard(); //Inicializa la matriz Led
  initAP("espAP","123456789");
  Serial.begin(115200); //USB
  Serial2.begin(115200); // Inicia UART2 con baudrate 9600, 8 bits de datos, sin paridad, 1 bit de parada
  attachInterrupt(digitalPinToInterrupt(SW_PIN), changeShip, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PORIENTATION), changeOrientation, RISING); 
  //Setup display oled
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  Serial.println(F("SSD1306 está fallando"));
  for (;;); // No proceder, hacer un bucle infinito
  }
  delay(2000); // Pausa de 2 segundos
  //Mostrar la imagen al inicio
  int xOffset = (DIS_WIDTH - IMG_WIDTH) / 2;
  int yOffset = (DIS_HEIGHT - IMG_HEIGHT) / 2 + 10; // Mueve la imagen 10 píxeles hacia abajo
  display.clearDisplay();
  display.drawBitmap(xOffset, yOffset, epd_bitmap_allArray[0], IMG_WIDTH, IMG_HEIGHT, WHITE);
  display.display();
  delay(2000);
}

void checkSerial() {
  while(Serial2.available()){ //Comunicacion entre esps32
    response = Serial2.readStringUntil('-');
    if(response.equals("f")) player2Ready = true; //EL jugador 2 ha ubicado todos sus barcos
    if(response.equals("sendShipData")) receiveShipData(); //Recibo de datos de barcos enemigos
    if(response.equals("d")) {
      shoots2++; //Se aumenta el disparo del jugador rival
      receiveShootData();
      //Lo ponemos abajo para que el jugador pueda realizar movimientos una vez que se haya mostrado el disparo
      turnoPlayer1 = true; //El jugador 2 ha realizado su disparo y es turno del jugador 1
    }
    if(response.equals("ea")) { //Señal de que se acabo el juego debido a aciertos del enemigo
      aciertos2 = 6; 
      inicioJuego = false; //Se deshabilita los disparos
      endGame = true; //Se acabo el juego
    }
  }
}

void sendShipData() {
  for(int i = 0; i < NUM_SHIPS; i++) {
    Ship_t *currentShip = &playerShips[i];
    Serial2.println(currentShip->x);
    Serial2.println(currentShip->y);
    Serial2.println(currentShip->orientation);
  }
}

void receiveShipData(){
  for(int i = 0; i < NUM_SHIPS; i++) {
    Ship_t *enemyShip      = &enemyShips[i]; //Barco enemigo
    enemyShip->x           = Serial2.parseInt();
    enemyShip->y           = Serial2.parseInt();
    enemyShip->orientation = Serial2.parseInt() == 0 ? HORIZONTAL : VERTICAL;
  }
  for(int i = 0; i < NUM_SHIPS; i++) {
    Ship_t *enemyShip = &enemyShips[i]; //Barco enemigo
  }
}

void sendShootData(){
  Serial2.write("d-"); //Enviamos señal de que se ha realizado el disparo
  Shoot_t *currentShoot = &playerShoots[shoots1 - 1]; //Se resta 1 porque para en la interrupcion se paso ya al siguiente pero deseamos mostrar el anterior
  Serial2.println(currentShoot->n);
  Serial2.println(currentShoot->x);
  Serial2.println(currentShoot->y);
  sendShootInfo = false;
}

void receiveShootData(){
  Shoot_t *enemyShoot = &enemyShoots[Serial2.parseInt()];
  enemyShoot->x = Serial2.parseInt();
  enemyShoot->y = Serial2.parseInt();
  if(shipSank(enemyShoot, playerShips)) {
    aciertos2++;
    shipHasSank = true; //Se activa para mostrar el barco hundido
    showShipSank(playerShips);
  }
  else {
    shipHasNoSank = true; //Se activa para mostrar la cuadricula azul
    fallidos2++;
    showShipNoSank(enemyShoots, enemyShoot->n);
  }
}

void defineWinner() {
  if(aciertos1 == 6 || (aciertos1 > aciertos2)) ganador = jugador; //El jugador 1 es el ganador
  else if(aciertos2 == 6 || (aciertos2 > aciertos1)) ganador = enemigo; //El jugador 2 es el ganador
  else ganador = determineWinnerByProximity(); //Probar
}

double calculateDistance(int x1, int y1, int x2, int y2) {
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

void determineProximity(Shoot_t *shoots, Ship_t *ships, double *proximity) {
  for (int i = 0; i < NUM_SHIPS; i++) {
    double minDistance = 1000; // Inicializar con un valor grande
    Ship_t Ship = ships[i];
    for (int j = 0; j < NUM_SHOOTS; j++) {
      Shoot_t Shoot = shoots[j];
      double distance;
      for(int k = 0; k < Ship.size; k++) {
        int ShipX = Ship.x + (Ship.orientation == HORIZONTAL ? k : 0);
        int ShipY = Ship.y + (Ship.orientation == VERTICAL   ? k : 0);
        distance = calculateDistance(Shoot.x, Shoot.y, ShipX, ShipY);
        if (distance < minDistance) {
          minDistance = distance;
        }
      }
    }
    //Afuera de este bucle for, minDistance habra tomado el valor minimo de distancia que hay entre el barco y cada uno de los disparos
    proximity[i] = minDistance;
    //En cada iteracion almacenamos la distancia minima entre los disparos y cada uno de los barcos
  }
}

int determineWinnerByProximity() {
  int n1 = 0;
  int n2 = 0;
  determineProximity(enemyShoots, playerShips, proximity1); //Distancia entre barcos propios y disparos enemigos
  determineProximity(playerShoots, enemyShips, proximity2);
  for(int i = 0; i < NUM_SHIPS; i++){
    if(proximity1[i] < proximity2[i]) n1++;
    else if(proximity2[i] < proximity1[i]) n2++;
  }
  if(n1 > n2) ganador = enemigo;
  else if(n2 > n1) ganador = jugador;
  else ganador = -1; //Señal de empate
  return ganador;
}

void madeInHeaven() {
  //Restablecemos la OLED
  // Mostrar la imagen al inicio
  int xOffset = (DIS_WIDTH - IMG_WIDTH) / 2;
  int yOffset = (DIS_HEIGHT - IMG_HEIGHT) / 2 + 10; // Mueve la imagen 10 píxeles hacia abajo
  display.clearDisplay();
  display.drawBitmap(xOffset, yOffset, epd_bitmap_allArray[0], IMG_WIDTH, IMG_HEIGHT, WHITE);
  display.display();
  // delay(2000);
  //Inicializacion de variables
  inicioJuego   = true; //Enclavamiento
  ShipsMode     = true;
  inicioGuerra  = false; // Bandera para saber si inicio la guerra
  aciertos1     = 0;
  fallidos1     = 0;
  aciertos2     = 0;
  endGame       = false; //Bandera para saber si el juego se ha acabado
  barco         = 0; //Numero de barco actual
  shoots1       = 0; //Numero de disparo actual
  shoots2       = 0; //Numero de disparo del enemigo
  player1Ready  = false; //El jugador 1 ha terminado de ubicar sus barcos
  player2Ready  = false; //El jugador 2 ha terminado de ubicar sus barcos
  turnoPlayer1  = false; //Turno del jugador 1 para realizar los disparos
  turnoPlayer2  = false; //Turno del jugador 2 para realizar los disparos  
  response      = "Waiting for something to happen?";
  //Inicializacion de los disparos tanto del jugador como del enemigo
  for(int i = 0; i < NUM_SHOOTS; i++) {
    playerShoots[i] = {i, 3, 3};
    enemyShoots[i]  = {i, 3, 3};
  }
  //Se inicializa cada una de las estructuras
  //Inicializamos los barcos del jugador
  playerShips[0] = {0, 4, 6, 3, HORIZONTAL, true,  false};//Barco horizontal de 3 puntos
  playerShips[1] = {1, 0, 6, 3, VERTICAL  , false, false};//Barco vertical de 3 puntos
  playerShips[2] = {2, 1, 5, 2, VERTICAL  , false, false};//Barco vertical de 2 puntos
  playerShips[3] = {3, 2, 4, 5, HORIZONTAL, false, false};//Barco horizontal de 5 puntos
  playerShips[4] = {4, 2, 0, 2, HORIZONTAL, false, false};//Barco horizontal de 2 puntos
  playerShips[5] = {5, 0, 0, 2, HORIZONTAL, false, false};//Barco horizontal de 2 puntos
  //Inicializamos los barcos del enemigo
  enemyShips[0] = {0, 4, 6, 3, HORIZONTAL, true,  false};//Barco horizontal de 3 puntos
  enemyShips[1] = {1, 0, 6, 3, VERTICAL  , false, false};//Barco vertical de 3 puntos
  enemyShips[2] = {2, 1, 5, 2, VERTICAL  , false, false};//Barco vertical de 2 puntos
  enemyShips[3] = {3, 2, 4, 5, HORIZONTAL, false, false};//Barco horizontal de 5 puntos
  enemyShips[4] = {4, 2, 0, 2, HORIZONTAL, false, false};//Barco horizontal de 2 puntos
  enemyShips[5] = {5, 0, 0, 2, HORIZONTAL, false, false};//Barco horizontal de 2 puntos
}

void loop() {
  if(!inicioJuego) loopAp();
  if(digitalRead(PSTART) == LOW && !inicioJuego && namesReceive) {    
    madeInHeaven(); //Inicializamos todas las estructuras en "0".
    Serial2.write("s-");//Señal para que la otra esp32 empiece el juego
  }
  checkSerial(); //Chequeamos el Serial en cada iteracion
  if(inicioJuego){
    if(ShipsMode) startGame(); //Modo ubicacion de los barcos
    if(unableShips && player1Ready && player2Ready) {
      Serial2.write("sendShipData-");
      sendShipData(); //Enviamos los datos de los barcos de este jugador
      unableShips = false; //No se ejecute continuamente
    } 
    //Pasamos a modo guerra cuando ambos jugadores esten listos y cuando sea el turno del jugador
    if(inicioGuerra) startWar();
  }
  if(endGame){ //Si finalizo el juego, se muestra al ganador por pantalla
    turnoLed(off); //Se apaga el led cuando finaliza la partida
    tira.clear();  //Apagamos Las matrices
    tira.show();
    defineWinner();
    showWinner();
    inicioGuerra = false;
    inicioJuego = false; //Se acabo el juego
    server.send(200, "text/plain", String(aciertos1) + "-" + String(fallidos1) + "-" + String(aciertos2) + "-"  + String(fallidos2));
  }
  //Si se acaban los disparos de ambos jugadores se acaba el juego
  if(shoots1 >= NUM_SHOOTS && shoots2 >= NUM_SHOOTS) { 
    endGame = true;
  }
}