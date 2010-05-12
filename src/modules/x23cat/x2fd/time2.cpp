#include "StdAfx.h"
#include "time2.h"

time_t TimeStampToLocalTimeStamp(const time_t *time)
{
	static const __int64 DELTA=116444736000000000i64;

	FILETIME wintime, localwin;
	ULARGE_INTEGER i;
	i.QuadPart=*time * 10000000i64 + DELTA; // convert to nanoseconds and add the difference
	
	wintime.dwHighDateTime=i.HighPart;
	wintime.dwLowDateTime=i.LowPart;

	FileTimeToLocalFileTime(&wintime, &localwin);

	i.HighPart=localwin.dwHighDateTime;
	i.LowPart=localwin.dwLowDateTime;
	
	return (time_t)((i.QuadPart - DELTA) / 10000000i64); // convert to seconds
}