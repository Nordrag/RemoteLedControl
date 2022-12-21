#pragma once
#include <vector>
#include <map>
#include "State.h"

class Transition
{
public:
	Transition(int to, bool* condition);
	~Transition() { }
	bool* Condition;
	int To;
};


class StateMachine
{
public:
	StateMachine() { }
	~StateMachine() { }
	void Update(); 
	void SetState(int state); 
	void AddTransition(int from, int to, bool* predicate);
	void AddAnyTransiton(int to, bool* predicate); 
	void AddState(State* newState); 
private:
	int currTransitionIndex = -1; 
	Transition* GetTransition();
	State* currentState;
	std::map<int, std::vector<Transition>> transitions; 
	std::vector<Transition> anyTransitions; 
	std::vector<Transition> currTransitions; 
	std::vector<State*> states; 
};

inline void StateMachine::Update()
{
	Transition* transition = GetTransition(); 
	if (transition != nullptr)
	{
		SetState(transition->To);
	}
	currentState->Update();
}

inline void StateMachine::SetState(int state)
{
	
	if (currTransitionIndex == state)
	{
		return;
	}
	if (currentState != nullptr)
	{
		currentState->OnExit(); 
	}
	currentState = states[state];
	currTransitions = transitions[state];
	currentState->OnEnter();

}

inline void StateMachine::AddTransition(int from, int to, bool* predicate)
{
	transitions[from].push_back(Transition(to, predicate));
}

inline void StateMachine::AddAnyTransiton(int to, bool* predicate)
{
	anyTransitions.push_back(Transition(to, predicate));
}

inline void StateMachine::AddState(State* newState)
{
	states.push_back(newState);
}

inline Transition* StateMachine::GetTransition()
{
	
	for (Transition at : anyTransitions)
	{
		if (*at.Condition)
		{
			return &at;
		}
	}
	for (Transition ct : currTransitions)
	{
		if (*ct.Condition == true)
		{
			return &ct;
		}
	}
	return nullptr;
}

inline Transition::Transition(int to, bool* condition)
{
	To = to;
	Condition = condition;
}
