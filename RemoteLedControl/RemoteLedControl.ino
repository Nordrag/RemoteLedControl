#pragma once
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

const char* localHost = "http://localhost:5289/Devices";
const char* azureServerDevicesUrl = "https://arduinowebapi.azure-api.net/Devices";
const char* azureServerTimersUrl = "https://arduinowebapi.azure-api.net/Datetime";

StaticJsonDocument<256> doc;
const char* input;
DeserializationError err;

String weekDays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

String months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

WiFiUDP udp;
NTPClient timeClient(udp);

const int pumpOutput = 4;
const int lightOutput = 2;
int deviceId;
bool isPumpOn, isLightOn;
bool isRequestOver = false;
bool hasTimerBeenSet = false;
bool isTimerActive;
bool wasRequestBeforeTimer = false;
DateTime Now(2022, 10, 10, 12,0,0);
DateTime timer(2022, 10, 10, 12, 0, 0);
DateTime onSetupTimer(2022, 10, 10, 12, 0, 0);
WiFiManager wMan;
String ssid, pw;
WebServer server(80);

unsigned long current, old, longDeltaTime;
double deltaTime;
float workTimeDelta;
int year, month, day, h, m, s;
int currYear, currMonth, monthDay, currentHour, currentMinute, currentSeconds;
struct tm* ptm;

int State = 0;
Preferences prefs;

void PumpOn()
{    
    digitalWrite(pumpOutput, LOW);
    server.send(200, "text/html", "pump on");  
    isPumpOn = true;
}

void PumpOff()
{    
    digitalWrite(pumpOutput, HIGH);
    server.send(200, "text/html", "pump off");
    isPumpOn = false;
}

void LightsOn()
{
    digitalWrite(lightOutput, LOW);
    server.send(200, "text/html", "lighs on");
    isLightOn = true;
}

void LightsOff()
{
    digitalWrite(lightOutput, HIGH);
    server.send(200, "text/html", "lighs off");
    isLightOn = false;
}

void OnConnect()
{
    server.send(200, "text/html", "base url");
}

void SendPumpStatus()
{
    server.send(200, "text/html", isPumpOn ? "on" : "off");
}

void SendLightStatus()
{
    server.send(200, "text/html", isLightOn ? "on" : "off");
}

void NotFound()
{
    server.send(404, "text/html", "not found");
}

void ToggleTimer()
{
    hasTimerBeenSet = !hasTimerBeenSet;
    State = hasTimerBeenSet;
    server.send(200, "text/html", hasTimerBeenSet? "timer on" : "timer off");
}

void SendTimerStatus()
{
    server.send(200, "text/html", hasTimerBeenSet ? "on" : "off");
}

void SendTemperature()
{
    auto rand = random(17, 20);
    server.send(200, "text/html", String(rand));
}

void GetCurrentTimer()
{   
    server.send(200, "text/html", "time");
    HTTPClient client;
    String request = azureServerTimersUrl;
    request += "/getCurrentTimer?deviceId=";
    request += deviceId;
    client.begin(request);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    int code = client.GET();
    if (code > 0)
    {
        String payload = client.getString();      
        err = deserializeJson(doc, payload);
        year = doc["year"];
        month = doc["month"];
        day = doc["day"];
        h = doc["hour"];
        m = doc["minutes"];
        s = doc["seconds"];
        workTimeDelta = doc["lenght"];
        hasTimerBeenSet = doc["isActive"];
        workTimeDelta *= 60;      
        timer.UpdateTime(year, month, day, h, m, s);           
        State = hasTimerBeenSet;     
    }
}

void GetNextTimer()
{  
    server.send(200, "text/html", "nextTime");
    HTTPClient client;
    String requestUrl = azureServerTimersUrl;
    requestUrl += "/minutesDebug?deviceId=";
    requestUrl += deviceId;
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();
    if (code > 0)
    {
        String payload = client.getString();    
        err = deserializeJson(doc, payload);
        year = doc["year"];
        month = doc["month"];
        day = doc["day"];
        h = doc["hour"];
        m = doc["minutes"];
        s = doc["seconds"];
        workTimeDelta = doc["lenght"];
        hasTimerBeenSet = doc["isActive"];
        workTimeDelta *= 60;
        timer.UpdateTime(year, month, day, h, m, s);
        wasRequestBeforeTimer = false;              
    }
}

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

void GetDeviceID()
{
    HTTPClient client;
    String requestUrl = azureServerDevicesUrl;
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
    client.begin(azureServerDevicesUrl);
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "application/json");
    int httpCode = client.POST(jResult);
    client.end();
}

void StateUpdate()
{
    switch (State)
    {
    case 0:
        break;
    case 1:
        if (DateTime::CompareTime(&Now, &timer))
        {
            
                workTimeDelta -= deltaTime;
                isPumpOn = true;
                digitalWrite(pumpOutput, LOW);
                if (workTimeDelta <= 0)
                {
                    isPumpOn = false;
                    hasTimerBeenSet = false;
                    GetNextTimer();
                    digitalWrite(pumpOutput, HIGH);
                }
            
        }
        break;
    default:
        break;
    }
}

void setup() {
   
    Serial.begin(115200);  
    pinMode(pumpOutput, OUTPUT);
    digitalWrite(pumpOutput, HIGH);
   // pinMode(lightOutput, OUTPUT);
    bool res = wMan.autoConnect("AutoConnectAP", "password");
   
    prefs.begin("waterpump", false);
    deviceId = prefs.getInt("deviceId", 0);

    if (!res) {
        Serial.println("Failed to connect");                 
    }
    else {      
        Serial.println("connected");         
        ssid = wMan.getWiFiSSID();
        pw = wMan.getWiFiPass();
    }
      
    timeClient.begin();
    PostNewDevice();
    GetDeviceID();
    GetCurrentTimer();
    UpdateDateTime();
    onSetupTimer.UpdateTime(currYear, currMonth, monthDay, currentHour, currentMinute, 0);
    wasRequestBeforeTimer = DateTime::CompareDayTime(&onSetupTimer, &timer);
    
    server.on("/", OnConnect);
    server.on("/pumpOn", PumpOn);
    server.on("/pumpOff", PumpOff);
    server.on("/dateTime", GetCurrentTimer);   
    server.on("/status", SendPumpStatus);   
    server.on("/temp", SendTemperature);   
    server.on("/lightOn", LightsOn);   
    server.on("/lightOff", LightsOff);   
    server.on("/lightStatus", SendLightStatus);
    server.on("/toggleTimer", ToggleTimer);
    server.on("/timerStatus", SendTimerStatus);
    server.onNotFound(NotFound);   
    server.begin(); 
}


void loop() {
    
    old = current;
    current = millis();
    longDeltaTime = current - old;
    deltaTime = longDeltaTime * 0.001;   
    timeClient.update();   
    UpdateDateTime();
    server.handleClient();  
    
    StateUpdate();
}

