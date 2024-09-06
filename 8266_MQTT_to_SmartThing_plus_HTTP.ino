/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp8266-nodemcu-plot-readings-charts-multiple/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "ThingSpeak.h"

float sensors_0_t_value;
float sensors_1_t_value;
float sensors_2_t_value;
float outdoor_t_value;
float indoor_t_value;
float mobsensors_1_t_value;
float mobsensors_2_t_value;
int sensors_0_humidity_value;
int sensors_1_humidity_value;
int sensors_2_humidity_value;
int sensors_0_battery_stat;
int sensors_1_battery_stat;
int sensors_2_battery_stat;
float mobsensors_1_pressure_value;
float mobsensors_1_voltage_value;
float mobsensors_2_pressure_value;
float mobsensors_2_voltage_value;
float buderus_t_heating_value;
int buderus_flame_status;
float buderus_t_heating_setpoint;
float buderus_t_target;
int buderus_modulation_level;
int buderus_heating_status;
char topic;
const char* buderus_flame_status_txt;
const char* buderus_heating_status_txt;
const char* sensors_0_battery_stat_txt;
const char* sensors_1_battery_stat_txt;
const char* sensors_2_battery_stat_txt;
const char* mobsensors_1_voltage_value_txt;
const char* mobsensors_2_voltage_value_txt;
int channel_value;

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "PASSWORDWIFI";

//We will use static ip
IPAddress ip(192, 168, 1, 70 );// pick your own suitable static IP address
IPAddress gateway(192, 168, 1, 1); // may be different for your network
IPAddress subnet(255, 255, 255, 0); // may be different for your network (but this one is pretty standard)
IPAddress dns(192, 168, 1, 1);



#define SECRET_CH_ID1 0000000			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY1 "XYZ"   // replace XYZ with your channel write API Key
unsigned long myChannelNumber1 = SECRET_CH_ID1;
const char * myWriteAPIKey1 = SECRET_WRITE_APIKEY1;

#define SECRET_CH_ID2 0000000			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY2 "XYZ"   // replace XYZ with your channel write API Key
unsigned long myChannelNumber2 = SECRET_CH_ID2;
const char * myWriteAPIKey2 = SECRET_WRITE_APIKEY2;

#define SECRET_CH_ID3 0000000			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY3 "XYZ"   // replace XYZ with your channel write API Key
unsigned long myChannelNumber3 = SECRET_CH_ID3;
const char * myWriteAPIKey3 = SECRET_WRITE_APIKEY3;

#define SECRET_CH_ID4 0000000			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY4 "XYZ"   // replace XYZ with your channel write API Key
unsigned long myChannelNumber4 = SECRET_CH_ID4;
const char * myWriteAPIKey4 = SECRET_WRITE_APIKEY4;

//String myStatus = "";

// MQTT Broker settings
const char *mqtt_broker = "192.168.1.99";  // EMQX broker endpoint
const char *mqtt_topic = "#";     // MQTT topic
const char *mqtt_username = "admin";  // MQTT username for authentication
const char *mqtt_password = "12345678";  // MQTT password for authentication
const int mqtt_port = 1883;  // MQTT port (TCP)

WiFiClient espClient;
PubSubClient mqtt_client(espClient);


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Json Variable to hold sensor readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;


// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}


// Initialize WiFi
void initWiFi() {
  WiFi.config(ip, dns, gateway, subnet); 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}


void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;
    //String message = (char*)data;
    // Check if the message is "getReadings"
    //if (strcmp((char*)data, "getReadings") == 0) {
      //if it is, send current sensor readings
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    //}
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  
  // Start server
  server.begin();

  //Start MQTT
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();
  Serial.println("++++++++++");

}

 //i2 code

void connectToMQTTBroker() {
    //WiFiClient espClient;
    
    while (!mqtt_client.connected()) {
        String client_id = "MQTT-to-ThingSpeak";
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            // Publish message upon successful connection
            //mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}
    
void mqttCallback(char* topic, byte* payload, unsigned int length) 
  {
    //Serial.print("Message received on topic: ");
    Serial.println(topic);
    //Serial.print("Message:");
    //for (int h=0;h<length;h++) 
    //{
        //Serial.print((char) payload[h]);
    //  }
    //Serial.println();
    //Serial.println("-----------------------");
    //Serial.println("——————");
    
    
     DynamicJsonBuffer jsonBuffer(768);
     JsonObject& root = jsonBuffer.parseObject(payload);
     JsonObject& temperatures = root["temperatures"];
     JsonObject& parameters = root["parameters"];
     JsonObject& sensors = root["sensors"];
     JsonObject& states = root["states"];
     JsonObject& heating = root["heating"];
    
     
     if (const char* temperature = root["temperature"]) {                       // check that temperatur coming
      //Serial.println("сигнал содержит информацию о температуре");
      //const char* channel_value = root["channel"]; 
           
      if (!strncmp(topic, "rf434/recv/nexus/", 17))  {     //topic check first 17 chars                    
      //Serial.println("Топик NEXUS");

      channel_value = root["channel"];                     // Channel No check          
      if (channel_value <3 && channel_value >=0 ) 
        {
        //Serial.print("Канал №"); 
        //Serial.println(channel_value);
        
         switch (channel_value) {                                //No channel check and set value
         case 0:
         { sensors_0_t_value = root["temperature"];
          sensors_0_humidity_value = root["humidity"];
          sensors_0_battery_stat = root["battery"];
          if (root["battery"] = "1") {(sensors_0_battery_stat_txt = "Батарея в норме");} else {(buderus_flame_status_txt = "Замените батарею!");}  // Переводим в удобочитаемый текст для сайта
          }
         break;
          case 1:
          { sensors_1_t_value = root["temperature"];
           sensors_1_humidity_value = root["humidity"];
           sensors_1_battery_stat = root["battery"];
           if (root["battery"] = "1") {(sensors_1_battery_stat_txt = "Батарея в норме");} else {(buderus_flame_status_txt = "Замените батарею!");}  // Переводим в удобочитаемый текст для сайта
          }
          break;
          case 2:
            { sensors_2_t_value = root["temperature"];
             sensors_2_humidity_value = root["humidity"];
             sensors_2_battery_stat = root["battery"];
             if (root["battery"] = "1") {(sensors_2_battery_stat_txt = "Батарея в норме");} else {(buderus_flame_status_txt = "Замените батарею!");}  // Переводим в удобочитаемый текст для сайта 
            }
          break;
        }
       }
      }
     }

     if (const char* temperature = root["temperature"]) {                       // check that temperature coming
      //Serial.println("сигнал содержит информацию о температуре");
           
         if (!strcmp(topic, "sensor1/external"))  {     //topic check for MobSensor1                    
         //Serial.println("Топик SENSOR1");
         mobsensors_1_t_value = root["temperature"];
         mobsensors_1_pressure_value = atof(root["pressure"]) * 0.75006375541921;
         mobsensors_1_voltage_value = atof(root["voltage"]);
         if (mobsensors_1_voltage_value > 1.9) {(mobsensors_1_voltage_value_txt = "Батарея в норме");} else {(mobsensors_1_voltage_value_txt = "Замените батарею!");}  // Переводим в удобочитаемый текст для сайта
        }
         
          if (!strcmp(topic, "sensor2/external"))  {     //topic check for MobSensor2                    
           //Serial.println("Топик SENSOR2");
           mobsensors_2_t_value = root["temperature"];
           mobsensors_2_pressure_value = atof(root["pressure"]) * 0.75006375541921;
           mobsensors_2_voltage_value = atof(root["voltage"]);
           if (mobsensors_2_voltage_value > 1.9) {(mobsensors_2_voltage_value_txt = "Батарея в норме");} else {(mobsensors_2_voltage_value_txt = "Замените батарею!");}  // Переводим в удобочитаемый текст для сайта
          }
      }
     // Buderus_OT data extraction
     if (const char* outdoor = temperatures["outdoor"] ) outdoor_t_value = temperatures["outdoor"];
     if (const char* indoor = temperatures["indoor"]) indoor_t_value = temperatures["indoor"];
     if (const char* heating = temperatures["heating"]) buderus_t_heating_value = temperatures["heating"];
     if (const char* heatingSetpoint = parameters["heatingSetpoint"]) buderus_t_heating_setpoint = parameters["heatingSetpoint"];
     if (const char* modulation = sensors["modulation"]) buderus_modulation_level = sensors["modulation"];
     if (const char* flame = states["flame"]) buderus_flame_status = states["flame"];
     if (states["flame"] = "1") {(buderus_flame_status_txt = "Вкл");} else {(buderus_flame_status_txt = "Выкл");}  // Переводим в удобочитаемый текст для сайта
     if (const char* heating = states["heating"]) buderus_heating_status = states["heating"];
     if (states["heating"] = "1") {(buderus_heating_status_txt = "Вкл");} else {(buderus_heating_status_txt = "Выкл");}  // Переводим в удобочитаемый текст для сайта
     if (const char* target = heating["target"]) buderus_t_target = heating["target"];
           
    //Serial.println(sensors_0_t_value);
    //Serial.println(sensors_1_t_value);
    //Serial.println(sensors_2_t_value);
    //Serial.println(outdoor_t_value); 
    //Serial.println(indoor_t_value);
    //Serial.println(mobsensors_1_t_value);
    //Serial.println(mobsensors_2_t_value);
    //Serial.println(temperature); 
    //Serial.println(sensors_1_value);
    //Serial.println(topic);
    //Serial.println(outdoor);
    //Serial.println(indoor);
    //Serial.println(pressure);
    //Serial.println(buderus_flame_status);
      
  }



// Get Sensor Readings and return JSON object for local
 String getSensorReadings(){
 
   readings["indoor_t_value"] = String(indoor_t_value);
   readings["sensors_0_t_value"] = String(sensors_0_t_value);
   readings["sensors_1_t_value"] = String(sensors_1_t_value);
   readings["sensors_2_t_value"] = String(sensors_2_t_value);
   readings["mobsensors_2_t_value"] = String(mobsensors_2_t_value);
   readings["mobsensors_1_t_value"] = String(mobsensors_1_t_value);
   readings["outdoor_t_value"] = String(outdoor_t_value);
   readings["mobsensors_2_pressure_value"] = String(mobsensors_2_pressure_value);
   readings["mobsensors_2_voltage_value"] = String(mobsensors_2_voltage_value);
   readings["mobsensors_2_voltage_value_txt"] = String(mobsensors_2_voltage_value_txt);
   readings["sensors_0_battery_stat"] = String(sensors_0_battery_stat_txt);
   readings["sensors_1_battery_stat"] = String(sensors_1_battery_stat_txt);
   readings["sensors_2_battery_stat"] = String(sensors_2_battery_stat_txt);
   readings["sensors_0_humidity_value"] = String(sensors_0_humidity_value);
   readings["sensors_1_humidity_value"] = String(sensors_1_humidity_value);
   readings["sensors_2_humidity_value"] = String(sensors_2_humidity_value);
   readings["buderus_t_heating_value"] = String(buderus_t_heating_value);
   readings["buderus_t_heating_setpoint"] = String(buderus_t_heating_setpoint);
   readings["buderus_t_target"] = String(buderus_t_target);
   readings["buderus_modulation_level"] = String(buderus_modulation_level);
   readings["buderus_flame_status"] = String(buderus_flame_status_txt);
   readings["buderus_heating_status"] = String(buderus_heating_status_txt);
      
   //Preparing for data sending
   
    WiFiClient TSClient;
    ThingSpeak.begin(TSClient);  // Initialize ThingSpeak

   // Data reading ALL_TEMPERATURES for ThingSpeak

   ThingSpeak.setField(1, sensors_0_t_value);
   ThingSpeak.setField(2, sensors_1_t_value);
   ThingSpeak.setField(3, sensors_2_t_value);
   ThingSpeak.setField(4, outdoor_t_value);
   ThingSpeak.setField(5, indoor_t_value);
   ThingSpeak.setField(6, mobsensors_1_t_value);
   ThingSpeak.setField(7, mobsensors_2_t_value);
   
   // write to the ThingSpeak channel ALL_TEMPERATURES
   
   int x = ThingSpeak.writeFields(myChannelNumber1, myWriteAPIKey1);
   if(x == 200){
    Serial.println("Channel ALL_TEMPERATURES updated successfuly!");
   }
   else{
    Serial.println("Problem updating channel ALL_TEMPERATURES. HTTP error code " + String(x));
   }
   
   //delay(5000);
   
   // Data reading HUMIDITY_BATTERY_NEXUS for ThingSpeak

   ThingSpeak.setField(1, sensors_0_humidity_value);
   ThingSpeak.setField(2, sensors_1_humidity_value);
   ThingSpeak.setField(3, sensors_2_humidity_value);
   ThingSpeak.setField(4, sensors_0_battery_stat);
   ThingSpeak.setField(5, sensors_1_battery_stat);
   ThingSpeak.setField(6, sensors_2_battery_stat);

    // write to the ThingSpeak channel HUMIDITY_BATTERY_NEXUS
   
   int xx = ThingSpeak.writeFields(myChannelNumber2, myWriteAPIKey2);
   if(xx == 200){
    Serial.println("Channel MOBILE_SENSORS updated successfuly!");
   }
   else{
    Serial.println("Problem updating channel MOBILE_SENSORS. HTTP error code " + String(xx));
   }
    
    //delay(5000);

    // Data reading MOBILE_SENSORS for ThingSpeak

   ThingSpeak.setField(1, mobsensors_1_t_value);
   ThingSpeak.setField(2, mobsensors_1_pressure_value);
   ThingSpeak.setField(3, mobsensors_1_voltage_value);
   ThingSpeak.setField(4, mobsensors_2_t_value);
   ThingSpeak.setField(5, mobsensors_2_pressure_value);
   ThingSpeak.setField(6, mobsensors_2_voltage_value);

    // write to the ThingSpeak channel MOBILE_SENSORS
   
   int xxx = ThingSpeak.writeFields(myChannelNumber3, myWriteAPIKey3);
   if(xxx == 200){
    Serial.println("Channel HUMIDITY_BATTERY_NEXUS updated successfuly!");
   }
   else{
    Serial.println("Problem updating channel HUMIDITY_BATTERY_NEXUS. HTTP error code " + String(xxx));
   }

   //  delay(5000);

  // Data reading BUDERUS_OT for ThingSpeak

   ThingSpeak.setField(1, indoor_t_value);
   ThingSpeak.setField(2, outdoor_t_value);
   ThingSpeak.setField(3, buderus_t_heating_value);
   ThingSpeak.setField(4, buderus_flame_status);
   ThingSpeak.setField(5, buderus_t_heating_setpoint);
   ThingSpeak.setField(6, buderus_t_target);
   ThingSpeak.setField(7, buderus_modulation_level);
   ThingSpeak.setField(8, buderus_heating_status);

    // write to the ThingSpeak channel BUDERUS_OT
   
   int xxxx = ThingSpeak.writeFields(myChannelNumber4, myWriteAPIKey4);
   if(xxxx == 200){
    Serial.println("Channel BUDERUS_OT updated successfuly!");
   }
   else{
    Serial.println("Problem updating channel BUDERUS_OT. HTTP error code " + String(xxxx));
   }

   //delay(15000); 

  
  //Send data to local HTTP server
  
  String jsonString = JSON.stringify(readings);
  return jsonString;
  
}

void loop() {
 
 
 if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    Serial.print("SensorsReadingsDataForSite: ");
    Serial.println(sensorReadings);
    notifyClients(sensorReadings);

  lastTime = millis();

  }

  ws.cleanupClients();


  if (!mqtt_client.connected()) {
        connectToMQTTBroker();
    }
  mqtt_client.loop();
  
}
