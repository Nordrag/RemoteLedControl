#pragma once
#include "Statemachine.h"
#include "DateTime.h"

extern double deltaTime;
extern bool isPumpOn, hasTimerBeenSet, wasRequestBeforeTimer;
extern DateTime Now, timer;
extern const int pumpOutput;

extern void GetNextTimer();

class ManualState : public State
{
public:
	ManualState();
	~ManualState();
	void OnEnter();
	void Update();
	void OnExit();

private:
};

ManualState::ManualState() { }
ManualState::~ManualState() { }

inline void ManualState::OnEnter()
{
	Serial.println("entered manual state");
}

inline void ManualState::Update()
{
}

inline void ManualState::OnExit()
{
	Serial.println("entered left state");
}
//end of manual state

class TimerState : public State
{
public:
	TimerState(float* WorkTime);
	~TimerState();
	void OnEnter();
	void Update();
	void OnExit();
private:
	float workTime;
};

TimerState::TimerState(float* WorkTime) { workTime = *WorkTime; }
TimerState::~TimerState() { }

inline void TimerState::OnEnter()
{
	if (workTime > 0)
	{
		isPumpOn = true;
	}
	Serial.println(workTime);
	Serial.println("entered timer state");
}

inline void TimerState::Update()
{
	workTime -= deltaTime;
	
	if (DateTime::CompareTime(&Now, &timer))
	{
		if (!wasRequestBeforeTimer)
		{
			workTime -= deltaTime;
			isPumpOn = true;
			digitalWrite(pumpOutput, LOW);
			if (workTime <= 0)
			{
				isPumpOn = false;
				hasTimerBeenSet = false;
				GetNextTimer();
				digitalWrite(pumpOutput, HIGH);
			}
		}
	}
}

inline void TimerState::OnExit()
{
	isPumpOn = false;
	Serial.println("left timer state state");
	Serial.println(workTime);
}

class StateOne : public State
{
public:
	StateOne() { }
	~StateOne() { }

	void OnEnter() override
	{
		Serial.println("entered state one");
	}
	void OnExit() override
	{
		Serial.println("left state one");
	}
	void Update() override
	{
	}
};


class StateTwo : public State
{
public:
	StateTwo() { }
	~StateTwo() { }

	void OnEnter()
	{
		Serial.println("entered state two");
	}
	void OnExit()
	{
		Serial.println("left state two");
	}
	void Update()
	{
		
	}
};
