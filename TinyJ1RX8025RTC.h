//
//  J1RX8025RTC.h
//  
//
//  Created by OKU Junichirou on 2015/10/17.
//
//

#ifndef J1RX8025RTC_h
#define J1RX8025RTC_h

#include <Time.h>

class J1RX8025RTC {
public:
    // constructor
    J1RX8025RTC();
    
    //
    void init();
    static time_t get();
    static bool set(time_t t);
    static bool read (tmElements_t &tm);
    static bool write(tmElements_t &tm);
    static bool chipPresent();
    
private:
    static bool _exists;
    static uint8_t bin2bcd(uint8_t);
    static uint8_t bcd2bin(uint8_t);
};

extern J1RX8025RTC RX8025RTC;

#endif /* J1RX8025RTC_h */
