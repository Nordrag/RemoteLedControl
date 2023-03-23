#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include "WifiConnector.h"
#include "ArduinoJson.h"
#include "DateTime.h"
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "States.h"
#include "StateMachine.h"
#define LOG(X) Serial.println(X)

#pragma region Members

const char* azureServerBaseUrl = "https://arduinowebapi.azure-api.net/";

//wifi connection
WiFiUDP udp;
NTPClient timeClient(udp);

//hardware specs
const int pumpOutput = 4;
const int lightOutput = 17;
const int heatSensor = 13;
const int consumptionInput = 19;
const int resetInput = 18;
const int heatPumpInput = 16;
bool PumpOn, LightOn, IsPumpManual, IsLightManual;
int deviceId;
bool secondsTicked = false;
bool autoHeat = false;

//net connection
WiFiManager wMan;
String ssid, pw;

//timers
DateTime Now(2022, 10, 10, 12, 0, 0);
DateTime pTimer(2022, 10, 10, 12, 0, 0);
DateTime lTimer(2022, 10, 10, 12, 0, 0);

int prevSecond = 0;
bool pShouldTick = false;
bool lShouldTick = false;

const int _measureUpdateRate = 5;
int measurreUpdateRate = 5;
int refreshRate = 5;
int workTimeDelta, lightTimerDelta;
int year, month, day, h, m, s, lYear, lMonth, lDay, lh,lm,ls;
int currYear, currMonth, monthDay, currentHour, currentMinute, currentSeconds;
struct tm* ptm;

//measurements
OneWire oneWire(heatSensor);
DallasTemperature sensors(&oneWire);

int temp = 0;
float ph = 0;
TaskHandle_t TemperatureReader;

//persistence
Preferences prefs;

//statemachine members
unsigned int MANUAL_STATE;
unsigned int TIMER_STATE;
unsigned int OVERRIDE_STATE;

bool inManualState = false;
bool inTimerState = false;
bool inOverrideState = false;

ManualState manualState;
TimerState timerState;
OverrideState overrideState;

StateMachine stateMachine;

void InitializeStateMachine()
{
    MANUAL_STATE = stateMachine.AddState(&manualState);
    TIMER_STATE = stateMachine.AddState(&timerState);
    OVERRIDE_STATE = stateMachine.AddState(&overrideState);

    stateMachine.AddAnyTransiton(OVERRIDE_STATE, &inOverrideState);

    stateMachine.AddTransition(MANUAL_STATE, TIMER_STATE, &inTimerState);
    stateMachine.AddTransition(TIMER_STATE, MANUAL_STATE, &inManualState);

    stateMachine.AddTransition(OVERRIDE_STATE, MANUAL_STATE, &inManualState);
    stateMachine.AddTransition(OVERRIDE_STATE, TIMER_STATE, &inTimerState);

    stateMachine.SetState(MANUAL_STATE);
}

#pragma endregion


void FactoryReset()
{
    prefs.clear();
    wMan.resetSettings();
    ESP.restart();
}

#pragma region New
//should be called with frequency of X
void GetStatusFromServer()
{
    StaticJsonDocument<512> doc;
    DeserializationError err;
    HTTPClient client;
    client.setTimeout(5000);
    String requestUrl = azureServerBaseUrl;
    requestUrl += "Status";
    requestUrl += "/fullStatus?id=";
    requestUrl += deviceId;
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();  
    String payload = client.getString();
    if (code > 0)
    {     
        err = deserializeJson(doc, payload);        
        //pump timer
        year = doc["year"];
        month = doc["month"];
        day = doc["day"];
        h = doc["hour"];
        m = doc["minutes"];
        s = doc["seconds"];
        IsPumpManual = doc["isPumpManual"];
     
        if (!IsPumpManual)
        {
            workTimeDelta = doc["pumpWorkTime"];
            workTimeDelta *= 60;           
        }    
        else
        {
            PumpOn = doc["isActive"];
            workTimeDelta = 0;
        }

        //light timer
        lYear = doc["lyear"];
        lMonth = doc["lmonth"];
        lDay = doc["lday"];
        lh = doc["lhour"];
        lm = doc["lminutes"];
        ls = doc["lseconds"];

        //IsLightManual = doc["isLightManual"];
       /* LOG(IsLightManual);
        if (!IsLightManual)
        {
            lightTimerDelta = doc["lightWorkTime"];
            lightTimerDelta *= 60;
        }   
        else
        {
            lightTimerDelta = 0;
            LightOn = doc["isLightActive"];          
        }*/

        LightOn = doc["isLightActive"];

        //LOG(LightOn);

        refreshRate = doc["refreshRate"];
           
        pTimer.UpdateTime(year, month, day, h, m, s);
        lTimer.UpdateTime(lYear, lMonth, lDay, lh, lm, ls);      
    }
    else
    {      
        refreshRate = 5;
        workTimeDelta = 0;
        lightTimerDelta = 0;
    }    
    client.end();  
}

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
    if (code > 0)
    {
        deviceId = client.getString().toInt();
        prefs.begin("waterpump", false);
        prefs.putInt("deviceId", deviceId);
        prefs.end();
    }
    client.end();
}

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

//updates the current time, used for comparing to server timers
void UpdateDateTime()
{
    timeClient.setTimeOffset(3600);
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    ptm = gmtime((time_t*)&epochTime);
    monthDay = ptm->tm_mday;
    currMonth = ptm->tm_mon + 1;
    currYear = ptm->tm_year + 1900;

    currentHour = timeClient.getHours();
    currentMinute = timeClient.getMinutes();
    currentSeconds = timeClient.getSeconds();   

    //String weekDay = weekDays[timeClient.getDay()];   
    Now.UpdateTime(currYear, currMonth, monthDay, currentHour, currentMinute, currentSeconds);

    pShouldTick = DateTime::CompareTime(&Now, &pTimer);
    lShouldTick = DateTime::CompareTime(&Now, &lTimer);

    if (currentSeconds != prevSecond)
    {
        //auto refresh delta
        refreshRate--;
        measurreUpdateRate--;
    }
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
void SendStatusToServer()
{
    StaticJsonDocument<256> pRequestDoc;
    JsonObject jObj = pRequestDoc.to<JsonObject>();
    String jResult;
    jObj["id"] = deviceId;   
    jObj["isPumpOn"] = workTimeDelta > 0;
    jObj["isLightOn"] = lightTimerDelta > 0;   
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

//runs on the 2nd thread taking measurements
void UpdateMeasures(void* param)
{
    while (true)
    {      
        sensors.requestTemperatures();
        temp = sensors.getTempCByIndex(0);            
        delay(10000);
    }
}

#pragma endregion

void setup() {
   
    WiFi.mode(WIFI_STA);
    Serial.begin(115200);  
    pinMode(pumpOutput, OUTPUT);
    pinMode(lightOutput, OUTPUT);
    pinMode(resetInput, INPUT);
    pinMode(heatPumpInput, INPUT);
    digitalWrite(pumpOutput, HIGH);
    digitalWrite(lightOutput, HIGH);
    //digitalWrite(resetInput, LOW);
    bool res = wMan.autoConnect("Waterpump", "password");
   
    prefs.begin("waterpump", false);
    deviceId = prefs.getInt("deviceId", 0);

    if (!res) {
        Serial.println("Failed to connect");                 
    }
    else {      
        Serial.println("connected");         
        ssid = wMan.getWiFiSSID();
        pw = wMan.getWiFiPass();
        sensors.begin();
    }
      
    wMan.setWiFiAutoReconnect(true);
    timeClient.begin();
    PostNewDevice();
    GetDeviceID();
    UpdateDateTime();
    GetStatusFromServer();  
    xTaskCreatePinnedToCore(UpdateMeasures, "TempReader", 10000, nullptr, 1, &TemperatureReader, 1);
    InitializeStateMachine();
}

void loop()
{
     timeClient.update();
     UpdateDateTime();
     autoHeat = analogRead(heatPumpInput) == HIGH;
    
     if (secondsTicked)
     {
         GetStatusFromServer();       
     }

     stateMachine.Update();

     inManualState = IsPumpManual;
     inTimerState = !IsPumpManual;
     inOverrideState = false;


     if (measurreUpdateRate <= 0)
     {
         SendMeasuresToServer();
         measurreUpdateRate = _measureUpdateRate;
     }

     secondsTicked = prevSecond != currentSeconds;   
     prevSecond = currentSeconds;
     
}
