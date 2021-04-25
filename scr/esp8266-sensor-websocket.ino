#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>

#include <ArduinoJson.h>
#include <FS.h>

#include <SoftwareSerial.h>
#include <Ticker.h>

#include <SimpleDHT.h>

#define led_pin 2       //D4  nodeMCU
//#define led_pin 1     //esp01

#define DHT11_pin 5     //D1  nodeMCU
//#define DHT11_pin 2;  //esp01 

Ticker timer;
Ticker weatherTimer;

bool get_data = true;

// Connecting to the Internet
//char * ssid = "TP-Link_41F8";
//char * password = "38517129";

char * ssid = "SKYMEDIA-49";
char * password = "99109265";

//static values
byte temperature_inside = 99;
byte humidity_inside = 99;

float temperature_outside;
float humidity_outside;

String APIKEY = "3e9b3259a28b1a2f6fc6d0e6d07a2b25";
String CityID = "2028462"; //Ulaanbaatar, MN

SimpleDHT11 dht11(DHT11_pin);    

// Running a web server
ESP8266WebServer server;

// Adding a websocket to the server
WebSocketsServer webSocket = WebSocketsServer(81);


void setup() {  
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);
  timer.attach(5, getData);
  
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature_inside, &humidity_inside, NULL)) != SimpleDHTErrSuccess);
    

  Serial.println("");
  WiFi.begin(ssid, password);
  Serial.print("connecting.");
  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(led_pin,!digitalRead(led_pin));
    delay(500);
  }

  Serial.println(""); 
  
  if (!MDNS.begin("automat-agaarjuulalt")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }  
  else {
    Serial.println("mDNS responder started at: automat-agaarjuulalt.local/");
  }
  
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  
  
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  server.on("/",homepage);  
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);  
  
  if(WiFi.status() == WL_CONNECTED)                     //WL_CONNECTED LED indicate
  {
    for(int n = 0; n < 15; n++)
    {
      digitalWrite(led_pin,!digitalRead(led_pin));
      delay(50);
    }
    digitalWrite(led_pin, HIGH);
  }
  
  Serial.println("");                                   // blank new line  
   
  weatherTimer.attach(3600, getWeatherData);
  getWeatherData();
}

void loop() {
  
  webSocket.loop();
  server.handleClient();
  MDNS.update();

  if (get_data) 
  {     
    digitalWrite(led_pin, LOW);
    
    int err = SimpleDHTErrSuccess;
    if ((err = dht11.read(&temperature_inside, &humidity_inside, NULL)) != SimpleDHTErrSuccess);    
    
    String json = "{\"temp_o\":";
    json += temperature_outside; 
    json += ",";
    json += "\"temp_i\":";
    json += temperature_inside;
    json += ",";
    json += "\"hum_o\":";
    json += humidity_outside;
    json += ",";
    json += "\"hum_i\":";
    json += humidity_inside;
    json += "}";
    
    webSocket.broadcastTXT(json.c_str(), json.length());
    get_data = false;

    digitalWrite(led_pin, HIGH);
  }
  
}

void getData() {
  get_data = true;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){  // Do something with the data from the client
  
  if(type == WStype_TEXT){    
    float dataRate = (float) atof((const char *) &payload[0]);
    
    timer.detach();
    timer.attach(dataRate, getData);                                                // dataRate * 60 to make min
    Serial.println(dataRate);
    Serial.println();
  }
}

void getWeatherData() //client function to send/receive GET request data.
{
    HTTPClient http;  //Object of class HTTPClient
    http.begin("http://api.openweathermap.org/data/2.5/weather?id=2028462&units=metric&APPID=3e9b3259a28b1a2f6fc6d0e6d07a2b25");

    int httpCode = http.GET();                                                                
    if (httpCode > 0) {
      // Parsing
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getString());

      float temperature = doc["main"]["temp"];
      float humidity = doc["main"]["humidity"];
      
      temperature_outside = temperature;
      humidity_outside = humidity;
    }
    http.end();   //Close connection
}

void homepage()
{
  File file = SPIFFS.open("/index.html","r");
  server.streamFile(file, "text/html");
}
