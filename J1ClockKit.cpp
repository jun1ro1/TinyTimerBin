//
//  J1ClockKit.cpp
//
//
//  Created by OKU Junichirou on 2015/10/18.
//
//

#include <Arduino.h>

#include "J1ClockKit.h"

static unsigned char _days[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

bool J1ClockKit::Crown::isLeap( int year ) {
    year = year + 2000;
    return ( year % 400 == 0 ) || (( year % 4 == 0 ) && ( year % 100 !=0 ));
}

void J1ClockKit::Crown::prepare() {
    switch ( this->field ) {
        case tmMinute:  // minute
            this->minval =  0;
            this->maxval = 59;
            this->value  = this->timeElements.Minute;
            break;
        case tmHour:    // hour
            this->minval =  0;
            this->maxval = 23;
            this->value  = this->timeElements.Hour;
            break;
        case tmDay:     // day
            this->minval =  1;
            this->maxval = _days[ this->timeElements.Month - 1 ];
            if ( this->isLeap( this->timeElements.Year )
                && this->timeElements.Month == 2 ) {
                this->maxval++;
            }
            this->value  = this->timeElements.Day;
            break;
        case tmMonth:   // month
            this->minval =  1;
            this->maxval = 12;
            this->value  = this->timeElements.Month;
            break;
        case tmYear:    // year
            this->minval =  0;
            this->maxval = 38;
            this->value  = this->timeElements.Year + 1970 - 2000; // offset from 2000
            break;
        default:
            // nothing to do
            break;
    }
}

void J1ClockKit::Crown::cleanup() {
    // write the value to the field
    switch ( this->field ) {
        case tmMinute:  // minute
            this->timeElements.Minute = this->value;
            break;
        case tmHour:    // hour
            this->timeElements.Hour   = this->value;
            break;
        case tmDay:     // day
            this->timeElements.Day    = this->value;
            break;
        case tmMonth:   // month
            this->timeElements.Month  = this->value;
            break;
        case tmYear:    // year
            this->timeElements.Year   = this->value + 2000 - 1970;
            break;
        default:
            // nothing to do
            break;
    }
}

J1ClockKit::Crown::Crown() {
    // set the day 2011-01-01 00:00:00 to the timeElements
    this->timeElements.Second =  0;
    this->timeElements.Minute =  0;
    this->timeElements.Hour   = 12;
    this->timeElements.Wday   =  2; // Monday is 1
    this->timeElements.Day    =  1;
    this->timeElements.Month  =  1;
    this->timeElements.Year   = 2001 - 1970;
    
    this->field              = tmYear;
    this->prepare();
}

void J1ClockKit::Crown::load( tmElements_t &tm ) {
    this->timeElements.Second = tm.Second;
    this->timeElements.Minute = tm.Minute;
    this->timeElements.Hour   = tm.Hour;
    this->timeElements.Day    = tm.Day;
    this->timeElements.Month  = tm.Month;
    this->timeElements.Year   = tm.Year;
    
    time_t t = makeTime( tm );
    this->timeElements.Wday   = weekday( t );
    
    this->field = (tmByteFields) min( max( (int)this->field, (int)tmMinute ), (int)tmYear );
    this->prepare();
}

void J1ClockKit::Crown::save( tmElements_t &tm ) {
    this->cleanup();
    
    time_t t = makeTime( this->timeElements );
    this->timeElements.Wday = weekday( t );
    
    tm.Second = this->timeElements.Second;
    tm.Minute = this->timeElements.Minute;
    tm.Hour   = this->timeElements.Hour;
    tm.Wday   = this->timeElements.Wday;
    tm.Day    = this->timeElements.Day;
    tm.Month  = this->timeElements.Month;
    tm.Year   = this->timeElements.Year;
}

void J1ClockKit::Crown::forward( const int val ) {
    this->value += val;
    this->value = min( max( this->value, this->minval ), this->maxval );
}

void J1ClockKit::Crown::backward( const int val ) {
    this->value -= val;
    this->value = min( max( this->value, this->minval ), this->maxval );
}

int J1ClockKit::Crown::getValue() {
    return this->value;
}

tmByteFields J1ClockKit::Crown::getField() {
    return this->field;
}

void J1ClockKit::Crown::select( const tmByteFields field ) {
    this->cleanup();
    if ( tmMinute <= field && field <= tmYear && field != tmWday) {
        this->field = field;
    }
    this->prepare();
}

void J1ClockKit::Crown::move( const int val ) {
    int field = (int)this->getField() + val;
    // skip tmWday
    if (field == (int)tmWday) {
        field += (val >=0) ? +1 : -1;
    }
    // round up between tmMinute and tmYear
    field = (field < (int)tmMinute) ? (int)tmYear   : field;
    field = (field > (int)tmYear)   ? (int)tmMinute : field;
    this->select( (tmByteFields)field );
}


J1ClockKit::ElapsedTimer::ElapsedTimer() {
    this->_origin = 0;
}

void J1ClockKit::ElapsedTimer::start( const int origin ) {
    this->_origin = (origin == 0) ? millis() : origin;
}

void J1ClockKit::ElapsedTimer::stop( void ) {
    this->_origin = 0;
}

unsigned long J1ClockKit::ElapsedTimer::elapsed() {
    unsigned long s = millis();
    if (this->_origin == 0) {
        return 0;
    }
    else {
        return (s >= this->_origin ) ? s - this->_origin :
            (unsigned long)0xFFFFFFFF - this->_origin + s + 1UL;
    }
}


J1ClockKit::WDTEnabledTimer::WDTEnabledTimer( const int interval ) {
    this->_wdtInterval  = interval;
    this->_sleepTimeSum = 0;
}
    
void J1ClockKit::WDTEnabledTimer::wakedUp( void ) {
    this ->_sleepTimeSum += (unsigned long )this->_wdtInterval;
}

unsigned long J1ClockKit::WDTEnabledTimer::millisecs( void ) {
    return millis() + this->_sleepTimeSum;
}



tmByteFields J1ClockKit::roundTime( const time_t time, byte &val ) {
    if (time >= (48UL * 60UL * 60UL)) {
        // rather than 2 days
        val = time / (24UL * 60UL * 60UL); // seconds to days
        return tmDay;
    }
    else if (time >= 60 * 60) {
        val = time / ( 60 * 60 );    // seconds to hours
        return tmHour;
    }
    else if (time >= 60) {
        val = time / 60;             // seconds to minutes
        return tmMinute;
    }
    else {
        val = time;
        return tmSecond;
    }
}
