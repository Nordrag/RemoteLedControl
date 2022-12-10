#pragma once

class DateTime
{
public:

	int year, month, day, hours, minutes, seconds;

	DateTime(int Year, int Month, int Day, int Hours, int Minutes, int Seconds);
	~DateTime();
	static bool IsToday(DateTime* now, DateTime* timer);
	static bool CompareDayTime(DateTime* now, DateTime* timer);
	static bool CompareTime(DateTime* now, DateTime* timer);
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

inline bool DateTime::IsToday(DateTime* now, DateTime* timer)
{	
	bool dayCheck = now->day == timer->day;
	bool monthCheck = now->month == timer->month;
	bool yearCheck = now->year == timer->year;	
	return dayCheck && monthCheck && yearCheck;
}

inline bool DateTime::CompareDayTime(DateTime* now, DateTime* timer)
{
	bool hourCheck = now->hours == timer->hours;
	bool minuteCheck = now->minutes >= timer->minutes;	
	return hourCheck && minuteCheck;
}

inline bool DateTime::CompareTime(DateTime* now, DateTime* timer)
{
	bool dayCheck = now->day == timer->day;
	bool monthCheck = now->month == timer->month;
	bool yearCheck = now->year == timer->year;
	bool hourCheck = now->hours == timer->hours;
	bool minuteCheck = now->minutes >= timer->minutes;
	return dayCheck && monthCheck && yearCheck && hourCheck && minuteCheck;
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


