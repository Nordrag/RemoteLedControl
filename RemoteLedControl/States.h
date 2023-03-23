#pragma once
#include "State.h"
#define LOG(X) Serial.println(X)

extern void SendMeasuresToServer();
extern void SendStatusToServer();
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

class ManualState : public State
{
public:
	ManualState() { }
	~ManualState() { }

	void OnEnter() override
	{
		LOG("manual state");
	}
	void OnExit() override
	{
		
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
		SendStatusToServer();
	}

	void OnExit() override
	{
		SendStatusToServer();
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
	}

	void OnExit() override
	{
		SendStatusToServer();
	}

	void Update() override
	{		
		digitalWrite(pumpOutput, LOW);
	}
};



