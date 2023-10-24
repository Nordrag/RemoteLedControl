#pragma once
#include <HTTP_Method.h>
#include <Uri.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include "WifiConnector.h"
#include "ArduinoJson.h"
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "States.h"
#include "StateMachine.h"
#include "LocalTimer.h"
#include <functional>
#include <vector>
#include <map>
#include "WebSiteBuilder.h"
#define LOG(X) Serial.println(X)

/*
* esp32 microcontroller running a water pump and meausuring temperature
* the device was connected to an api server for remote controll
* the front end was also able to connect to this device if they were on the same wifi network
* the device could be manually started as well with hardware buttons
* the device could turn on and off based on timers, and a secondary water heater
*/


#pragma region Members

//not online ofc
const char* azureServerBaseUrl = "https://arduinowebapi.azure-api.net/";

//wifi connection
WiFiUDP udp;
NTPClient timeClient(udp);

//hardware specs and pins
const int pumpOutput = 4;
const int lightOutput = 17;
const int heatSensor = 13;
const int consumptionInput = 19;
const int lightReadInput = 18;
const int heatPumpInput = 16;
bool PumpOn, LightOn, IsPumpManual, IsLightManual;
int deviceId;
bool secondsTicked = false;
bool autoHeat = false;
bool lightSwitch = false;
//net connection
WiFiManager wMan;
String ssid, pw;

//timers from server
std::vector<Day> timers;
//the toggle for manual controll
bool isLocalManual = false;

int prevSecond = 0;

const int _measureUpdateRate = 300;
int measurreUpdateRate = 300;
int refreshRate = 5;
int workTimeDelta, lightTimerDelta;
long localSeconds = 0;
int localDay = 0;
struct tm* ptm;

//heat measurements
OneWire oneWire(heatSensor);
DallasTemperature sensors(&oneWire);

int temp = 0;
float ph = 0;
//esp32 multi thread object
TaskHandle_t TemperatureReader;

//persistence
Preferences prefs;

//statemachine members
unsigned int MANUAL_STATE;
unsigned int TIMER_STATE;
unsigned int OVERRIDE_STATE;
unsigned int LOCAL_STATE;

bool inManualState = false;
bool inTimerState = false;
bool inOverrideState = false;
bool inLocalState = false;

bool isSubscriber = false;

//state machine initialization
ManualState manualState;
TimerState timerState;
OverrideState overrideState;
LocalTimerState localTimerState;

StateMachine stateMachine;

void InitializeStateMachine()
{
    MANUAL_STATE = stateMachine.AddState(&manualState);
    TIMER_STATE = stateMachine.AddState(&timerState);
    OVERRIDE_STATE = stateMachine.AddState(&overrideState);
    LOCAL_STATE = stateMachine.AddState(&localTimerState);

    stateMachine.AddAnyTransiton(OVERRIDE_STATE, &inOverrideState);
    stateMachine.AddAnyTransiton(LOCAL_STATE, &inLocalState);

    stateMachine.AddTransition(MANUAL_STATE, TIMER_STATE, &inTimerState);
    stateMachine.AddTransition(TIMER_STATE, MANUAL_STATE, &inManualState);

    stateMachine.AddTransition(OVERRIDE_STATE, MANUAL_STATE, &inManualState);
    stateMachine.AddTransition(OVERRIDE_STATE, TIMER_STATE, &inTimerState);
    stateMachine.AddTransition(OVERRIDE_STATE, LOCAL_STATE, &inLocalState);
    stateMachine.AddTransition(LOCAL_STATE, MANUAL_STATE, &inManualState);
    stateMachine.AddTransition(LOCAL_STATE, TIMER_STATE, &inTimerState);

    stateMachine.SetState(isSubscriber? MANUAL_STATE : LOCAL_STATE);
}

//local server for direct phone controll
WebServer server(80);

const char* htmlType = "text/html";
const char* jsonType = "application/json";

#pragma endregion

void FactoryReset()
{
    prefs.clear();
    wMan.resetSettings();
    ESP.restart();
}

#pragma region New
//polled from server
int GetStatusFromServer()
{
    StaticJsonDocument<512> doc;
    DeserializationError err;
    HTTPClient client;
    client.setTimeout(5000);
    String requestUrl = azureServerBaseUrl;
    requestUrl += "Status";
    requestUrl += "/simpleStatus?id=";
    requestUrl += deviceId;
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();  
    String payload = client.getString();
    if (code > 0)
    {     
        err = deserializeJson(doc, payload);            
        int subId = doc["subscriptionTierId"];
        isSubscriber = subId > 1;
        IsPumpManual = doc["isPumpManual"];      

        if (IsPumpManual)
        {
            PumpOn = doc["isActive"];
        }    

        LightOn = doc["isLightActive"];       

        refreshRate = doc["refreshRate"];
               
    }
    else
    {      
        LOG(String("could not get status from server ") + String(code) + String(deviceId));
        refreshRate = 5;
        workTimeDelta = 0;
        lightTimerDelta = 0;
    }    
    client.end();  
    return code;
}

//after the device registers itself on the server, get its primary key for database access
void GetDeviceID()
{
    HTTPClient client;
    String requestUrl = azureServerBaseUrl;
    requestUrl += "Devices";
    requestUrl += "/getId?SSID=";
    requestUrl += ssid;
    requestUrl += "&Password=";
    requestUrl += pw;
    client.begin(requestUrl);
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();
    LOG(requestUrl);
    if (code > 0)
    {
        deviceId = client.getString().toInt();      
        prefs.end();
    }
    LOG(client.getString().toInt());
    client.end();
}

//register the device on the server
void PostNewDevice()
{
    StaticJsonDocument<256> pRequestDoc;
    JsonObject jObj = pRequestDoc.to<JsonObject>();
    String jResult;
    jObj["id"] = 0;
    jObj["deviceType"] = 0;
    jObj["deviceName"] = "WaterPump";
    jObj["ssid"] = ssid;
    jObj["routerPassword"] = pw;
    jObj["userID"] = 1;
    jObj["ip"] = WiFi.localIP().toString();
    serializeJson(pRequestDoc, jResult);
    HTTPClient client;
    client.begin(azureServerBaseUrl + String("Devices"));
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "application/json");
    int httpCode = client.POST(jResult);
    client.end();
}

//send the temp and ph to the server
void SendMeasuresToServer()
{
    StaticJsonDocument<256> pRequestDoc;
    JsonObject jObj = pRequestDoc.to<JsonObject>();
    String jResult;
    jObj["id"] = deviceId;
    jObj["temp"] = temp;
    jObj["ph"] = ph;  
    serializeJson(pRequestDoc, jResult);
    HTTPClient client;
    String url = azureServerBaseUrl;
    url += "Status";
    url += "/measures";
    client.begin(url);
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "application/json");
    int httpCode = client.PUT(jResult);
    client.end();
}

//sends the activity status to the server
void SendStatusToServer(bool pump, bool light)
{
    StaticJsonDocument<256> pRequestDoc;
    JsonObject jObj = pRequestDoc.to<JsonObject>();
    String jResult;
    jObj["id"] = deviceId;   
    jObj["isPumpOn"] = pump;
    jObj["isLightOn"] = light;
    jObj["heatOverride"] = inOverrideState;
    serializeJson(pRequestDoc, jResult);
    HTTPClient client;
    String url = azureServerBaseUrl;
    url += "Status";
    client.begin(url);
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "application/json");
    int httpCode = client.PUT(jResult);
 
    //LOG(jResult);    
    client.end();
}

//gets all the timers
int GetAllTimers()
{ 
    StaticJsonDocument<2048> doc;
    HTTPClient client;
    String requestUrl = azureServerBaseUrl;
    requestUrl += "Datetime";
    requestUrl += "/getAllTimers?deviceId=";
    requestUrl += deviceId;
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();
    String payload = client.getString();
    deserializeJson(doc, payload);
    if (code > 0)
    {
        timers.clear();

        for (JsonVariant dayObj : doc.as<JsonArray>()) {
            Day day;
            day.day = dayObj["day"].as<int>();
            // Loop through the timers array and deserialize each object into a Timer instance
            for (JsonVariant timerObj : dayObj["timers"].as<JsonArray>()) {
                Timer timer;
                timer.from = timerObj["from"].as<long>();
                timer.to = timerObj["to"].as<long>();
                day.timers.push_back(timer);
            }
            timers.push_back(day);           
        }    
    }
    LOG(payload);
    client.end();
    return code;
}

//gets the day and time from the server
void GetCurrentTime()
{
    //Datetime/getCurrentTime
    StaticJsonDocument<512> doc;
    DeserializationError err;
    HTTPClient client; 
    String requestUrl = azureServerBaseUrl;
    requestUrl += "Datetime";
    requestUrl += "/getCurrentTime";
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();
    String payload = client.getString();
    deserializeJson(doc, payload);
    LOG(payload);

    if (code >0)
    {
        localDay = doc["item1"];
        localSeconds = doc["item2"];
        LOG(localDay);
        LOG(localSeconds);
    }
}

//runs on the 2nd thread taking measurements
void UpdateMeasures(void* param)
{
    while (true)
    {     
        //using this thread to manually count seconds as well
        delay(1000);
        OnSecondsPassed();    
        //every minute we measure the temperature
        if (localSeconds % 60 == 0)
        {
            sensors.requestTemperatures();
            temp = sensors.getTempCByIndex(0);            
        }
    }
}

#pragma endregion

#pragma region local server

//serializes a response object, direct html to phone was scrapped for custom front end
template<typename T>
String ArduinoResult(bool success, const T& json, const String& reason, size_t size = 256)
{
    const size_t capacity = JSON_OBJECT_SIZE(3) + size + reason.length();
    DynamicJsonDocument doc(capacity);

    // Add the properties to the JSON object
    doc["Success"] = success;
    doc["Result"] = json;
    doc["Reason"] = reason;

    // Serialize the JSON object to a String
    String jsonString;
    serializeJson(doc, jsonString);

    return jsonString;
}

//esp32 can create a local api server, these are the endpoint functions
void Echo()
{
    //String body = server.arg("plain");
    LOG("Pong");
    server.send(200, jsonType, ArduinoResult(true, "pong", "ok"));
}

void TogglePump()
{  
    PumpOn = !PumpOn;
}

void ToggleLight()
{ 
    LightOn = !LightOn;
}

void ToggleManual()
{ 
    isLocalManual = !isLocalManual;
}

void ServerInit()
{
    server.on("/tPump", TogglePump);
    server.on("/tLight", ToggleLight);
    server.on("/tManual", ToggleManual);
    server.on("/echo", Echo);
}



#pragma endregion

#pragma region local timers

//everything here that wants to be check continously
void OnSecondsPassed()
{
    localSeconds++;

    refreshRate--;
    measurreUpdateRate--;

    if (localSeconds >= 86400)
    {
        localDay++;
        if (localDay > 6)
        {
            localDay = 0;
        }
        localSeconds = 0;
    }

    if (measurreUpdateRate <= 0)
    {
        SendMeasuresToServer();
        measurreUpdateRate = _measureUpdateRate;
    }  
}

#pragma endregion

void setup() {
    
    Serial.begin(115200);  
    pinMode(pumpOutput, OUTPUT);
    pinMode(lightOutput, OUTPUT);
    pinMode(lightReadInput, INPUT);
    pinMode(heatPumpInput, INPUT);
    digitalWrite(heatPumpInput, HIGH);
    digitalWrite(pumpOutput, HIGH);
    digitalWrite(lightOutput, HIGH);
    //create a wifi hotspot, to connect the device to the users network, creates a local wifi this name and password
    bool res = wMan.autoConnect("Waterpump", "password");
  

    if (!res) {
        Serial.println("Failed to connect");                 
    }
    else {      
        Serial.println("connected");         
        ssid = wMan.getWiFiSSID();
        pw = wMan.getWiFiPass();
        sensors.begin();
    }
      
    //get data from server
    wMan.setWiFiAutoReconnect(true);
    GetCurrentTime();
    PostNewDevice();
    GetDeviceID();
    GetStatusFromServer();  
    GetAllTimers();
    //launch the 2nd thread
    xTaskCreatePinnedToCore(UpdateMeasures, "TempReader", 10000, nullptr, 1, &TemperatureReader, 1);
    //initialize state machine
    InitializeStateMachine();
    //start the local api server
    ServerInit();
    server.begin();
}

void loop()
{
    //handle the api clients
     server.handleClient();  
     //read the low / high voltage signal from the heater
     autoHeat = digitalRead(heatPumpInput) == HIGH;
    //run the current state logic, see states.h and StateMachine.h
     stateMachine.Update();

     if (isSubscriber)
     {
         if (refreshRate <= 0)
         {
             GetStatusFromServer();           
         }

         inManualState = IsPumpManual;
         inTimerState = !IsPumpManual;
     }

     inOverrideState = autoHeat && !IsPumpManual;      
}
