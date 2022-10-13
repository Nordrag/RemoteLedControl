#pragma once
#include "Delegate.h"

class ApiCommand
{
public:
	ApiCommand();
	~ApiCommand();
	String commandName;
	Action command;
	bool* condition;	

private:
	
};

class EntryCondition
{
public:
	EntryCondition(bool c, bool* ptr);
	~EntryCondition();	
	bool Condition;
	bool* conditionPtr;

private:

};

EntryCondition::EntryCondition(bool c, bool* ptr)
{
	Condition = c;
	conditionPtr = ptr;
}

EntryCondition::~EntryCondition()
{
}

ApiCommand::ApiCommand()
{
	
}

ApiCommand::~ApiCommand()
{
}

int commandCount;
ApiCommand commands[5];

void CreateCommands(int size)
{
	commandCount = size;
	commands[commandCount];
}

void AddCommand(String commandName, Action commandToAdd)
{
	for (size_t i = 0; i < commandCount; i++)
	{
		if (commands[i].command == NULL)
		{
			commands[i].commandName = commandName;
			commands[i].command = commandToAdd;
			return;
		}
	}
}

void AddCommand(String commandName, Action commandToAdd, bool* entryCondition)
{
	for (size_t i = 0; i < commandCount; i++)
	{
		if (commands[i].command == NULL)
		{
			commands[i].commandName = commandName;
			commands[i].command = commandToAdd;
			commands[i].condition = entryCondition;
			return;
		}
	}
}

void InvokeCommand(String name)
{
	for (size_t i = 0; i < commandCount; i++)
	{
		if (commands[i].commandName == name)
		{
			Invoke(commands[i].command);
			return;
		}
	}
}

void InvokeCommand(int index)
{
	Invoke(commands[index].command);
}

void Tick()
{
	for (size_t i = 0; i < commandCount; i++)
	{		
		if (*commands[i].condition == true)
		{
			Invoke(commands[i].command);
		}
	}
}

bool GetApiEndPoint(String route,String header)
{		
	return header.indexOf(route) >= 0;
}