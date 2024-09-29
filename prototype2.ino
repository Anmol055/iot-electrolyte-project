//firebase definations
#define WIFI_SSID     "kalaana[4g]"
#define WIFI_PASSWORD "221kalaana@ezz"
#define REFERENCE_URL "https://iot-electrolyte-default-rtdb.firebaseio.com/"
#define AUTH_TOKEN "uLcI1meImRRXsjQATgligxhukUo31AMpWFThkDmK"


#include <Firebase.h>
#include <ArduinoJson.h>
/* Use the following instance for Test Mode (No Authentication) */
// Firebase fb(REFERENCE_URL);

/* Use the following instance for Locked Mode (With Authentication) */
Firebase fb(REFERENCE_URL, AUTH_TOKEN);
const int LED_BUILTIN = 2;

//hx711 definations
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//pins:
const int HX711_dout = 21; //mcu > HX711 dout pin
const int HX711_sck = 22; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {
  Serial.begin(115200);
  #if !defined(ESP32)
    WiFi.mode(WIFI_STA);
  #else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
  #endif
  WiFi.disconnect();
  delay(1000);

  /* Connect to WiFi */
  Serial.println();
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("-");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println();

  #if defined(ESP32)
    digitalWrite(LED_BUILTIN, HIGH);
  #endif


//Loadcell
LoadCell.begin();
//LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 276.42; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  bool _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
}


void loop() {
  /* ----- Serialization: Set example data in Firebase ----- */

  // Create a JSON document to hold the output data
  JsonDocument docOutput;

  static bool newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  //if (LoadCell.update()) newDataReady = true;
  newDataReady = LoadCell.update();
  // get smoothed value from the dataset:
  if (newDataReady) {
    // if (millis() > t + serialPrintInterval) {
      // Add various data types to the JSON document
      float i = LoadCell.getData();
      docOutput["Load Cell reading"] = i;

      t = millis();
      docOutput["time elapsed in milliseconds"] = t;
      
      // Create a string to hold the serialized JSON data
      String output;

      // Optional: Shrink the JSON document to fit its contents exactly
      docOutput.shrinkToFit();

      // Serialize the JSON document to a string
      serializeJson(docOutput, output);

      // Set the serialized JSON data in Firebase
      fb.setJson("parameters", output);
      
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      
      // fb.setFloat("Load Cell reading",i);
      // fb.set("time elapsed in seconds",t/1000);
      //delay(10);
      // fb.remove("Measured Weight");  
      newDataReady = 0;
      
    //}
  }
  
  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}