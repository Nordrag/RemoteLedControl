#pragma once


String GetDefaultStyle()
{
	String style = "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
	style += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
	style += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
	style += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
	style += ".button-on {background-color: #3498db;}\n";
	style += ".button-on:active {background-color: #2980b9;}\n";
	style += ".button-off {background-color: #34495e;}\n";
	style += ".button-off:active {background-color: #2c3e50;}\n";
}

String GetDefaultBody(bool pumpOn, bool lightOn, bool isManual, String extraContent = "")
{
	String body = "<h1>ESP32 Web Server</h1>\n";
	body += extraContent;

	if (isManual)
	{
		if (pumpOn)
		{
			body += "<p>Pump Status: ON</p><a class=\"button button-off\" href=\"/tPump\">OFF</a>\n";
		}
		else
		{
			body += "<p>Pump Status: OFF</p><a class=\"button button-on\" href=\"/tPump\">ON</a>\n";
		}

		if (lightOn)
		{
			body += "<p>Light Status: ON</p><a class=\"button button-off\" href=\"/tLight\">OFF</a>\n";
		}
		else
		{
			body += "<p>Light Status: OFF</p><a class=\"button button-on\" href=\"/tLight\">ON</a>\n";
		}

		body += "<p>Manual : ON< / p> < a class = \"button button-on\" href=\"/tManual\">ON</a>\n";
	}
	else
	{
		body += "<p>Manual : OFF< / p> < a class = \"button button-on\" href=\"/tManual\">OFF</a>\n";
	}
	

	body += "<p>Sync with server< / p> < a class = \"button button-on\" href=\"/getTimers\">ON</a>\n";

	return body;
}

String CreateSite(String style, String body)
{
	String site = "<!DOCTYPE html> <html>\n";
	site += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
	site += "<title>Pool Controller</title>\n";
	site += style;
	site += "</head>\n";
	site += body;
	site += "</html>\n";

	return site;
}