#pragma once
#include "State.h"
#define LOG(X) Serial.println(X)

extern void SendMeasuresToServer();
extern void SendStatusToServer(bool pump, bool light);
extern void GetStatusFromServer();
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

class ManualState : public State
{
public:
	ManualState() { }
	~ManualState() { }

	void OnEnter() override
	{
		GetStatusFromServer();
		LOG("manual state");
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
		if (secondsTicked)
		{		

			if (workTimeDelta > 0 && pShouldTick)
			{
				workTimeDelta--;
				PumpOn = true;
				digitalWrite(pumpOutput, LOW);
			}
			else
			{
				digitalWrite(pumpOutput, HIGH);
				PumpOn = false;
			}
		}		
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



