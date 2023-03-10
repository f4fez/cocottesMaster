#include <DNSServer.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include <ArduinoOTA.h>
#include <ESPNtpClient.h>

#include "ESPAsyncWebServer.h"
#include "net.h"
#include "SystemState.h"
#include "io.h"


extern struct SystemState state;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

bool apMode;
DNSServer dnsServer;

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 255, 0);

IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(8, 8, 4, 4);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

String lightState;
String doorState;

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);

  if(ip!=""){
    Serial.println("Set fixed IP.");
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());
    if (!WiFi.config(localIP, localGateway, subnet, dns1, dns2)){
        Serial.println("STA Failed to configure");
        return false;
    }
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  return true;
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  if(var == "LIGHT_STATE") {
    if(state.light) {
      return "ON";
    }
    else {
      return "OFF";
    }
  } else if(var == "DOOR_STATE") {
    if(state.doorMotor == DOOR_GO_UP) {
      return "Ouverture";
    }
    else if(state.doorMotor == DOOR_GO_DOWN) {
      return "Fermeture";
    }
    else {
      if (state.door == DOOR_OPEN) {
        return "Ouvert";
      }
      else if (state.door == DOOR_CLOSE) {
        return "Ferm&eacute;";
      }
      else if (state.door == DOOR_OPEN_MANUAL) {
        return "Ouvert (Manuel)";
      }
      else if (state.door == DOOR_CLOSE_MANUAL) {
        return "Ferm&eacute; (Manuel)";
      }
      else {
        return "Inconnu";
      }
    }
  } else if(var == "TEMP") {
    return String(state.temperature);
  } else if(var == "HUMIDITY") {
    return String(state.humidity);
  } else if(var == "RSSI") {
    return String(state.rssi);
  }

  return String();
}



void startWebServer() {
// Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");

    server.on("/light_on", HTTP_GET, [](AsyncWebServerRequest *request) {
      commandLightOn();
      request->redirect("/");
    });

    server.on("/light_off", HTTP_GET, [](AsyncWebServerRequest *request) {
      commandLightOff();
      request->redirect("/");
    });

    server.on("/door_open", HTTP_GET, [](AsyncWebServerRequest *request) {
      commandDoorUp(true);
      request->redirect("/");
    });

    server.on("/door_close", HTTP_GET, [](AsyncWebServerRequest *request) {
      commandDoorDown(true);
      request->redirect("/");
    });
    server.begin();
}

void startApMode() {
// Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER");

    IPAddress IP = WiFi.softAPIP();
    dnsServer.start(53, "*", IP);
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
}

void net_init() {
  
  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  apMode = !initWiFi();

  if(!apMode) {
    NTP.setTimeZone(TZ_Europe_Paris);
  }
}

void net_start() {
  if(apMode) {
    startApMode();
  }
  else {
    NTP.begin();
    startWebServer();
  }
}

int get_rssi() {
  return WiFi.RSSI();
}

void net_loop(){
  if(apMode) {
    dnsServer.processNextRequest();
  }
  ArduinoOTA.handle();
}