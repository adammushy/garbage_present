#include <TinyGPS++.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include <HardwareSerial.h>
// #include <SoftwareSerial.h>

// #include <ESP8266WiFi.h>

// #include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
// #include <UUID.h> 
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>


#include <addons/TokenHelper.h>


/* 2. Define the API Key */
#define API_KEY "AIzaSyB7s-Lzzk6IEV6XC3Y6Ojte4hTMGV7JpcM"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "smart-garbage-collection-8fb03"

#define USER_EMAIL "esp8266@gmail.com"
#define USER_PASSWORD "123456"

TinyGPSPlus gps;  // The TinyGPS++ object

SoftwareSerial Serial2(2, 15); // The serial connection to the GPS device

const char* ssid = "....";
const char* password = "20002000";


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


float Latitude, Longitude;
// double Longitude = 39.209927, Latitude = -6.722574;
int year, month, date, hour, minute, second;
String date_str, time_str, lat_str, lng_str;
int pm;

// Initialize the LCD with the I2C address 0x27 and 20 columns and 4 rows
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Replace with your server name
const char* serverName = "http://157.245.109.105:6000/trash-management/trashbin-view";
// const char* serverName = "http://192.168.251.68:8000/trash-management/trashbin-view";


long duration;
float distanceCm;
const int trigPin = 12;
const int echoPin = 14;
float SOUND_SPEED = 0.0343;
int value;
String state;
long dataMillis = 0;
bool taskcomplete = false;
String name= "DIT 1";
String id ="ILA-KKOO-DIT-01";
WiFiClient wificlient;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);

  lcd.init(); // Initialize the LCD
  lcd.backlight();
 // Turn on the backlight
  lcd.setCursor(0, 0);
  lcd.print("   WELCOME... ");
  // delay(500);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
     Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
    /* Assign the api key (required) */
    config.api_key = API_KEY;
    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    Serial.println("authenticated ");
    lcd.setCursor(0, 1);
    lcd.print("Authentication ");

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

  delay(500);

  lcd.clear();


  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  HTTPClient http;
  // http.begin(serverName);
    http.begin(wificlient, serverName);
  
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
    Serial.println("Encode ");
    
    Serial.println(gps.encode(Serial2.read()));
  }

  if (gps.location.isUpdated()) {
   
  }

    Latitude = gps.location.lat();
    Longitude = gps.location.lng();
    Serial.print("Latitude= ");
    Serial.print(Latitude, 6);
    Serial.print(" Longitude= ");
    Serial.println(Longitude, 6);
    Serial.print("Characters processed: ");
    Serial.println(gps.charsProcessed());
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Lat: ");
    lcd.print(Latitude, 6);
    lcd.setCursor(0, 1);
    lcd.print("Lng: ");
    lcd.print(Longitude, 6);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;

 
// Calculate percentage based on a container height of 100cm
  int percentage = map(distanceCm, 0, 17, 100, 0);
  if (percentage < 0) {
    percentage = 0;
  } else if (percentage > 100) {
    percentage = 100;
  }

  // Determine status based on percentage
  String status;
  if (distanceCm > 20) {
    status = "dustbin errors";
  } else if (percentage == 100) {
    status = "full";
  } else if (percentage >= 75) {
    status = "almost full";
  } else if (percentage >= 50) {
    status = "medium";
  } else if (percentage >= 25) {
    status = "low";
  } else if (percentage == 0) {
    status = "empty";
  } 
  else {
    status = "very low";
  }

  Serial.print("fill level: ");
  Serial.println(int(percentage));
  Serial.println("State of trash ");
  Serial.println(state);
    // lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print("fill level: ");
    lcd.print(int(percentage));
    lcd.setCursor(0, 3);
    lcd.print("state: ");
    lcd.print(status);

  http.addHeader("Content-Type", "application/json");
  DynamicJsonDocument doc(1024);
  // doc["id"] = "21231sdarewerfdsdfsf"; // Replace with your ID generation function
  doc["id"] = id; // Replace with your ID generation function

  doc["name"] = name;
  doc["lat"] = Latitude;
  doc["lon"] = Longitude;
  doc["percentage"] = percentage;
  doc["status"] = status;

  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(F("below are doc and json string"));
  Serial.println(jsonString);
  int httpResponseCode = http.POST(jsonString);
  Serial.println("\nHTTP response code: " + String(httpResponseCode));
  http.end();

  

  // Firestore operations (if needed)
  // Note: Make sure to include and configure the Firebase library properly
  if (Firebase.ready() && (millis() - dataMillis > 15000 || dataMillis == 0)) {
    dataMillis = millis();
    FirebaseJson content;
    String documentPath = "data/"+String(id);
    if (!taskcomplete) {
      taskcomplete = true;
      content.clear();
      content.set("fields/Latitude/doubleValue", Latitude);
      content.set("fields/Longitude/doubleValue", Longitude);
      content.set("fields/state/stringValue", status);
      content.set("fields/percentage/integerValue", percentage);
      content.set("fields/name/stringValue", name);
      Serial.print("Create a document... ");
      if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw()))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      else
        Serial.println(fbdo.errorReason());
    }
    content.clear();
    content.set("fields/Latitude/doubleValue", Latitude);
    content.set("fields/Longitude/doubleValue", Longitude);
    content.set("fields/state/stringValue", status);
    content.set("fields/percentage/integerValue", percentage);
    content.set("fields/name/stringValue", name);
    Serial.print("Update a Smart... ");
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "name,state,Latitude,Longitude,percentage" /* updateMask */))
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());
  }
delay(10000);
}
