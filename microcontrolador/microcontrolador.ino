#include "DHT.h"
#include "TSL2561.h"
#include <Wire.h>
#include "RTClib.h"

#define SIZE_BUFFER_DATA       150

#define PIN03 3
#define PIN04 4
#define PIN05 5
#define PIN06 6
#define PIN07 7
#define PIN08 8

#define PIN_FADE 10

// I2C device found at address 0x39  !  -->> TSL2561_1
// I2C device found at address 0x48  !  -->> No Aplica
// I2C device found at address 0x49  !  -->> TSL2561_2
// I2C device found at address 0x68  !  -->> DS3231 Clock

// CONFIG GENERAL
const char* idDevice = "LC01";
boolean     stringComplete = false;
String      inputString = "";
char        bufferData [SIZE_BUFFER_DATA];

// Declaramos un RTC DS3231
RTC_DS3231 rtc;

// Configuración de Sensor de Movimiento
int PIR_PIN = 9;
int pirState = LOW;             // we start, assuming no motion detected

// Configuración de Sensor de Sonido
#define ANALOG_SOUND_PIN 0
#define ADC_SOUND_REF 40
#define DB_SOUND_REF 50

#define SOUND_PIN A1

/* Configuración de Sensor de Iluminación
   
   Connections
   ===========
   Connect SCL to analog 5
   Connect SDA to analog 4
   Connect VDD to 3.3V DC
   Connect GROUND to common ground
 */
TSL2561 tsl_1(TSL2561_ADDR_FLOAT);
TSL2561 tsl_2(TSL2561_ADDR_HIGH);

// Configuración de Sensor de Temeratura y Humedad
#define DHTTYPE DHT11      // DHT 11
const int DHTPin = 2;
 
DHT dht(DHTPin, DHTTYPE);

// Configuración de Sensor de Gas
#define GAS_PIN A2
float Ro = 10000.0;    // this has to be tuned 10K Ohm

int index = 0;
int brillo = 0;

void setup() {
  Serial.begin(9600);
  inputString.reserve(150);

  // Comprobamos si tenemos el RTC conectado
  if (! rtc.begin()) {
    while (1);
  }
  // rtc.adjust(DateTime(2018, 5, 6, 13, 8, 50));

  pinMode(PIN03,    OUTPUT);
  pinMode(PIN04,    OUTPUT);
  pinMode(PIN05,    OUTPUT);
  pinMode(PIN06,    OUTPUT);
  pinMode(PIN07,    OUTPUT);
  pinMode(PIN08,    OUTPUT);
  pinMode(PIN_FADE, OUTPUT);
  tester_leds();
  
  pinMode(PIR_PIN, INPUT);     // declare sensor as input
  
  dht.begin();

  if (tsl_1.begin()) {
    // Serial.println("Found sensor1");
  } else {
    // Serial.println("No sensor1?");
    while (1);
  }

  if (tsl_2.begin()) {
    // Serial.println("Found sensor2");
  } else {
    // Serial.println("No sensor2?");
    while (1);
  }

  tsl_1.setGain(TSL2561_GAIN_16X);                // set 16x gain (for dim situations)
  tsl_1.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)

  tsl_2.setGain(TSL2561_GAIN_16X);                // set 16x gain (for dim situations)
  tsl_2.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
}

void loop() {

  if (index == 10) { 
    float  ppm   = 0.0;
    double db    = 0.0;
    int    lux_1 = 0;
    int    lux_2 = 0;
  
    // Temperatura
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)) {
      return;
    }
    
    if(t < 15.5 || t > 27.0) {
      digitalWrite(PIN04, HIGH);
    }
    else {
      digitalWrite(PIN04, LOW);
    }
  
    if(h < 40.0 || h > 60.0) {
      digitalWrite(PIN05, HIGH);
    }
    else {
      digitalWrite(PIN05, LOW);
    }
  
    // Sonido
    int sound_level = analogRead(SOUND_PIN);
        
    if(sound_level > 10) {
      db = get_abs_db_v2(sound_level);        
    
      if(db > 85) {
        digitalWrite(PIN07, HIGH);  
      }
      else {
        digitalWrite(PIN07, LOW);
      }
    }
    else {
      digitalWrite(PIN07, LOW);
    }
  
    // Iluminación #1
    uint32_t lum_1 = tsl_1.getFullLuminosity();
    uint16_t ir_1, full_1;
    ir_1 = lum_1 >> 16;
    full_1 = lum_1 & 0xFFFF;
    
    lux_1 = tsl_1.calculateLux(full_1, ir_1);    
    if(lux_1 < 100 || lux_1 > 700) {
      digitalWrite(PIN06, HIGH);  
    }
    else {
      digitalWrite(PIN06, LOW);
    }
  
    // Iluminación #2
    uint32_t lum_2 = tsl_2.getFullLuminosity();
    uint16_t ir_2, full_2;
    ir_2 = lum_2 >> 16;
    full_2 = lum_2 & 0xFFFF;
    
    lux_2 = tsl_2.calculateLux(full_2, ir_2);
    
    // Gas - Monóxido de Carbono
    int gas_value = analogRead(GAS_PIN);
    float Vrl = gas_value * ( 5.0 / 1024.0 );       // V
    float Rs = 20000 * ( 5.0 - Vrl) / Vrl ;         // Ohm 
    ppm = get_CO(Rs/Ro);
    
    if(ppm > 100) {
      digitalWrite(PIN03, HIGH);  
    }
    else {
      digitalWrite(PIN03, LOW);
    }
  
    // Movimiento
    int state = digitalRead(PIR_PIN);
    if (state == HIGH) { 
      if (pirState == LOW) {
        pirState = HIGH;
        digitalWrite(PIN08, HIGH);
      } 
    } else {
      if (pirState == HIGH) {
        pirState = LOW;
        digitalWrite(PIN08, LOW);
      }
    }
    DateTime now = rtc.now();
    String datetime = (String)now.day() + "-" + (String)now.month() + "-" + (String)now.year();
    datetime += "_" + (String)now.hour() + ":" + (String)now.minute() + ":" + (String)now.second();
    
    String base = (String)idDevice + "&" + datetime;
  
    String payload = base + "|T:" + (String)t + "|H:" + (String)h + "|S:" + (String)db + "|M:" + (String)ppm + "|LI:" + (String)lux_1 + "|LE:" + (String)lux_2 + "|P:" + (String)state + "@";
    Serial.println(payload);
    index = 0;
  }

//    analogWrite(PIN_FADE, 255);
//    // Efecto FADE-IN LED Blanco
//    for(brillo=0; brillo < 256; brillo++) {
//      analogWrite(PIN_FADE, brillo);
//      delay(50);
//    }
//    // Efecto FADE_OUT LED Blanco
//    for(brillo=255; brillo >=0; brillo--) {
//      analogWrite(PIN_FADE, brillo);
//      delay(50);
//    }
//  }

  receiveData();
  processData();
  
  index++;
  delay(500);  
}

void tester_leds() {
  digitalWrite(PIN03, LOW);
  digitalWrite(PIN04, LOW);
  digitalWrite(PIN05, LOW);
  digitalWrite(PIN06, LOW);
  digitalWrite(PIN07, LOW);
  digitalWrite(PIN08, LOW);
  delay(500);

  //LED03
  digitalWrite(PIN03, HIGH);
  delay(100);

  //LED04
  digitalWrite(PIN03, LOW);
  digitalWrite(PIN04, HIGH);
  delay(100);

  //LED05
  digitalWrite(PIN04, LOW);
  digitalWrite(PIN05, HIGH);
  delay(100);

  //LED06
  digitalWrite(PIN05, LOW);
  digitalWrite(PIN06, HIGH);
  delay(100);

  //LED07
  digitalWrite(PIN06, LOW);
  digitalWrite(PIN07, HIGH);
  delay(100);

  //LED08
  digitalWrite(PIN07, LOW);
  digitalWrite(PIN08, HIGH);
  delay(100);

  digitalWrite(PIN03, LOW);
  digitalWrite(PIN04, LOW);
  digitalWrite(PIN05, LOW);
  digitalWrite(PIN06, LOW);
  digitalWrite(PIN07, LOW);
  digitalWrite(PIN08, LOW);

  analogWrite(PIN_FADE, 255);
  delay(1000);
  analogWrite(PIN_FADE, 0);
}

double get_abs_db_v2(int input) {
   return 20 * log((double)input / (double)ADC_SOUND_REF) + DB_SOUND_REF;
}

// get CO ppm
float get_CO (float ratio){
  return 37143 * pow (ratio, -3.178);
}

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void receiveData() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;

    if (inChar == '\n') {
      inputString.toCharArray(bufferData, SIZE_BUFFER_DATA);
      stringComplete = true;
    }
  }
}

void processData() {
  if (stringComplete) {
    String result[2];
    processCommand(result, inputString);
    
    result[1].toCharArray(bufferData, SIZE_BUFFER_DATA);
    int index = atoi(bufferData);
      
    if(result[0] == "LIGHT_ON") {
      // Encender luz
      analogWrite(PIN_FADE, 255);
    }
    if(result[0] == "LIGHT_OFF") {
      // Apagar luz
      analogWrite(PIN_FADE, 0);
    }
    if(result[0] == "FADE") {
      // Fade
      analogWrite(PIN_FADE, index);
    }
    
    stringComplete = false;
    inputString = "";    
  }
}

// Methods that divides the command by parameters
void processCommand(String* result, String command) {
  int i = 0;
  char* token;
  char buf[command.length() + 1];
  
  command.toCharArray(buf, sizeof(buf));
  token = strtok(buf, ":");
  
  while(token != NULL) {
    result[i++] = token;
    token = strtok(NULL, ":");
  }  
}
