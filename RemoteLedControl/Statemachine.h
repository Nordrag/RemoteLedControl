#pragma once
#include <vector>
#include <map>

//base class that acts as an interface
class State
{
public:
	State() { }
	~State() { }
	//called once when the machine sets this state to be active
	virtual void OnEnter()
	{
		
	}
	//called once when the machine sets this state to be inactive
	virtual void OnExit()
	{
		
	}
	//called every time the machine updates
	virtual void Update()
	{

	}
};

//class that represents a logical connection between states
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
	void Update(); //call it in a while(true) loop
	void SetState(int state); //attempts to set the currently active state
	void AddTransition(int from, int to, bool* predicate); //adds a transition based on indexes
	void AddAnyTransiton(int to, bool* predicate); //any transition does not care about what state is active
	void AddState(State* newState); //adds a state to the statemachine pointer array
private:
	int currTransitionIndex = -1; //helper to prevent entering the same state
	Transition* GetTransition();
	State* currentState;
	std::map<int, std::vector<Transition>> transitions; //all the possible transitions
	std::vector<Transition> anyTransitions; //transition that dont care about the current state
	std::vector<Transition> currTransitions; //transitions that belong to the current state
	std::vector<State*> states; //all possible states
};

inline void StateMachine::Update()
{
	Transition* transition = GetTransition(); //try to get the first transition that matches the entry condition
	if (transition != nullptr)
	{
		SetState(transition->To);
	}
	currentState->Update();
}

inline void StateMachine::SetState(int state)
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
	//need to check any transitions first, they have priority
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
