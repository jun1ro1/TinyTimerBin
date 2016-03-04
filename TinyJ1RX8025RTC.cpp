//
//  J1RX8025RTC.cpp
//  
//
//  Created by OKU Junichirou on 2015/10/17.
//
//

#include "TinyJ1RX8025RTC.h"
#include <TinyWireM.h>
#include <Arduino.h>

#pragma mark - constants

#define RX8025_ADDR                0x32

#define RX8025_SECONDS             0x00
#define RX8025_MINUTES             0x10
#define RX8025_HOURS               0x20
#define RX8025_WEEKDAYS            0x30
#define RX8025_DAYS                0x40
#define RX8025_MONTHS              0x50
#define RX8025_YEARS               0x60

#define RX8025_DIGITAL_OFFSET      0x70

#define RX8025_WALARM_MINUTE       0x80
#define RX8025_WALARM_HOUR         0x90
#define RX8025_WALARM_WEEKDAY      0xA0

#define RX8025_DALARM_MINUTE       0xB0
#define RX8025_DALARM_HOUR         0xC0

#define RX8025_CONTROL1            0xE0
#define RX8025_CONTROL2            0xF0

bool J1RX8025RTC::_exists = false;

#pragma mark - constructor
J1RX8025RTC::J1RX8025RTC() {
}

void J1RX8025RTC::init() {
    TinyWireM.begin();
    
    // initialize RX8025
    TinyWireM.beginTransmission( RX8025_ADDR );
    
    TinyWireM.write( RX8025_CONTROL1 );
    
    // CONTROL1
    //   WALE  week alarm off
    //   DALE  day  alarm off
    //   24H   mode
    //   CLEN2 off
    //   CT2, CT1, CT0 off
    TinyWireM.write( 0x20 ); // CONTROL1
    
    // CONTROL2
    //   VDSL
    //   VDET
    //   XST
    //   PON
    //   CLEN1
    //   CTFG
    //   WAFG
    //   DAFG
    TinyWireM.write( 0x00 ); // CONTROL2
    TinyWireM.endTransmission();
    
    TinyWireM.beginTransmission(RX8025_ADDR);
    TinyWireM.write( RX8025_SECONDS );
    TinyWireM.write( 0x00 ); // Seconds
    TinyWireM.write( 0x00 ); // Minutes
    TinyWireM.write( 0x12 ); // Hours
    TinyWireM.write( 0x01 ); // Weekdays Sunday = 0, Monday = 1
    TinyWireM.write( 0x01 ); // Days
    TinyWireM.write( 0x01 ); // Months January
    TinyWireM.write( 0x01 ); // Years 2001
    TinyWireM.write( 0x00 ); // Digital Offset
    TinyWireM.endTransmission();
}

#pragma mark - read/write on tmElements_t type
bool J1RX8025RTC::read(tmElements_t &tm) {
    // read from address 0x0F (CONTROL2), Ox00 (SECONDS), ...
    TinyWireM.requestFrom(RX8025_ADDR, 8);

    uint8_t control = TinyWireM.read();      // CONTROL1
    tm.Second = bcd2bin( TinyWireM.read() );
    tm.Minute = bcd2bin( TinyWireM.read() );
    tm.Hour   = bcd2bin( TinyWireM.read() );
    tm.Wday   = TinyWireM.read() + 1;        // Sunday = 0 to 1
    tm.Day    = bcd2bin( TinyWireM.read() );
    tm.Month  = bcd2bin( TinyWireM.read() );
    tm.Year   = (int)bcd2bin( TinyWireM.read() ) + 2000 - 1970; // offset from 2000 -> 1970

    J1RX8025RTC::_exists = ((control & 0x10) == 0); // false if PON is on
    return J1RX8025RTC::_exists;
}

bool J1RX8025RTC::write(tmElements_t &tm) {

    time_t t = makeTime(tm);
    TinyWireM.beginTransmission(RX8025_ADDR);
    
    // write from seconds register
    TinyWireM.write(RX8025_SECONDS);

    // makeTime -> breakTime
    
    // write data
    TinyWireM.write( bin2bcd( tm.Second)   );
    TinyWireM.write( bin2bcd( tm.Minute)   );
    TinyWireM.write( bin2bcd( tm.Hour)     );
    TinyWireM.write( bin2bcd( weekday(t) - 1 ) ); // Sunday = 1 to 0
    TinyWireM.write( bin2bcd( tm.Day)      );
    TinyWireM.write( bin2bcd( tm.Month)    );
    TinyWireM.write( bin2bcd( (uint8_t)( (int)tm.Year + 1970 - 2000 ) ) ); // tm.Year : offset from 1970

    J1RX8025RTC::_exists =  (TinyWireM.endTransmission() == 0);
    return J1RX8025RTC::_exists;
}


#pragma mark - get/set on time_t

time_t J1RX8025RTC::get() {
    tmElements_t tm;
    time_t time = J1RX8025RTC::read(tm) ? makeTime(tm) : 0;
    return time;
}


bool J1RX8025RTC::set(time_t t) {
    tmElements_t tm;
    breakTime(t, tm);
    return J1RX8025RTC::write(tm);
}


#pragma mark -
bool J1RX8025RTC::chipPresent() {
    return J1RX8025RTC::_exists;
}

#pragma mark - utility methods

/**
  convert a binary value to a BCD format
*/
uint8_t J1RX8025RTC::bin2bcd(uint8_t val) {
    return val + (val / 10) * 6;
}

uint8_t J1RX8025RTC::bcd2bin(uint8_t val) {
    return val - ( val >> 4 ) * 6;
}

J1RX8025RTC RX8025RTC = J1RX8025RTC(); // create an instance

