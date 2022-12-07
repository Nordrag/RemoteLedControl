#pragma once
#include <vector>
#include <map>
//#include "Delegate.h"

typedef bool(*Condition)();

class State
{
public:
	State();
	~State();
	virtual void OnEnter();
	virtual void OnExit();
	virtual void Update();
};

class Transition
{
public:
	Transition(State* to, bool condition);
	~Transition();

private:
	State From, To;
	bool Condition;
};

class StateMachine
{
public:
	StateMachine();
	~StateMachine();
	void Update();
	void SetState(State* state);
	void AddTransition(State* from, State* to, Condition predicate);
	void AddAnyTransiton(State* to, Condition predicate);

private:

	Transition* GetTransition();

	State currentState;
	std::map<int, std::vector<Transition>> transitions;
	std::vector<Transition> anyTransitions;
	std::vector<Transition> currTransitions;
	std::vector<Transition> emptyTransitions;
};

StateMachine::StateMachine()
{
	
}

StateMachine::~StateMachine()
{
}

inline void StateMachine::Update()
{
	currentState.Update();
}

inline void StateMachine::SetState(State* state)
{
	currentState.OnExit();
	currentState = *state;
	//get from map .enter
}

inline void StateMachine::AddAnyTransiton(State* to, Condition predicate)
{		
	anyTransitions.push_back(Transition(to, predicate));
}

inline Transition::Transition(State* to, bool condition)
{
	To = *to;
	Condition = condition;
}


Transition::~Transition()
{
}

State::State()
{
}

State::~State()
{
}

inline void State::OnEnter()
{
}

inline void State::OnExit()
{
}

inline void State::Update()
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
