#pragma once
#include <vector>
#include <map>
#include "State.h"


//class that represents a logical connection between states
class Transition
{
public:
	Transition(unsigned int to, bool* condition);
	Transition() { predicate = nullptr; }
	~Transition() { }
	bool* predicate;
	unsigned int To;
};


class StateMachine
{
public:
	StateMachine() { }
	~StateMachine() { }
	void Update(); //call it in a while(true) loop
	void SetState(unsigned int state); //attempts to set the currently active state
	void AddTransition(unsigned int from, unsigned int to, bool* predicate); //adds a transition based on indexes
	void AddAnyTransiton(unsigned int to, bool* predicate); //any transition does not care about what state is active
	unsigned int AddState(State* newState); //adds a state to the statemachine pointer array
private:
	int currTransitionIndex = -1; //helper to prevent entering the same state
	Transition GetTransition();
	State* currentState;
	std::map<unsigned int, std::vector<Transition>> transitions; //all the possible transitions
	std::vector<Transition> anyTransitions; //transition that dont care about the current state
	std::vector<Transition> currTransitions; //transitions that belong to the current state
	std::vector<State*> states; //all possible states

	
};

inline void StateMachine::Update()
{
	Transition transition = GetTransition(); //try to get the first transition that matches the entry condition
	//Serial.println(transition->To);

	if (transition.predicate != nullptr)
	{		
		SetState(transition.To);
	}
	currentState->Update();
}

inline void StateMachine::SetState(unsigned int state)
{
	//dont enter the same state
	if (currTransitionIndex == state)
	{
		return;
	}
	if (currentState != nullptr)
	{
		currentState->OnExit(); //exit the current state
	}
	currentState = states[state];
	currTransitionIndex = state;
	currTransitions = transitions[state];
	currentState->OnEnter();
}

inline void StateMachine::AddTransition(unsigned int from, unsigned int to, bool* predicate)
{
	transitions[from].push_back(Transition(to, predicate));
}

inline void StateMachine::AddAnyTransiton(unsigned int to, bool* predicate)
{
	anyTransitions.push_back(Transition(to, predicate));
}

inline unsigned int StateMachine::AddState(State* newState)
{
	states.push_back(newState);
	return states.size() - 1;
}

inline Transition StateMachine::GetTransition()
{
	Transition nextTransition;
	//need to check any transitions first, they have priority
	for (Transition at : anyTransitions)
	{
		if (*at.predicate == 1)
		{			
			nextTransition = at;
			return nextTransition;
		}
	}
	for (Transition ct : currTransitions)
	{		
		if (*ct.predicate == 1)
		{					
			nextTransition = ct;
			return nextTransition;
		}
	}
	return nextTransition;	
}

inline Transition::Transition(unsigned int to, bool* condition)
{
	To = to;
	predicate = condition;
}