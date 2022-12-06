#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include "WifiConnector.h"
#include "ArduinoJson.h"
#include "DateTime.h"
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>

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

// Assign output variables to GPIO pins
const int pumpOutput = 4;
const int lightOutput = 2;
int deviceId;
bool isOn;

bool hasTimerBeenSet = false;
bool timerActionTriggered = false;
DateTime Now(2022, 10, 10, 12,0,0);
DateTime timer(2022, 10, 10, 12, 0, 0);
WiFiManager wMan;
String ssid, pw;
WebServer server(80);
const char* prefDomain = "waterPump";
unsigned long current, old, longDeltaTime;
double deltaTime;
float workTimeDelta;
int year, month, day, h, m, s;
int currYear, currMonth, monthDay, currentHour, currentMinute, currentSeconds;
struct tm* ptm;

void LedOn()
{    
    digitalWrite(pumpOutput, LOW);
    server.send(200, "text/html", "led on");  
    isOn = true;
}

void LedOff()
{    
    digitalWrite(pumpOutput, HIGH);
    server.send(200, "text/html", "led off");
    isOn = false;
}

void OnConnect()
{
    server.send(200, "text/html", "base url");
}

void SendStatus()
{
    server.send(200, "text/html", isOn ? "on" : "off");
}

void NotFound()
{
    server.send(404, "text/html", "not found");
}

void SendTemperature()
{
    auto rand = random(17, 20);
    server.send(200, "text/html", String(rand));
}

void GetDateTime2()
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
        Serial.println(payload);
        err = deserializeJson(doc, payload);
        year = doc["year"];
        month = doc["month"];
        day = doc["day"];
        h = doc["hour"];
        m = doc["minutes"];
        s = doc["seconds"];
        workTimeDelta = doc["lenght"];
        workTimeDelta *= 60;

        timer.UpdateTime(year, month, day, h, m, s);
        hasTimerBeenSet = true;       
    }
}

void GetNextTimer()
{
    server.send(200, "text/html", "nextTime");
    HTTPClient client;
    String requestUrl = azureServerTimersUrl;
    requestUrl += "?deviceId=";
    requestUrl += deviceId;
    client.begin(requestUrl);
    client.addHeader("accept", "application/json");
    client.addHeader("Content-Type", "text/plain");
    auto code = client.GET();
    if (code > 0)
    {
        String payload = client.getString();
        Serial.println(payload);
        err = deserializeJson(doc, payload);
        year = doc["year"];
        month = doc["month"];
        day = doc["day"];
        h = doc["hours"];
        m = doc["minutes"];
        s = doc["seconds"];
        workTimeDelta = doc["lenght"];
        workTimeDelta *= 60;
        timer.UpdateTime(year, month, day, h, m, s);
        hasTimerBeenSet = true;
        Serial.println(year);
        Serial.println(month);
        Serial.println(day);
        Serial.println(h);
        Serial.println(m);
    }
}

void GetDateTime()
{
    String params;
    server.send(200, "text/html", server.hostHeader());
    if (server.args() > 0)
    {
        for (size_t i = 0; i < server.args(); i++)
        {          
            params += server.argName(i);
        }
    }
    
  
    err = deserializeJson(doc, params);
    year = doc["year"];
    month = doc["month"];
    day = doc["day"];
    h = doc["hours"];
    m = doc["minutes"];
    s = doc["seconds"];
    workTimeDelta = doc["worktime"];
       
    timer.UpdateTime(year, month, day, h, m, s);    
    hasTimerBeenSet = true;
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

void setup() {
   
    Serial.begin(115200);  
    pinMode(pumpOutput, OUTPUT);
    digitalWrite(pumpOutput, HIGH);
   // pinMode(lightOutput, OUTPUT);
    bool res = wMan.autoConnect("AutoConnectAP", "password");

    
    if (!res) {
        Serial.println("Failed to connect");
        timeClient.begin();           
    }
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");   
        ssid = wMan.getWiFiSSID();
        pw = wMan.getWiFiPass();
    }
      
    
    PostNewDevice();
    GetDeviceID();
    
    server.on("/", OnConnect);
    server.on("/ledOn", LedOn);
    server.on("/ledOff", LedOff);
    server.on("/dateTime", GetDateTime2);   
    server.on("/status", SendStatus);   
    server.on("/temp", SendTemperature);   
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

    if (hasTimerBeenSet == 0)
    {
        workTimeDelta -= deltaTime;
        if (DateTime::IsToday(&timer, &Now))
        {
            if (DateTime::CompareDayTime(&Now, &timer))
            {               
                GetNextTimer();
                delay(2000);
            }           
        }
        if (workTimeDelta <= 0)
        {
            isOn = false;
            digitalWrite(pumpOutput, HIGH);       
        }
        else
        {
            isOn = true;
            digitalWrite(pumpOutput, LOW);
        }
    }
}

