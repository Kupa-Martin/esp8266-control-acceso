#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

/* Hay que mapear los pines de Arduino UNO al pinout del esp8266

// Pins RFID (MEGA | UNO){
//   SDA  -> 53 | 10
//   SCK  -> 52 | 13
//   MOSI -> 51 | 11
//   MISO -> 50 | 12
//   IRQ  -> NO CONECTADO
//   GND  -> GND | GND
//   RST  ->  9 |  9
//   3.3V -> 3.3V | 3.3V
// }

// Pins LCD (MEGA | UNO){
//   GND -> GND | GND
//   VCC -> 5V  | 5V
//   SDA -> 20  | A4
//   SCL -> 21  | A5
// }

*/
#define RST_PIN 9                   //Pin para el reset del RC522
#define SS_PIN 53                   //Pin para el SS (SDA) del RC522

#define SSID "ssid"
#define PASSWORD "password"
#define SERVER_NAME "192.168.0.100"
#define PORT 3000
#define HTTPS false

MFRC522 mfrc522(SS_PIN, RST_PIN);   //Creamos el objeto para el RC522
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Function Declarations
String readCardId(void);
void connectWiFi(void);
//

// Main
void setup() {
  Serial.begin(9600);  //Iniciamos la comunicación  serial (hay que modificar los baudios para el esp8266)
  SPI.begin();         //Iniciamos el Bus SPI
  mfrc522.PCD_Init();  // Iniciamos  el MFRC522

  lcd.init();
  lcd.backlight();
  lcd.print("ET 35 DE 18");

  connectWiFi();

  Serial.println("Finished setup");
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  String cardId= readCardId();
  if (cardId == "") return; // readCardId() failed
  
  
  Serial.print("Read card: ");
  Serial.println(cardId);

  

  // Begin connection with the server
  //to do: implement HTTPS using WiFiClientSecure
  WiFiClient client;
  HTTPClient http;
  http.begin(client, SERVER_NAME, PORT, "/", HTTPS);
  http.addHeader("Content-Type", "application/json");
  
  // Make a json doc
  StaticJsonDocument<JSON_OBJECT_SIZE(1)> reqBody;
  reqBody["cardId"]= cardId;
  String reqBodyAsStr= "";
  serializeJson(reqBody, reqBodyAsStr);

  // Make the request
  //to do: stream json directly through the WiFiClient object
  t_http_codes statusCode= http.sendRequest("GET", reqBodyAsStr);

  switch (statusCode) {
    case HTTP_CODE_OK: {
      Serial.println(http.getString());
    }
    default: {
      Serial.printf("default: %d", statusCode);
    }
  }

}
//


// Function Definitions

String readCardId(void) {

  
  // Si no hay una tarjeta, no hacer nada
  if (!mfrc522.PICC_IsNewCardPresent()) { 
    //to do: update lcd
    return "";
  }
  
  Serial.println("Card Present");

  // Si no se pudo leer?, no hacer nada
  if (!mfrc522.PICC_ReadCardSerial()) {
    //to do: update lcd
    Serial.println("no entiendo exactamente en q casos se dispara este if");
    return "";
  }

  String buffer= "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    buffer += String(mfrc522.uid.uidByte[i], HEX);
  }

  // Finished reading
  mfrc522.PICC_HaltA();
  return buffer;
}

void connectWiFi(void) {
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Connecting");

  do {
    switch (WiFi.status()) {
      case WL_CONNECTED: {
        Serial.println("");
        Serial.print("Connected to WiFi network with IP Address: ");
        Serial.println(WiFi.localIP());
        //to do: lcd feedback
        return;
      }
      case WL_IDLE_STATUS: {
        // connecting
        Serial.print(".");
        continue;
      }
      case WL_CONNECT_FAILED: {
        Serial.println("Connection failed. Retrying...");
        //to do: lcd feedback
        break;
      }
      case WL_WRONG_PASSWORD: {
        //to do: lcd feedback
        
        // !!!!!!!!! If the code is rewritten such that the password isn't hardcoded, this should be removed !!!!!!!!!
        Serial.println("SSID: " SSID);
        Serial.println("Password: " PASSWORD);
        Serial.println("Got incorrect password. The password is hardcoded, the board must be reflashed");
        Serial.println("Sleeping...");
        ESP.deepSleep(0);
      }
      default: continue;
    }
    // break jumps here
    WiFi.begin(SSID, PASSWORD);
    Serial.println("Connecting");
  } while (true);
}
//