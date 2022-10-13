#pragma once

class DateTime
{
public:

	int year, month, day, hours, minutes, seconds;

	DateTime(int Year, int Month, int Day, int Hours, int Minutes, int Seconds);
	~DateTime();
	static bool IsToday(DateTime* lhs, DateTime* rhs);
	static bool CompareMinutes(DateTime* lhs, DateTime* rhs);
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

inline bool DateTime::CompareMinutes(DateTime* lhs, DateTime* rhs)
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
