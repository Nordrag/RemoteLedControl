#pragma once
#include <WiFiManager.h>

#ifndef _WIFICONNECTOR_h
#define _WIFICONNECTOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


#endif

class WifiConnector
{
public:
	WifiConnector();
	~WifiConnector();
	bool TryConnect();
	bool TryConnect(const char* apName);
	bool TryConnect(const char* apName, const char* pw);
	void SetConfigPortal(unsigned long timeout);
	bool StartConfigPortal(const char* name);
	bool GetConnected() const;
	void ResetSettings();
	String GetSSID();
	String GetPassword();

private:
	WiFiManager wManager;
	bool res;
};

WifiConnector::WifiConnector()
{
	WiFi.mode(WIFI_STA);
}

WifiConnector::~WifiConnector()
{
}

/// <summary>
/// creates a connection without password, and the hardware name as connection name
/// </summary>
/// <returns></returns>
inline bool WifiConnector::TryConnect()
{
	return wManager.autoConnect();
}

/// <summary>
/// creates a connection with a name and no pw
/// </summary>
/// <param name="apName"></param>
/// <returns></returns>
inline bool WifiConnector::TryConnect(const char* apName)
{
	return wManager.autoConnect(apName);
}

/// <summary>
/// creates a connection with a name and a password
/// </summary>
/// <param name="apName"></param>
/// <param name="pw"></param>
/// <returns></returns>
inline bool WifiConnector::TryConnect(const char* apName, const char* pw)
{
	return wManager.autoConnect(apName, pw);
}

inline void WifiConnector::SetConfigPortal(unsigned long timeout)
{
	wManager.setConfigPortalTimeout(timeout);
}

inline bool WifiConnector::StartConfigPortal(const char* name)
{
	return wManager.startConfigPortal(name);
}

/// <summary>
/// returns if we are connected to the wifi
/// </summary>
/// <returns></returns>
inline bool WifiConnector::GetConnected() const
{
	return res;
}

/// <summary>
/// should only be used in a debug environment
/// </summary>
inline void WifiConnector::ResetSettings()
{
	wManager.resetSettings();
}

inline String WifiConnector::GetSSID()
{
	return wManager.getWiFiSSID();
}

inline String WifiConnector::GetPassword()
{
	return wManager.getWiFiPass();
}



