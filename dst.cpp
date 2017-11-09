//version 20171105
#include "dst.h"
//#define _DEBUG // prints debug lines
#ifdef _DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif
// Converts a unix time stamp to a strDateTime structure
// see comment by captncraig http://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date
strDateTime ConvertUnixTimestamp( unsigned long _tempTimeStamp) {
  strDateTime _tempDateTime;
  uint8_t _year, _month, _monthLength;
  uint32_t _time;
  unsigned long _days;
  static const uint8_t _monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  _time = (uint32_t)_tempTimeStamp;
  _tempDateTime.second = _time % 60;
  _time /= 60; // now it is minutes
  _tempDateTime.minute = _time % 60;
  _time /= 60; // now it is hours
  _tempDateTime.hour = _time % 24;
  _time /= 24; // now it is _days
  _tempDateTime.dayofWeek = ((_time + 4) % 7) + 1;  // Sunday is day 1
 DEBUG_PRINT("dst.cpp _tempDateTime.dayofWeek= "); DEBUG_PRINT(_tempDateTime.dayofWeek);
  _year = 0;
  _days = 0;
  while ((unsigned)(_days += (LEAP_YEAR(_year) ? 366 : 365)) <= _time) {
    _year++;
  }
  _tempDateTime.year = _year; // year is offset from 1970

  _days -= LEAP_YEAR(_year) ? 366 : 365;
  _time  -= _days; // now it is days in this year, starting at 0

  _days = 0;
  _month = 0;
  _monthLength = 0;
  for (_month = 0; _month < 12; _month++) {
    if (_month == 1) { // february
      if (LEAP_YEAR(_year)) {
        _monthLength = 29;
      } else {
        _monthLength = 28;
      }
    } else {
      _monthLength = _monthDays[_month];
    }

    if (_time >= _monthLength) {
      _time -= _monthLength;
    } else {
      break;
    }
  }
  _tempDateTime.month = _month + 1;  // jan is month 1
  _tempDateTime.day = _time + 1;     // day of month
  _tempDateTime.year += 1970;

  return _tempDateTime;
} // end // strDateTime ConvertUnixTimestamp( unsigned long _tempTimeStamp) 
  
//  --------------------------------------------------------------------

boolean daylightSavingTime(unsigned long _timeStamp) {

  strDateTime  _tempDateTime;
  _tempDateTime = ConvertUnixTimestamp(_timeStamp);

// here the US code
  //return false;
  // see http://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date
  // since 2007 DST begins on second Sunday of March and ends on first Sunday of November. 
  // Time change occurs at 2AM locally
  if (_tempDateTime.month < 3 || _tempDateTime.month > 11) return false;  //January, february, and december are out.
  if (_tempDateTime.month > 3 && _tempDateTime.month < 11) return true;   //April to October are in
  int previousSunday = _tempDateTime.day - (_tempDateTime.dayofWeek - 1);  // dow Sunday input was 1,
    // need it to be Sunday = 0. If 1st of month = Sunday, previousSunday=1-0=1
  if (previousSunday <1) previousSunday=0;  // in first week ofmonth, it is possible for previousSunday to be 0 
  // or negative. If first of month is Monday (2), previousSunday = 1 [date]  - 2 [dow] = -1
  //int previousSunday = day - (dow-1);
  // -------------------- March ---------------------------------------
  //In march, we are DST if our previous Sunday was = to or after the 8th.
  if (_tempDateTime.month == 3 ) {  // in march, if previous Sunday is after the 8th, is DST
          // unless Sunday and hour < 2am
    if ( previousSunday >= 8 ) { // Sunday = 1
      // return true if day > 14 or (dow == 1 and hour >= 2)
      return ((_tempDateTime.day > 14) || 
        ((_tempDateTime.dayofWeek == 1 && _tempDateTime.hour >= 2) || _tempDateTime.dayofWeek > 1));
    } // end if ( previousSunday >= 8 && _dateTime.dayofWeek > 0 )
    else
    {
      // previousSunday has to be < 8 to get here
      //return (previousSunday < 8 && (_tempDateTime.dayofWeek - 1) = 0 && _tempDateTime.hour >= 2)
      return false;
    } // end else
  } // end if (_tempDateTime.month == 3 )

// ------------------------------- November -------------------------------

  // gets here only if month = November
  //In november we must be on or after the first Sunday to be dst.
        //That means the first Sunday must be between 1 and 7th.
        DEBUG_PRINT(" previousSunday=");DEBUG_PRINT(previousSunday);

  if (previousSunday >0 && previousSunday < 8) // eliminates previousSunday before the first
    {  // Sunday is dow = 1
      // is not true for Sunday after 2am or any day after 1st Sunday any time
      if ((_tempDateTime.dayofWeek == 1 && _tempDateTime.hour < 2))
      {
        DEBUG_PRINT("Gets here if November && previousSunday between 1-7 and dow = 1 and time < 2");
        return true;
      }// end if ((_tempDateTime.dayofWeek == 1 && _tempDateTime.hour < 2))
    } // end  if (previousSunday >0 && previousSunday < 8) // eliminates previousSunday before the first    
    // gets here if november, and previousSunday = 0 or greater than 7
    return (previousSunday==0);//false unless prev sunday == 0;
} // end boolean NTPtime::daylightSavingTime(unsigned long _timeStamp)

//  --------------------------------------------------------------------
