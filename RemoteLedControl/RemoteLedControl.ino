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
const char* azureServerBaseUrl = "https://arduinowebapi.azure-api.net/Devices";

const char* SSID = "DIGI-e9gF";
const char* PW = "8MPyJHjv";

StaticJsonDocument<128> doc;
const char* input;
DeserializationError err;

String weekDays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

String months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org");

// Assign output variables to GPIO pins
const int output4 = 4;

bool hasTimerBeenSet = false;
DateTime Now(2022, 10, 10, 12,0,0);
DateTime timer(2022, 10, 10, 12, 0, 0);

WebServer server(80);

void LedOn()
{    
    digitalWrite(output4, HIGH);
    server.send(200, "text/html", "led on");   
}

void LedOff()
{    
    digitalWrite(output4, LOW);
    server.send(200, "text/html", "led off");
}

void OnConnect()
{
    Serial.println("connected");
    server.send(200, "text/html", "base url");
}

void NotFound()
{
    server.send(404, "text/html", "not found");
}

int year, month, day, h, m, s;

void GetDateTime()
{
    String params;
    server.send(200, "text/html", server.hostHeader());
    if (server.args() > 0)
    {
        for (size_t i = 0; i < server.args(); i++)
        {
            Serial.println(server.argName(i));
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
       
    timer.UpdateTime(year, month, day, h, m, s);    
    hasTimerBeenSet = true;
    Serial.println(timer.minutes);
    Serial.println(Now.minutes);
    Serial.println(hasTimerBeenSet);
    Serial.println(DateTime::IsToday(&timer, &Now));
}

int currYear, currMonth, monthDay, currentHour, currentMinute, currentSeconds;
struct tm* ptm;

void UpdateDateTime()
{
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

WiFiManager wMan;
String ssid, pw;

void UseJson()
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
    Serial.println(jResult);
    HTTPClient client;
    bool b = client.begin(azureServerBaseUrl);
    client.addHeader("accept", "text/plain");
    client.addHeader("Content-Type", "application/json");
    int httpCode = client.POST(jResult);
    Serial.println(client.errorToString(httpCode));
    Serial.println(b);
    Serial.println(httpCode);
    Serial.println(WiFi.localIP().toString());
    client.end();
}

//http://localhost:7266/Devices?ssid=testName&pw=testPw&ip=someIp
void UseParameters()
{
    String apiRequest;
    apiRequest.concat("?ssid=");
    apiRequest.concat(ssid);
    apiRequest.concat("&pw=");
    apiRequest.concat(pw);
    apiRequest.concat("&ip=");
    apiRequest.concat(WiFi.localIP());
    HTTPClient client;
    client.begin(azureServerBaseUrl);
    client.addHeader("Content-type", "text/plain");
    int httpCode = client.POST(apiRequest);
    Serial.println(httpCode);
    Serial.println(apiRequest);
    client.end();
}

void setup() {
   
    //9600 for bt
    Serial.begin(115200);  
    //WiFi.mode(WIFI_STA); 
    //wMan.resetSettings();
    bool res = wMan.autoConnect("AutoConnectAP", "password");

    
    if (!res) {
        Serial.println("Failed to connect");
        timeClient.setTimeOffset(7200);
        // ESP.restart();
    }
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");   
        
    }
      
    

    
    server.on("/", OnConnect);
    server.on("/ledOn", LedOn);
    server.on("/ledOff", LedOff);
    server.on("/dateTime", GetDateTime);   
    server.onNotFound(NotFound);   
    server.begin(); 
    
}

bool success = false;


//serial.flush()
void loop() {
    timeClient.update();   
    UpdateDateTime();
    server.handleClient();  


    ssid = wMan.getWiFiSSID();
    pw = wMan.getWiFiPass();

    UseJson();

    delay(5000);


    if (!hasTimerBeenSet) return;
    if (!DateTime::IsToday(&timer, &Now)) return;
    if (DateTime::CompareMinutes(&Now, &timer))
    {
        digitalWrite(output4, LOW);
        Serial.println("passed");
        hasTimerBeenSet = false;
    }
}

