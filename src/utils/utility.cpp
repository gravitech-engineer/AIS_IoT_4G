#include <Arduino.h>
#include <time.h>
#include "./utility.h"

    tm utility::convertUnix(unsigned long unix, int timeZone)
    {
        time_t timestamp = unix + (timeZone * 60 * 60);
        tm *local_time = gmtime(&timestamp);
        return *local_time;
    }

    String utility::toDateTimeString(unsigned long unixtTime, int timeZone)
    {
        tm buff_tm = utls.convertUnix(unixtTime, timeZone);
        char dateSTR[] = "00/00/0000 00:00:00";
        if(buff_tm.tm_year < 1900)
        {
            sprintf(dateSTR, "%02i/%02i/%04i %02i:%02i:%02i", buff_tm.tm_mday, buff_tm.tm_mon +1, buff_tm.tm_year + 1900, buff_tm.tm_hour, buff_tm.tm_min, buff_tm.tm_sec);
        }
        else
        {
            sprintf(dateSTR, "%02i/%02i/%04i %02i:%02i:%02i", buff_tm.tm_mday, buff_tm.tm_mon +1, buff_tm.tm_year, buff_tm.tm_hour, buff_tm.tm_min, buff_tm.tm_sec);        
        }        
        return String(dateSTR);
    }

    String utility::toUniversalTime(unsigned long unixtTime, int timeZone)
    {
        tm buff_tm = convertUnix(unixtTime, timeZone);
        char _utc[] = "0000-00-00T00:00:00";
        char _lc[] = "00:00";
        char _lc_h[] = "00";
        String result = "0000-00-00T00:00:00+00:00";
        sprintf(_utc, "%04i-%02i-%02iT%02i:%02i:%02i", buff_tm.tm_year + 1900, buff_tm.tm_mon +1, buff_tm.tm_mday, 
        buff_tm.tm_hour,buff_tm.tm_min, buff_tm.tm_sec);
        if(timeZone >= 0)
        {
            sprintf(_lc_h, "%02i", timeZone);
            sprintf(_lc, "%03s:%02s", "+"+String(_lc_h), "00");
        }
        else{
            sprintf(_lc_h, "%03i", timeZone);
            sprintf(_lc, "%03s:%02s", String(_lc_h), "00");
        }
        result = String(_utc) + String(_lc);
        return result;
    }

    unsigned long utility::toUnix(tm time_)
    {
        unsigned long _unix = (long)0000000000;
            tm t;
            t.tm_mday = time_.tm_mday;
            t.tm_mon = time_.tm_mon;
            if(time_.tm_year < 1900)
            {
                t.tm_year = time_.tm_year;
            }
            else
            {
                t.tm_year = time_.tm_year -1900;
            }   
            t.tm_hour = time_.tm_hour;
            t.tm_min = time_.tm_min;
            t.tm_sec = time_.tm_sec;
            t.tm_isdst = -1;
            _unix = mktime(&t);
        return _unix;       
    }

    boolean utility::StringIsDigit(String validate_str)
    {
        for(byte i = 0; i < validate_str.length(); i++)
        {
          if(!isDigit(validate_str.charAt(i))) return false;
        }
        return true;
    }