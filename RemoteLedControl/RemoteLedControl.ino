#include <Uri.h>
#include <ESP8266WebServerSecure.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "WifiConnector.h"
#include "ArduinoJson.h"
#include "DateTime.h"

//serial.print sends bl data

StaticJsonDocument<128> doc;
const char* input;
DeserializationError err;

// Replace with your network credentials
const char* ssid = "DIGI-e9gF";
const char* password = "8MPyJHjv";

String weekDays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

String months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org");

// Assign output variables to GPIO pins
const int output4 = 4;

bool hasTimerBeenSet = false;
DateTime Now(2022, 10, 10, 12,0,0);
DateTime timer(2022, 10, 10, 12, 0, 0);

ESP8266WebServer server(80);

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

void setup() {
   
    //9600 for bt
    Serial.begin(115200);
    // Initialize the output variables as outputs
    timeClient.setTimeOffset(7200);
     
    pinMode(output4, OUTPUT);
    digitalWrite(output4, HIGH);
       
    WifiConnector connector;
    //connector.ResetSettings();
    connector.TryConnect("AutoConnectAP", "password"); // password protected ap

    if (!connector.GetConnected()) {
        Serial.println("Failed to connect");
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

//serial.flush()
void loop() {
    timeClient.update();   
    UpdateDateTime();
    server.handleClient();


    if (!hasTimerBeenSet) return;
    if (!DateTime::IsToday(&timer, &Now)) return;
    if (DateTime::CompareMinutes(&Now, &timer))
    {
        digitalWrite(output4, LOW);
        Serial.println("passed");
        hasTimerBeenSet = false;
    }
}

