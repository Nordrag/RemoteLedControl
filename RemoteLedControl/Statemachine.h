#pragma once

class State
{
public:
	State();
	~State();
	virtual void OnEnter() = 0;
	virtual void OnExit() = 0;
	virtual void Update() = 0;
	static State* currstate;
	static State* manualState, *firstTimerState, *recurringTimerState;
};

State::State()
{
}

State::~State()
{
}

class ManualState : public State
{
public:
	ManualState();
	~ManualState();
	void OnEnter();
	void OnExit();
	void Update();
private:

};

ManualState::ManualState() {

}

ManualState::~ManualState() {

}

inline void ManualState::OnEnter()
{
}

inline void ManualState::OnExit()
{
}

inline void ManualState::Update()
{
}
