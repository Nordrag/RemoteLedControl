#pragma once
#include "State.h"
#include <vector>
#include "LocalTimer.h"
#define LOG(X) Serial.println(X)

extern void SendMeasuresToServer();
extern void SendStatusToServer(bool pump, bool light);
extern int GetStatusFromServer();
extern bool PumpOn, LightOn, IsPumpManual, IsLightManual;
extern bool secondsTicked;
extern const int pumpOutput;
extern const int lightOutput;
extern int workTimeDelta;
extern int lightTimerDelta;
extern bool pShouldTick;

extern bool inManualState;
extern bool inTimerState;
extern bool inOverrideState;

extern const int heatPumpInput;

extern std::vector<Day> timers;
extern bool isLocalManual; 
extern long localSeconds;
extern int localDay;

int timerIndex;
bool timerStartPrinted, timerEndPrinted;

void CalculateTimers()
{
	for (size_t i = 0; i < timers[localDay].timers.size(); i++)
	{
		if (timers[localDay].timers[i].from <= localSeconds && timers[localDay].timers[i].to >= localSeconds)
		{
			timerIndex = i;
		}
	}
	if (timers[localDay].timers.size() == 0 || timerIndex >= timers[localDay].timers.size()) return;
	if (timers[localDay].timers[timerIndex].from <= localSeconds && timers[localDay].timers[timerIndex].to >= localSeconds)
	{
		if (!timerStartPrinted)
		{
			LOG("timer on");
			timerStartPrinted = true;
			timerEndPrinted = false;
		}
		digitalWrite(pumpOutput, LOW);
	}
	else
	{
		if (!timerEndPrinted)
		{
			LOG("timer off");
			timerEndPrinted = true;
			timerStartPrinted = false;
		}
		digitalWrite(pumpOutput, HIGH);
	}
}

class ManualState : public State
{
public:
	ManualState() { }
	~ManualState() { }

	void OnEnter() override
	{
		GetStatusFromServer();
		LOG("manual state");
		LOG(PumpOn);
	}
	void OnExit() override
	{
		SendStatusToServer(PumpOn, LightOn);
	}
	void Update() override
	{		
		digitalWrite(pumpOutput, PumpOn ? LOW : HIGH);
		digitalWrite(lightOutput, LightOn ? LOW : HIGH);
	}
};

class TimerState : public State
{
public:
	TimerState() { }
	~TimerState() { }

	void OnEnter() override
	{
		LOG("timer state");
		SendStatusToServer(PumpOn, LightOn);
	}

	void OnExit() override
	{
		LOG("left timer state");
		SendStatusToServer(PumpOn, LightOn);
	}

	void Update() override
	{
		digitalWrite(lightOutput, LightOn ? LOW : HIGH);
		//if (secondsTicked)
		//{		
		//	if (workTimeDelta > 0)
		//	{
		//		//workTimeDelta--;
		//		PumpOn = true;
		//		digitalWrite(pumpOutput, LOW);
		//	}
		//	else
		//	{
		//		digitalWrite(pumpOutput, HIGH);
		//		PumpOn = false;
		//	}			
		//}	
		CalculateTimers();
	}
};

class OverrideState : public State
{
public:
	OverrideState() { }
	~OverrideState() { }

	void OnEnter() override
	{
		LOG("override state");
		PumpOn = true;
		SendStatusToServer(PumpOn, LightOn);
	}

	void OnExit() override
	{
		LOG("left override state");	
		PumpOn = false;
		digitalWrite(pumpOutput, HIGH);
		SendStatusToServer(PumpOn, LightOn);
	}

	void Update() override
	{		
		digitalWrite(pumpOutput, LOW);
	}
};

class LocalTimerState : public State
{

private:
	Timer currentTimer;

public:
	LocalTimerState() {};
	~LocalTimerState() {};

	void OnEnter() override
	{
		LOG("local state");
	}

	void OnExit() override
	{
		LOG("left local state");
		
	}

	void Update() override
	{
		if (!isLocalManual)
		{
			CalculateTimers();
		}
		else
		{
			digitalWrite(lightOutput, LightOn ? LOW : HIGH);
			digitalWrite(pumpOutput, PumpOn ? LOW : HIGH);
		}
	}
};

