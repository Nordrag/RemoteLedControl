#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include "WifiConnector.h"
#include "ArduinoJson.h"
#include "DateTime.h"
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
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
bool PumpOn, LightOn, IsPumpManual, IsLightManual;
int deviceId;

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
unsigned long current, old, longDeltaTime;
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

//lockouts
bool wasPumpStatusSent = false;
bool wasLightStatusSent = false;

//persistence
Preferences prefs;
#pragma endregion

#pragma region New
//should be called with frequency of X
void GetStatusFromServer()
{
    StaticJsonDocument<512> doc;
    DeserializationError err;
    HTTPClient client;
    String requestUrl = azureServerBaseUrl;
    requestUrl += "Status";
    requestUrl += "/fullStatus?id=";
    requestUrl += deviceId;
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();  
    String payload = client.getString();      
    //LOG(payload);
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
        delay(1000);
    }
}

#pragma endregion

void StateUpdate()
{
    pShouldTick = DateTime::CompareTime(&Now, &pTimer);
    lShouldTick = DateTime::CompareTime(&Now, &lTimer);
  
    if (currentSeconds != prevSecond)
    {
        //auto refresh delta
        refreshRate--;
        measurreUpdateRate--;  

       /* int volt = analogRead(consumptionInput);
        LOG(volt);*/
        LOG(esp_get_free_heap_size());
    }

    if (IsPumpManual)
    {      
        digitalWrite(pumpOutput, PumpOn? LOW : HIGH);
    }
    else
    {
        if (currentSeconds != prevSecond)
        {           
            //pump toggle                 
            if (workTimeDelta > 0 && pShouldTick)
            {
                workTimeDelta--;
                LOG("pump on");
                PumpOn = true;
                digitalWrite(pumpOutput, LOW);
            }
            else
            {
                digitalWrite(pumpOutput, HIGH);
                if (!wasPumpStatusSent)
                {
                    LOG("pump off");
                    PumpOn = false;
                    SendStatusToServer();
                    wasPumpStatusSent = true;
                }
            }
        }
    }

    digitalWrite(lightOutput, LightOn? LOW : HIGH);
    /*if (IsLightManual)
    {
    }*/
    //else
    //{
    //    if (currentSeconds != prevSecond)
    //    {
    //        //light toggle
    //        if (lightTimerDelta > 0 && lShouldTick)
    //        {
    //            lightTimerDelta--;
    //            LightOn = true;
    //            LOG("light on");
    //            digitalWrite(lightOutput, LOW);
    //        }
    //        else
    //        {
    //            digitalWrite(lightOutput, HIGH);
    //            if (!wasLightStatusSent)
    //            {
    //                LOG("light off");
    //                LightOn = false;
    //                SendStatusToServer();
    //                wasLightStatusSent = true;
    //            }
    //        }
    //    }
    //}
                           //auto refresh call
        if (refreshRate <= 0)
        {
            GetStatusFromServer();
        }
    prevSecond = currentSeconds;
} 

void setup() {
   
    Serial.begin(115200);  
    pinMode(pumpOutput, OUTPUT);
    pinMode(lightOutput, OUTPUT);
    digitalWrite(pumpOutput, HIGH);
    digitalWrite(lightOutput, HIGH);
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
      
    timeClient.begin();
    PostNewDevice();
    GetDeviceID();
    UpdateDateTime();
    GetStatusFromServer();

    LOG(WiFi.localIP().toString());
    xTaskCreatePinnedToCore(UpdateMeasures, "TempReader", 10000, nullptr, 1, &TemperatureReader, 1);
}

void loop() {

    timeClient.update();   
    UpdateDateTime();    
    StateUpdate();    

    if (measurreUpdateRate <= 0)
    {
        SendMeasuresToServer();
        measurreUpdateRate = _measureUpdateRate;
    }
}

