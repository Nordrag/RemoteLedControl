#pragma once
#include "Statemachine.h"
#include "DateTime.h"


extern bool isPumpOn, hasTimerBeenSet, wasRequestBeforeTimer;
extern DateTime Now, timer;
extern const int pumpOutput;

extern void GetNextTimer();

class IdleState : public State
{
public:
	IdleState();
	~IdleState();
	void OnEnter();
	void Update();
	void OnExit();

private:
};

IdleState::IdleState() { }
IdleState::~IdleState() { }

inline void IdleState::OnEnter()
{
	Serial.println("entered manual state");
}

inline void IdleState::Update()
{
}

inline void IdleState::OnExit()
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
