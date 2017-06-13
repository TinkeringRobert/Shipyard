/**
 * httpUpdate.ino
 *
 *  Created on: 27.11.2015
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <PubSubClient.h>

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#define PIN            2
#define NUMPIXELS      24
#define MAX_POWER      1024
#define FREQ_UPDATE    10
#define FREQ_RGB       1
#define MAX_ERRORS     4

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WiFiMulti WiFiMulti;

String fw_version = "2.4";
char st_ssid[32] = "Kappert";
char st_password[32] = "1234567890Trepak03";
char st_node[8] = "2000";
char ssid[32] = "";
char password[32] = "";
char node[8] = "";

const char* mqtt_server = "10.0.0.100";

bool wifi_connecting = false;
bool wifi_connected = false;
int wl_status = -1;
int wifi_errors = 0;

void setup() {
  pixels.begin(); // This initializes the NeoPixel library.
  for(int i=0;i<NUMPIXELS;i++){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,30,0)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(50);
  }
  Serial.begin(115200);
  //Serial.println("\n----------------------------------------");
  //saveCredentials();
  Serial.println("----------------------------------------");
  loadCredentials();
  Serial.println("----------------------------------------");
  Serial.print("Version " + fw_version + " Updating node : ");
  Serial.println(node);

  wifi_connecting = true;
  wifi_connected = false;
  WiFiMulti.addAP(ssid, password);
  Serial.println("Wifi connection starting");
}

int process_count = 0;

void loop() {
  WiFiMulti.run();
  client.loop();
  manageWifiConnection();  
  if(wifi_connected == true){
    Serial.print("Wifi connection up and running : ");
    Serial.println(process_count);
    manageMqttConnection();
  
    String nodeId((char*)node);
    if (process_count % FREQ_UPDATE == 0)
    {
      getFirmware( getHttpRequest("http://10.0.0.101:4000/firmware/" + nodeId), nodeId);
    }
    if (process_count % FREQ_RGB == 0)
    {
      //setRgbStrip( getHttpRequest("http://10.0.0.101:4000/node/rgb/" + nodeId) );
    }
    
    //Serial.println("GET END----------------------------------------------------");

    //----------------------------------------------------------------------
    if( wifi_errors >= MAX_ERRORS ){
      Serial.print("Resetting wifi due to error count reached :: ");
      Serial.println(wifi_errors);
      wifi_connected = false;
    }
  }

//  if(process_count == 50){
//    process_count = 0;
//    wifi_connected = false;
//    Serial.println("Reset connection testing");
//  }
  process_count++;
  delay(500);
}

String getHttpRequest(String httpUrl)
{        
  HTTPClient http;
  String payload = "";
  Serial.println("---------------------------------------------------------");
  Serial.print("GET URL : ");
  Serial.println(httpUrl);
  
  http.begin(httpUrl); //HTTP

  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0)
  {
      // HTTP header has been send and Server response header has been handled
      //Serial.print("GET [HTTP] GET... code: ");
      //Serial.print(httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK)
      {
          wifi_errors = 0;
          payload = http.getString();
//          Serial.print(" Recv: ");
//          Serial.println(payload);
      } else {
        //Serial.println("");
      }
  } else {
    wifi_errors++;
    Serial.print("GET [HTTP] GET... failed, err_cnt: ");
    Serial.print(wifi_errors);
    Serial.print(" error: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  http.end();
  return payload;
}

// Control the RgbStrip
void getFirmware(String payload, String nodeId)
{
  bool download_fw = false;
  if( payload != "")
  {
    if( payload != fw_version )
    {
//      Serial.println("Firmware versions do not match");
//      Serial.print("fw_version :");
//      Serial.println(fw_version);
//      Serial.print("payload    :");
//      Serial.println(payload);
      download_fw = true;
    }
  }
  
  if(download_fw == true)
  {  
    Serial.print("Request update from http://10.0.0.101:4000/download/" + nodeId);
    for(int i=0;i<NUMPIXELS;i++){
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(NUMPIXELS,NUMPIXELS,NUMPIXELS)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      delay(50);
    }
    t_httpUpdate_return ret = ESPhttpUpdate.update("http://10.0.0.101:4000/download/" + nodeId + "/" + fw_version + "/" + payload);
    Serial.println(ret);
    switch(ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES\n");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK\n");
            break;
    }
  }
}

// Control the RgbStrip
//void setRgbStrip(String payload)
//{
//  DynamicJsonBuffer jsonBuffer;
//  if( payload != "" )
//  {
//    JsonObject& root = jsonBuffer.parseObject(payload);
//    int ired = MAX_POWER;
//    int dred = 0;
//    int red = root[String("r")];
//    int green = root[String("g")];
//    int blue = root[String("b")];
////    Serial.print("red : ");
////    Serial.print(red);
////    Serial.print(" green : ");
////    Serial.print(green);
////    Serial.print(" blue : ");
////    Serial.print(blue);
//    
//    int each_led = MAX_POWER / NUMPIXELS;
//    
////    Serial.print(" each_led : ");
////    Serial.print(each_led);
////    Serial.println("");
//    for(int i=0;i<NUMPIXELS;i++){
//      
////      Serial.print(i);
////      Serial.print(": red : ");
////      Serial.print(red);
////      Serial.print(" green : ");
////      Serial.print(green);
////      Serial.print(" blue : ");
////      Serial.println(blue);
////      Serial.print(i);
////      Serial.print(": red : ");
//      if((red + each_led) < ired){
//        //Serial.print("s");
//        dred = 0;
//      } else if(red < ired) {
//        //Serial.print("a");
//        dred = ired - red;
//      } else {
//        //Serial.print("b");
//        dred = 255;
//      }
////      Serial.print(dred);
////      Serial.print(" green : ");
////      Serial.print((green>each_led)?255:(green*(255/each_led)));
////      Serial.print(" blue : ");
////      Serial.println((blue>each_led)?255:(blue*(255/each_led)));
//      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
//      pixels.setPixelColor(i, 
//                           pixels.Color(
//                            dred,
//                            (green>each_led)?255:(green*(255/each_led)),
//                            (blue>each_led)?255:(blue*(255/each_led))));
//      green = green - each_led;
//      blue = blue - each_led;
//      ired = ired - each_led;
//      if( ired < 0 ) { ired = 0; }
//      if( green < 0 ) { green = 0; }
//      if( blue < 0 ) { blue = 0; }
//    }
//    pixels.show(); // This sends the updated pixel color to the hardware.
//  }
//}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] Length = ");
  Serial.println(length);
  for(int i=0;i<NUMPIXELS;i++){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,0,0)); // Moderately bright green color.
  }
  for (int i = 0; i < (length/4); i++) {
//    Serial.print(payload[(i*3) + 0]);
//    Serial.print(".");
//    Serial.print(payload[(i*3) + 1]);
//    Serial.print(".");
//    Serial.print(payload[(i*3) + 2]);
//    Serial.println(".");
    pixels.setPixelColor(payload[(i*4) + 0], 
                         pixels.Color(
                          payload[(i*4) + 1],
                          payload[(i*4) + 2],
                          payload[(i*4) + 3])); // Moderately bright green color. 
  }
  pixels.show();
}

void manageWifiConnection() { 
  int wl_status = WiFi.status();
  //Check if there is no connection
  if (wl_status != WL_CONNECTED) {
    if (wifi_connecting == false) {
      wifi_connecting = true;
      wifi_connected = false;
      Serial.println("Wifi connection restarting");
    } else {
      Serial.print("Connecting :");
      Serial.println(process_count);
    }
  } 
  else if (wl_status == WL_CONNECTED) {
    if (wifi_connecting == false && wifi_connected == false) {
      wifi_connecting = false;
      process_count = 0;
      WiFi.disconnect();
      Serial.println("Wifi connection terminated");
    }
    else if (wifi_connecting == true) {
      wifi_errors = 0;
      wifi_connected = true;
      wifi_connecting = false;
      Serial.println("Wifi connection started");
      client.setServer(mqtt_server, 1883);
      client.setCallback(callback);
      //client.subscribe("testTopic");
      while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client")) {
          Serial.println("connected");
          client.subscribe("testTopic");
        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 1 seconds");
          // Wait 5 seconds before retrying
          delay(1000);
        }
      }
    }
    else if (wifi_connected == false) {
      Serial.println("Wifi connection booting");
    }
  }
  
  //Serial.print("process_count:");
  //Serial.println(process_count);
}

void manageMqttConnection() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("testTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}
/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  Serial.println("loading Credentials");
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  EEPROM.get(0+sizeof(ssid)+sizeof(password), node);
  char ok[2+1];
  EEPROM.get(0+sizeof(ssid)+sizeof(password)+sizeof(node), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.print("ssid :: ");
  Serial.println(ssid);
  Serial.print("pass :: ");
  Serial.println(strlen(password)>0?"********":"<no password>");
  Serial.print("node :: ");
  Serial.println(node);
  Serial.println("loaded Credentials");
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
//  Serial.println("saving Credentials");
  EEPROM.begin(512);
  EEPROM.put(0, st_ssid);
  EEPROM.put(0+sizeof(st_ssid), st_password);
  EEPROM.put(0+sizeof(st_ssid)+sizeof(st_password), st_node);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(st_ssid)+sizeof(st_password)+sizeof(st_node), ok);
  EEPROM.commit();
  EEPROM.end();
//  Serial.println("saved Credentials");
}
