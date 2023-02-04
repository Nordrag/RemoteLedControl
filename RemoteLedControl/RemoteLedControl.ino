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

const char* localHost = "http://localhost:5289/Devices";
const char* azureServerDevicesUrl = "https://arduinowebapi.azure-api.net/Devices";
const char* azureServerTimersUrl = "https://arduinowebapi.azure-api.net/Datetime";
const char* azureServerBaseUrl = "https://arduinowebapi.azure-api.net/";

StaticJsonDocument<256> doc;
DeserializationError err;

const int _measureUpdateRate = 5;
int measurreUpdateRate = 5;

String weekDays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

String months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

WiFiUDP udp;
NTPClient timeClient(udp);

const int pumpOutput = 4;
const int lightOutput = 16;
const int heatSensor = 13;
int deviceId;
bool isPumpOn, isLightOn;
bool isRequestOver = false;
bool isTimerOn = false;
bool lockOut = false;
bool wasRequestBeforeTimer = false;
DateTime Now(2022, 10, 10, 12, 0, 0);
DateTime timer(2022, 10, 10, 12, 0, 0);
DateTime onSetupTimer(2022, 10, 10, 12, 0, 0);
WiFiManager wMan;
String ssid, pw;
WebServer server(80);

OneWire oneWire(heatSensor);
DallasTemperature sensors(&oneWire);

int prevSecond = 0;
bool shouldTick = false;

unsigned long current, old, longDeltaTime;
int refreshRate = 5;
int workTimeDelta;
int year, month, day, h, m, s;
int currYear, currMonth, monthDay, currentHour, currentMinute, currentSeconds;
struct tm* ptm;

int temp = 0;
float ph = 0;
Preferences prefs;
TaskHandle_t TemperatureReader;
#pragma endregion

#pragma region WebServer
void Restart()
{
    server.send(200, "text/html", "pump on");
    ESP.restart();
}

void GetAllStatus()
{
    server.send(200, "text/html", String(String(isPumpOn) + "|" + String(isLightOn) + "|" + String(isTimerOn)));
}

void PumpOn()
{
    digitalWrite(pumpOutput, LOW);
    server.send(200, "text/html", "pump on");
    isPumpOn = true;
    LOG("on");
}

void GetTemperature()
{
    server.send(200, "text/html", String(temp));
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

void SendTimerStatus()
{
    server.send(200, "text/html", isTimerOn ? "on" : "off");
}

void SendTemperature()
{
    server.send(200, "text/html", String(temp));
}

#pragma endregion

void GetStatusFromServer()
{
    HTTPClient client;
    String requestUrl = azureServerBaseUrl;
    requestUrl += "/Status?id=";
    requestUrl += deviceId;
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();  
    String payload = client.getString();        
    if (code > 0)
    {
        err = deserializeJson(doc, payload);      
        isPumpOn = doc["isPumpOn"];
        isLightOn = doc["isLightOn"];
        isTimerOn = doc["isTimerOn"];
        refreshRate = doc["refreshRate"];
        temp = doc["temperature"];
        ph = doc["ph"];
       
        if (isTimerOn == 1 && shouldTick)
        {
           
        }  
        else
        {
            workTimeDelta = doc["maxRunTime"];
            workTimeDelta *= 60;
        }     
    }
    else
    {
        refreshRate = 3000;
    }    
    client.end();
    GetCurrentTimer();
}

//void GetRemainingTime()
//{
//    server.send(200, "text/html", "remTime");
//    HTTPClient client;
//    String requestUrl = azureServerTimersUrl;
//    requestUrl += "/getRemainingTime?deviceId=";
//    requestUrl += deviceId;
//    client.begin(requestUrl);
//    client.addHeader("accept", "application/json");
//    client.addHeader("Content-Type", "text/plain");
//    auto code = client.GET();
//    if (code > 0)
//    {
//        String payload = client.getString();
//        err = deserializeJson(doc, payload);
//        String d = doc["time"];
//        workTimeDelta = d.toInt() * 60;
//        isTimerOn = doc["isActive"];
//    }
//    client.end();
//}

//void ToggleTimer()
//{
//    GetRemainingTime();  
//}

void GetCurrentTimer()
{      
    HTTPClient client;
    String request = azureServerTimersUrl;
    server.send(200, "text/html", "getting current time");
    request += "/getCurrentTimer?deviceId=";
    request += deviceId;
    client.begin(request);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    int code = client.GET();
    String payload = client.getString();                 
    if (code > 0)
    {       
        err = deserializeJson(doc, payload);
        year = doc["year"];
        month = doc["month"];
        day = doc["day"];
        h = doc["hour"];
        m = doc["minutes"];
        s = doc["seconds"];           
        timer.UpdateTime(year, month, day, h, m, s);         
    }
    client.end();   
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

    String payload = client.getString();    
    //LOG(payload);
    if (code > 0)
    {
        err = deserializeJson(doc, payload);
        year = doc["year"];
        month = doc["month"];
        day = doc["day"];
        h = doc["hour"];
        m = doc["minutes"];
        s = doc["seconds"];
        workTimeDelta = doc["workTime"];
        isTimerOn = doc["isTimerOn"];
        workTimeDelta *= 60;
        timer.UpdateTime(year, month, day, h, m, s);
        wasRequestBeforeTimer = false;              
    }
    client.end();
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
    client.begin(azureServerDevicesUrl);
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "application/json");
    int httpCode = client.POST(jResult);
    client.end();
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

    if (currentSeconds != prevSecond)
    {
        refreshRate--;
        measurreUpdateRate--;
        prevSecond = currentSeconds;         
        if (isTimerOn == 1 && shouldTick)
        {
            workTimeDelta--;
            LOG(workTimeDelta);
        }
        if (refreshRate <= 0)
        {
            GetStatusFromServer();
        }
    }

    //String weekDay = weekDays[timeClient.getDay()];   
    Now.UpdateTime(currYear, currMonth, monthDay, currentHour, currentMinute, currentSeconds);
}

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

void SendStatusToServer()
{
    StaticJsonDocument<256> pRequestDoc;
    JsonObject jObj = pRequestDoc.to<JsonObject>();
    String jResult;
    jObj["id"] = deviceId;
    jObj["temperature"] = temp;
    jObj["ph"] = ph;
    jObj["isPumpOn"] = isPumpOn;
    jObj["isLightOn"] = isLightOn;
    jObj["isTimerOn"] = isTimerOn;
    serializeJson(pRequestDoc, jResult);
    HTTPClient client;
    String url = azureServerBaseUrl;
    url += "Status";
    client.begin(url);
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "application/json");
    int httpCode = client.PUT(jResult);

    if (httpCode < 0)
    {
        isPumpOn = false;
        isTimerOn = false;
        workTimeDelta = 0;
    }
    client.end();
}

void UpdateMeasures(void* param)
{
    while (true)
    {      
        sensors.requestTemperatures();
        temp = sensors.getTempCByIndex(0);            
        delay(1000);
    }
}

void StateUpdate()
{
    shouldTick = DateTime::CompareTime(&Now, &timer);
    if (isTimerOn == 1)
    {
        if (shouldTick)
        {
            if (!lockOut)
            {
                lockOut = true;
                SendStatusToServer();
                LOG("timer on");
            }
            isPumpOn = true;
            digitalWrite(pumpOutput, LOW);
            if (workTimeDelta <= 0)
            {
                LOG("timer off");
                isPumpOn = false;
                SendStatusToServer();
                lockOut = false;
                GetNextTimer();
                digitalWrite(pumpOutput, HIGH);
            }
        }
    }
  
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
    GetCurrentTimer();
    UpdateDateTime();
    GetStatusFromServer();
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
    //server.on("/toggleTimer", ToggleTimer);
    server.on("/timerStatus", SendTimerStatus);
    server.on("/allStatus", GetAllStatus);
    server.on("/restart", Restart);
    server.onNotFound(NotFound);   
    server.begin(); 

    xTaskCreatePinnedToCore(UpdateMeasures, "TempReader", 10000, nullptr, 1, &TemperatureReader, 1);
}

void loop() {

    timeClient.update();   
    UpdateDateTime();
    server.handleClient();     
    StateUpdate();    

    if (measurreUpdateRate <= 0)
    {
        SendMeasuresToServer();
        measurreUpdateRate = _measureUpdateRate;
    }
}

