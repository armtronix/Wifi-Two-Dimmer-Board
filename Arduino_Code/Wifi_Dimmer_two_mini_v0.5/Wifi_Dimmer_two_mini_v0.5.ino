/*
  Code for Wifi two triac 2 amps Big board
  This code is for ESP8266
  Firmware Version: 0.5
  Hardware Version: 0.1
  use Board as Node mcu 1.0

  Code Edited By :Naren N Nayak
  Date: 07/12/2017
  Last Edited By:Karthik B
  Date: 14/3/19

    While a WiFi config is not set or can't connect:
      http://server_ip will give a config page with
    While a WiFi config is set:
    http://server_ip/gpio?state_sw=0   -->> Turn off dimmer one
    http://server_ip/gpio?state_sw=1   -->> Turn on dimmer one
    http://server_ip/gpio?state_led=0    -->> Turn off dimmer two
    http://server_ip/gpio?state_led=1    -->> Turn on dimmer two
    http://server_ip/gpio?state_dimmer={dim percentage 0-99} -->> dim dimmer one
          example: http://192.168.1.52/gpio?state_dimmer=30  -->> this will dim the dimmer one to 30%, here 192.168.1.52 is device ipaddress
    http://server_ip/gpio?state_dimmer_2={dim percentage 0-99} -->> Simillarly dim dimmer two
    http://server_ip/cleareeprom -> Will reset the WiFi setting and rest to configure mode as AP
    server_ip is the IP address of the ESP8266 module, will be
    printed to Serial when the module is connected. (most likly it will be 192.168.4.1)
   To force AP config mode, press button 20 Secs!

   mqtt commands:

  mosquitto_pub -h brokerip -t DeviceSubscribetopic -m R13_ON   -->> Turn on dimmer one
  mosquitto_pub -h brokerip -t DeviceSubscribetopic -m R13_OFF    -->> Turn off dimmer one
  mosquitto_pub -h brokerip -t DeviceSubscribetopic -m R14_ON   -->> Turn on dimmer two
  mosquitto_pub -h brokerip -t DeviceSubscribetopic -m R14_OFF    -->> Turn off dimmer two
  mosquitto_pub -h brokerip -t DeviceSubscribetopic -m Dimmer1:{dim percentage 0-99}  -->> dim dimmer one
  mosquitto_pub -h brokerip -t DeviceSubscribetopic -m Dimmer2:{dim percentage 0-99}  -->> dim dimmer two

    For several snippets used, the credit goes to:
    - https://github.com/esp8266
    - https://github.com/chriscook8/esp-arduino-apboot
    - https://github.com/knolleary/pubsubclient
    - https://github.com/vicatcu/pubsubclient <- Currently this needs to be used instead of the origin
    - https://gist.github.com/igrr/7f7e7973366fc01d6393
    - http://www.esp8266.com/viewforum.php?f=25
    - http://www.esp8266.com/viewtopic.php?f=29&t=2745
    - And the whole Arduino and ESP8266 comunity
*/

#define DEBUG
//#define WEBOTA
//debug added for information, change this according your needs

#ifdef DEBUG
#define Debug(x)    Serial.print(x)
#define Debugln(x)  Serial.println(x)
#define Debugf(...) Serial.printf(__VA_ARGS__)
#define Debugflush  Serial.flush
#else
#define Debug(x)    {}
#define Debugln(x)  {}
#define Debugf(...) {}
#define Debugflush  {}
#endif

#include <Espalexa.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
//#include <EEPROM.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "FS.h"

extern "C" {
#include "user_interface.h" //Needed for the reset command
}

//callback functions
void firstLightChanged(uint8_t brightness);

//***** Settings declare *********************************************************************************************************
String hostName = "Armtronix"; //The MQTT ID -> MAC adress will be added to make it kind of unique
int iotMode = 0; //IOT mode: 0 = Web control, 1 = MQTT (No const since it can change during runtime)
//select GPIO's
#define INPIN 0  // input pin (push button)
#define RESTARTDELAY 3 //minimal time in sec for button press to reset
#define HUMANPRESSDELAY 50 // the delay in ms untill the press should be handled as a normal push by human. Button debounce. !!! Needs to be less than RESTARTDELAY & RESETDELAY!!!
#define RESETDELAY 20 //Minimal time in sec for button press to reset all settings and boot to config mode
#define RESET_PIN 16
//#define Dimmer_State

// Check values every 2 seconds
#define UPDATE_TIME                     2000
#define STATUS_UPDATE_TIME              1000

//##### Object instances #####
int https_port_no = 80; //added on 14/02/2019
MDNSResponder mdns;
ESP8266WebServer server(https_port_no);
WebSocketsServer webSocket = WebSocketsServer(81);
WiFiClient wifiClient;
PubSubClient mqttClient;
Ticker btn_timer;
Ticker otaTickLoop;
Espalexa espalexa;

//##### Flags ##### They are needed because the loop needs to continue and cant wait for long tasks!
int rstNeed = 0; // Restart needed to apply new settings
int toPub = 0; // determine if state should be published.
int configToClear = 0; // determine if config should be cleared.
int otaFlag = 0;
boolean inApMode = 0;
//##### Global vars #####
int webtypeGlob;
int otaCount = 300; //imeout in sec for OTA mode
int current; //Current state of the button

int freqStep = 375;//75*5 as prescalar is 16 for 80MHZ
volatile int i = 0;
volatile int dimming = 0;
volatile int dimming_2 = 0;
//volatile boolean zero_cross=0;

unsigned long count = 0; //Button press time counter
String st; //WiFi Stations HTML list
String state; //State of light
char buf[40]; //For MQTT data recieve
char* host; //The DNS hostname
//To be read from Config file
String esid = "";
String epass = "";
String pubTopic;
String subTopic;
String mqttServer = "";
String mqtt_user = "";     //added on 28/07/2018
String mqtt_passwd = "";   //added on 28/07/2018
String mqtt_will_msg = " disconnected"; //added on 28/07/2018
String mqtt_port = "";  //added on 14/02/2019

#ifdef Dimmer_State
String DimmerState1 = "0";
String DimmerState2 = "0";
#endif  //Dimmer_State

const char* otaServerIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
boolean wifiConnected = false;
String javaScript, XML;

/*Alexa event names */
String firstName;
String secondName;


char string[32];
char byteRead;
String serialReceived = "";
String serialReceived_buf = "";

int dimmer_state;
int dimmer_state2;
int old_dimmer_state;
int old_dimmer_state2;
int mqtt_dimmer_state;
int mqtt_dimmer_state2;

volatile boolean mqtt_dimpub = false;
volatile boolean mqtt_dim2pub = false;





//-------------- void's -------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(10);
  WiFi.printDiag(Serial);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  btn_timer.attach(0.05, btn_handle);
  Debugln("DEBUG: Entering loadConfig()");
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
  }

  uint8_t mac[6];
  WiFi.macAddress(mac);
  hostName += "-";
  hostName += macToStr(mac);
  String hostTemp = hostName;
  hostTemp.replace(":", "-");
  host = (char*) hostTemp.c_str();
  loadConfig();
  //loadConfigOld();
  Debugln("DEBUG: loadConfig() passed");

  // Connect to WiFi network
  Debugln("DEBUG: Entering initWiFi()");
  initWiFi();
  Debugln("DEBUG: initWiFi() passed");
  Debug("iotMode:");
  Debugln(iotMode);
  Debug("webtypeGlob:");
  Debugln(webtypeGlob);
  Debug("otaFlag:");
  Debugln(otaFlag);
  Debugln("DEBUG: Starting the main loop");

  if (wifiConnected) {
    server.on("/", HTTP_GET, []() {
      server.send(200, "text/plain", "This is an example index page your server may send.");
    });
    server.on("/test", HTTP_GET, []() {
      server.send(200, "text/plain", "This is a second subpage you may have.");
    });
    server.onNotFound([]() {
      if (!espalexa.handleAlexaApiCall(server.uri(), server.arg(0))) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        server.send(404, "text/plain", "Not found");
      }
    });
  }
  // Define your devices here.
  espalexa.addDevice((char*)firstName.c_str(), firstLightChanged, 0); //simplest definition, default state off
  espalexa.addDevice((char*)secondName.c_str(), secondLightChanged, 0); //simplest definition, default state off

  espalexa.begin(&server); //give espalexa a pointer to your server object so it can use your server instead of creating its own


#ifdef Dimmer_State
  Serial.print("Dimmer1:");
  Serial.println(DimmerState1);
  Serial.print("Dimmer2:");
  Serial.println(DimmerState2);
#endif  //Dimmer_State

}

void InitInterrupt(timercallback handler, int Step )
{
  timer1_disable();
  timer1_isr_init();
  timer1_attachInterrupt(handler);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  timer1_write(Step);//max 8388607 //75*5
}



void btn_handle()
{
  if (!digitalRead(INPIN)) {
    ++count; // one count is 50ms
  } else {
    if (count > 1 && count < HUMANPRESSDELAY / 5) { //push between 50 ms and 1 sec
      Serial.print("button pressed ");
      Serial.print(count * 0.05);
      Serial.println(" Sec.");
      if (iotMode == 1 && mqttClient.connected()) {
        toPub = 1;
        Debugln("DEBUG: toPub set to 1");
      }
    } else if (count > (RESTARTDELAY / 0.05) && count <= (RESETDELAY / 0.05)) { //pressed 3 secs (60*0.05s)
      Serial.print("button pressed ");
      Serial.print(count * 0.05);
      Serial.println(" Sec. Restarting!");
      setOtaFlag(!otaFlag);
      ESP.reset();
    } else if (count > (RESETDELAY / 0.05)) { //pressed 20 secs
      Serial.print("button pressed ");
      Serial.print(count * 0.05);
      Serial.println(" Sec.");
      Serial.println(" Clear settings and resetting!");
      configToClear = 1;
    }
    count = 0; //reset since we are at high
  }
}



//-------------------------------- Main loop ---------------------------
void loop() {

  webSocket.loop();
  static unsigned long last = millis();


  if ((millis() - last) > STATUS_UPDATE_TIME)
  {
    Serial.println("status:");
    if (Serial.available())
    {
      size_t len = Serial.available();
      uint8_t sbuf[len];
      Serial.readBytes(sbuf, len);
      serialReceived = (char*)sbuf;
      if (serialReceived.substring(0, 2) == "D:")
      {
        //               mqttClient.publish((char*)pubTopic.c_str(),serialReceived.substring(2,4).c_str());
        serialReceived_buf = serialReceived;
        serialReceived = "";
        dimmer_state = serialReceived_buf.substring(2, 4).toInt();
        dimmer_state2 = serialReceived_buf.substring(5, 7).toInt(); //added on 15/02/19
        if (dimmer_state != old_dimmer_state)
        {
          old_dimmer_state = dimmer_state;
          mqtt_dimmer_state = dimmer_state;
          mqtt_dimpub = true;
          //Serial.println(dimmer_state);
        }
        if (dimmer_state2 != old_dimmer_state2)
        {
          old_dimmer_state2 = dimmer_state2;
          mqtt_dimmer_state2 = dimmer_state2;
          mqtt_dim2pub = true;
        }
        serialReceived_buf = "";
      }
    }
    last = millis();
  }


  if (mqttClient.connected())
  {
    if (mqtt_dimmer_state == 0 && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS0");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 0 && mqtt_dimmer_state <= 5 ) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS5");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 5 && mqtt_dimmer_state <= 10) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS10");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 10 && mqtt_dimmer_state <= 15) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS15");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 15 && mqtt_dimmer_state <= 20) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS20");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 20 && mqtt_dimmer_state <= 25) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS25");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 25 && mqtt_dimmer_state <= 30) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS30");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 30 && mqtt_dimmer_state <= 35) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS35");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 35 && mqtt_dimmer_state <= 40) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS40");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 40 && mqtt_dimmer_state <= 45) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS45");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 45 && mqtt_dimmer_state <= 50) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS50");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 50 && mqtt_dimmer_state <= 55) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS55");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 55 && mqtt_dimmer_state <= 60) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS60");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 60 && mqtt_dimmer_state <= 65) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS65");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 65 && mqtt_dimmer_state <= 70) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS70");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 70 && mqtt_dimmer_state <= 75) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS75");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 75 && mqtt_dimmer_state <= 80) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS80");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 80 && mqtt_dimmer_state <= 85) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS85");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 85 && mqtt_dimmer_state <= 90) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS90");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 90 && mqtt_dimmer_state <= 95) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS95");
      mqtt_handler();
      mqtt_dimpub = false;
    }
    else if ((mqtt_dimmer_state > 95 && mqtt_dimmer_state <= 100) && mqtt_dimpub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer1IS99");
      mqtt_handler();
      mqtt_dimpub = false;
    }

    //#########################################Dimmer2########################################
    if (mqtt_dimmer_state2 == 0 && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS0");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 0 && mqtt_dimmer_state2 <= 5 ) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS5");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 5 && mqtt_dimmer_state2 <= 10) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS10");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 10 && mqtt_dimmer_state2 <= 15) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS15");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 15 && mqtt_dimmer_state2 <= 20) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS20");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 20 && mqtt_dimmer_state2 <= 25) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS25");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 25 && mqtt_dimmer_state2 <= 30) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS30");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 30 && mqtt_dimmer_state2 <= 35) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS35");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 35 && mqtt_dimmer_state2 <= 40) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS40");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 40 && mqtt_dimmer_state2 <= 45) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS45");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 45 && mqtt_dimmer_state2 <= 50) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS50");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 50 && mqtt_dimmer_state2 <= 55) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS55");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 55 && mqtt_dimmer_state2 <= 60) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS60");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 60 && mqtt_dimmer_state2 <= 65) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS65");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 65 && mqtt_dimmer_state2 <= 70) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS70");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 70 && mqtt_dimmer_state2 <= 75) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS75");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 75 && mqtt_dimmer_state2 <= 80) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS80");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 80 && mqtt_dimmer_state2 <= 85) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS85");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 85 && mqtt_dimmer_state2 <= 90) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS90");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 90 && mqtt_dimmer_state2 <= 95) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS95");
      mqtt_handler();
      mqtt_dim2pub = false;
    }
    else if ((mqtt_dimmer_state2 > 95 && mqtt_dimmer_state2 <= 100) && mqtt_dim2pub == true)
    {
      mqttClient.publish((char*)pubTopic.c_str(), "Dimmer2IS99");
      mqtt_handler();
      mqtt_dim2pub = false;
    }

  }




  //Debugln("DEBUG: loop() begin");
  if (configToClear == 1) {
    //Debugln("DEBUG: loop() clear config flag set!");
    clearConfig() ? Serial.println("Config cleared!") : Serial.println("Config could not be cleared");
    delay(1000);
    ESP.reset();
  }
  //Debugln("DEBUG: config reset check passed");
  if (WiFi.status() == WL_CONNECTED && otaFlag) {
    if (otaCount <= 1) {
      Serial.println("OTA mode time out. Reset!");
      setOtaFlag(0);
      ESP.reset();
      delay(100);
    }
    server.handleClient();
    delay(1);
  } else if (WiFi.status() == WL_CONNECTED || webtypeGlob == 1) {
    //Debugln("DEBUG: loop() wifi connected & webServer ");
    if (iotMode == 0 || webtypeGlob == 1) {
      //Debugln("DEBUG: loop() Web mode requesthandling ");
      server.handleClient();
      delay(1);
      if (esid != "" && WiFi.status() != WL_CONNECTED) //wifi reconnect part
      {
        Scan_Wifi_Networks();
      }
    } else if (iotMode == 1 && webtypeGlob != 1 && otaFlag != 1) {
      //Debugln("DEBUG: loop() MQTT mode requesthandling ");
      if (!connectMQTT()) {
        delay(200);
      }
      if (mqttClient.connected()) {
        //Debugln("mqtt handler");
        mqtt_handler();
      } else {
        Debugln("mqtt Not connected!");
      }
    }
  } else {
    Debugln("DEBUG: loop - WiFi not connected");
    delay(1000);
    initWiFi(); //Try to connect again
  }
  //Debugln("DEBUG: loop() end");

  espalexa.loop();
  delay(1);
}





//our callback functions
void firstLightChanged(uint8_t brightness) {
  //Serial.print("Device 1 changed to ");
  //Serial.println(brightness);
  //do what you need to do here

  //EXAMPLE
  if (brightness == 255) {
    Serial.println("Dimmer1:99");
  }
  else if (brightness == 0) {
    Serial.println("Dimmer1:0");
  }
  else if (brightness == 1) {
    Serial.println("Dimmer1:0");
  }
  else {
    float mul = 0.388; //  99/255 for values between 0-99
    float bness = (brightness * mul);
    int ss = bness;
    Serial.print("Dimmer1:");
    Serial.println(ss);
  }
}

void secondLightChanged(uint8_t brightness) {

  //Serial.print("Device 2 changed to ");
  //Serial.println(brightness);

  //do what you need to do here

  //EXAMPLE
  if (brightness == 255) {
    Serial.println("Dimmer2:99");
  }
  else if (brightness == 0) {
    Serial.println("Dimmer2:0");
  }
  else if (brightness == 1) {
    Serial.println("Dimmer2:0");
  }
  else {
    float mul = 0.388; //  99/255 for values between 0-99
    float bness = (brightness * mul);
    int ss = bness;
    Serial.print("Dimmer2:");
    Serial.println(ss);
  }

}
