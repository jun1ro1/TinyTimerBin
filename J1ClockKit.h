//
//  J1ClockKit.hpp
//
//
//  Created by OKU Junichirou on 2015/10/18.
//
//

#ifndef J1ClockKit_h
#define J1ClockKit_h

#include <Time.h>

namespace J1ClockKit {
    class Crown {
    public:
        Crown();

        void load( tmElements_t &tm );
        void save( tmElements_t &tm );

        int  getValue();

        void forward ( const int val = 1 );
        void backward( const int val = 1 );

        tmByteFields getField();
        void select( const tmByteFields field );
        void move( const int val );

    protected:
        bool isLeap( const int year );
        tmElements_t  timeElements;
        tmByteFields  field;
        int           minval;
        int           maxval;
        int           value;

        void prepare();
        void cleanup();
    };

    class ElapsedTimer {
    public:
        ElapsedTimer();

        void start( const int origin = 0 );
        void stop ( void );
        unsigned long elapsed();

    protected:
        unsigned long _origin;
    };
    
    class WDTEnabledTimer {
    public:
        WDTEnabledTimer( const int interval = 0 );
        
        void wakedUp( void );
        unsigned long millisecs( void );
        
    protected:
        unsigned long _wdtInterval;
        unsigned long _sleepTimeSum;
    };
    
    tmByteFields roundTime( const time_t time, byte &val );
    
} // namespace

#endif /* J1ClockKit_h */
