#include <Arduino.h>
/*
 * By the Energy Policy Act of 2005, daylight saving time (DST) was extended in the United 
 * States beginning in 2007. As from that year, DST begins on the second Sunday of March (8-14) 
 * and ends on the first Sunday of November (1-7).
 */
struct strDateTime // declared in NTPtimeESP.h, which is included by including NTPtimeESP.cpp above
{
  byte hour;
  byte minute;
  byte second;
  int year;
  byte month;
  byte day;
  byte dayofWeek;// sunday = 1
  boolean valid;
};
#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
strDateTime ConvertUnixTimestamp( unsigned long );
boolean daylightSavingTime(unsigned long );
