#pragma once

class DateTime
{
public:

	int year, month, day, hours, minutes, seconds;

	DateTime(int Year, int Month, int Day, int Hours, int Minutes, int Seconds);
	~DateTime();
	static bool IsToday(DateTime* lhs, DateTime* rhs);
	static bool CompareDayTime(DateTime* lhs, DateTime* rhs);
	int* GetCurrentHour();
	int* GetCurrentMinutes();
	void UpdateTime(int* Year, int* Month, int* Day, int* Hours, int* Minutes, int* Seconds);
	void UpdateTime(int Year, int Month, int Day, int Hours, int Minutes, int Seconds);

private:

};

DateTime::DateTime(int Year, int Month, int Day, int Hours, int Minutes, int Seconds)
{
	year = Year;
	month = Month;
	day = Day;
	hours = Hours;
	minutes = Minutes;
	seconds = Seconds;
}

DateTime::~DateTime()
{
}

inline bool DateTime::IsToday(DateTime* lhs, DateTime* rhs)
{
	if (lhs->day != rhs->day)
	{
		return false;
	}
	if (lhs->month != rhs->month)
	{
		return false;
	}
	if (lhs->year != rhs->year)
	{
		return false;
	}
	return true;
}

inline bool DateTime::CompareDayTime(DateTime* lhs, DateTime* rhs)
{
	
	if (lhs->hours < rhs->hours)
	{
		return false;
	}
	if (lhs->minutes >= rhs->minutes)
	{
		return true;
	}
	return false;
}

inline int* DateTime::GetCurrentHour()
{
	return &hours;
}

inline int* DateTime::GetCurrentMinutes()
{
	return &minutes;
}



inline void DateTime::UpdateTime(int* Year, int* Month, int* Day, int* Hours, int* Minutes, int* Seconds)
{
	year = *Year;
	month = *Month;
	day = *Day;
	hours = *Hours;
	minutes = *Minutes;
	seconds = *Seconds;
}

inline void DateTime::UpdateTime(int Year, int Month, int Day, int Hours, int Minutes, int Seconds)
{
	year = Year;
	month = Month;
	day = Day;
	hours = Hours;
	minutes = Minutes;
	seconds = Seconds;
}

class Time
{
public:
	Time();
	~Time();
	void Tick();
	unsigned long GetDeltaTime();

private:
	unsigned long current, old, deltaTime;
};

Time::Time()
{
}

Time::~Time()
{
}

inline void Time::Tick()
{
	old = current;
	current = millis();
	deltaTime = current - old;
}

inline unsigned long Time::GetDeltaTime()
{
	return deltaTime;
}

