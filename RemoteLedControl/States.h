#pragma once
#include "State.h"
#include "DateTime.h"

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
}

inline void ManualState::Update()
{
}

inline void ManualState::OnExit()
{
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
}

inline void TimerState::Update()
{
	workTime -= deltaTime;
	
	if (DateTime::CompareTime(&Now, &timer))
	{
		if (!wasRequestBeforeTimer)
		{
			workTimeDelta -= deltaTime;
			isPumpOn = true;
			digitalWrite(pumpOutput, LOW);
			if (workTimeDelta <= 0)
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
}
