#pragma once
typedef void(*Action)();
typedef bool(*Condition)();

void Invoke(Action e)
{
	e();
}

bool Invoke(Condition c)
{
	return c();
}