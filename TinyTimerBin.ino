#include <Arduino.h>

#include <avr/sleep.h>
#include <avr/interrupt.h>

// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <TInyWireM.h>
#include <Time.h>

#include "ST7032.h"
#include "Adafruit_ADXL345_U.h"

#include "J1ClockKit.h"
#include "TinyJ1RX8025RTC.h"

// type definitions
// http://keitetsu.blogspot.jp/2014/11/arduinotypedef.html

#include "typedef.h"

// static variables
ST7032 lcd;

/* Constant */
const int LED = 1;
const int TIMED_OUT = 10 * 1000;

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

static state_t sState = stateIdle;

static J1ClockKit::ElapsedTimer *sTimer = NULL;

static unsigned long sUpsideDownTime = 0;

static J1ClockKit::WDTEnabledTimer *sMillis = NULL;

volatile boolean f_wdt = 1;

event_t getEvent( state_t state ) {
    static event_t prevEvent = eventNothing;

    event_t event = eventNothing;
    
    // Get an acceleration sensor event
    uint8_t reg = accel.readRegister(ADXL345_REG_INT_SOURCE);
    
    // analyze an acceleration event
    if (accel.getZ() < 0) {
        if (prevEvent != eventUpsideDown) {
            event     = eventUpsideDown;
        }
        prevEvent = eventUpsideDown;
    }
    else if (accel.getX() < -100 || accel.getX() > 100 ||
             accel.getY() < -100 || accel.getY() > 100) {
        if (prevEvent != eventTilt) {
            event = eventTilt;
        }
        prevEvent = eventTilt;
    }
    else {
        prevEvent = eventNothing;
    }
    
    // analyze a timed out event
    if (state != stateSleep && sTimer->elapsed() > TIMED_OUT) {
        event = eventTimedOut;
    }

    return event;
}

transition_t selectTransition( const event_t event, const state_t state ) {
    transition_t transition = transitionNothing;
    
    // make an transition from the event
    switch (state) {
        case stateIdle:
            switch (event) {
                case eventUpsideDown:
                    transition = transitionToMarked;
                    break;
                case eventTilt:
                    transition = transitionToElapsed;
                    break;
                case eventTimedOut:
                    transition = transitionToSleep;
                    break;
            }
            break;
       case stateSleep:
            switch (event) {
                case eventUpsideDown:
                    transition = transitionToMarked;
                    break;
                case eventTilt:
                    transition = transitionToElapsed;
                    break;
            }
            break;
        case stateElapsed:
            switch (event) {
                case eventTimedOut:
                    transition = transitionToIdle;
                    break;
                case eventUpsideDown:
                    transition = transitionToMarked;
                    break;
            }
            break;
        case stateMarked:
            transition = transitionToElapsed;
            break;
    }
    return transition;
}

state_t doTransition( const transition_t transition, const state_t state ) {
    state_t nextState = state;
    
    // transition
    switch (transition) {
        case transitionNothing:
            break;
        case transitionToIdle:
            sTimer -> start();
            displayOff();
            nextState = stateIdle;
            break;
        case transitionToSleep:
            sTimer -> stop();
            nextState = stateSleep;
            break;
        case transitionToElapsed:
             sTimer -> start();
            nextState = stateElapsed;
            break;
        case transitionToMarked:
            sUpsideDownTime = sMillis->millisecs(); // Mark he current time
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
            delay(100);
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
            nextState = stateMarked;
            break;
    }

    return nextState;
}

void doAction( const state_t state ) {
    switch (state) {
        case stateSleep:
//            digitalWrite(LED, HIGH);
//            delay(300);
//            digitalWrite(LED, LOW);
//            delay(150);
            
            system_sleep();  // Send the unit to sleep
            sMillis->wakedUp();

//            digitalWrite(LED, HIGH);
//            delay(10);
//            digitalWrite(LED, LOW);
//            delay(150);
//            digitalWrite(LED, HIGH);
//            delay(50);
//            digitalWrite(LED, LOW);
            break;
        case stateElapsed:
            displayRoundedTime( sUpsideDownTime == 0 ?
                               0 : (sMillis->millisecs() - sUpsideDownTime) / 1000 );
            delay(500);
            break;
        case stateMarked:
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);

            break;
    }
}


void displayRoundedTime( const time_t time ) {
    byte val = 0;
    tmByteFields field = J1ClockKit::roundTime( time, val );
    char buf[8] = "1234567";
    char *s = buf;
    itoa(val, s, 10);

    lcd.display();
    lcd.noBlink();
    lcd.clear();
    
    lcd.setCursor(0,0);
    lcd.print(s);
    
    lcd.setCursor(3,0);
    char ss[] = "sec*";
    char sm[] = "min*";
    char sh[] = "hour*";
    char sd[] = "day*";

    switch (field) {
        case tmSecond:  // Second
            s = ss;
            break;
        case tmMinute:  // minute
            s = sm;
            break;
        case tmHour:    // hour
            s = sh;
            break;
        case tmDay:     // day
            s = sd;
            break;
    }
    int i = 0;
    while (i < sizeof(buf) && s[i] != '*') {
        i++;
    }
    if (i < sizeof(buf) - 1) {
        if (val > 1) {
            s[ i     ] = 's';
            s[ i + 1 ] = '\0';
        }
        else {
            s[ i     ] = '\0';
        }
    }
    lcd.print(s);
}

void displayOff( void ) {
    lcd.noDisplay();
}

#pragma mark -

// http://www.re-innovation.co.uk/web12/index.php/en/blog-75/306-sleep-modes-on-attiny85

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
    
    byte bb;
    int ww;
    if (ii > 9 ) ii=9;
    bb=ii & 7;
    if (ii > 7) bb|= (1<<5);
    bb|= (1<<WDCE);
    ww=bb;
    
    MCUSR &= ~(1<<WDRF);
    // start timed sequence
    WDTCR |= (1<<WDCE) | (1<<WDE);
    // set new watchdog timeout value
    WDTCR = bb;
    WDTCR |= _BV(WDIE);
}

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
    
    cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
    
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
    sleep_enable();
    
    sleep_mode();                        // System actually sleeps here
    
    sleep_disable();                     // System continues execution here when watchdog timed out
    
    sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
    
}

void setup() {
    lcd.begin(8,2);
    lcd.setContrast(30);
    lcd.display();
    lcd.setCursor(0,0);
    lcd.print(F("TimerBin"));
    lcd.setCursor(0,1);
    lcd.print(F("  v0.02"));
    
    delay(2000);
    lcd.clear();
    
    /* Initialise the sensor */
    if(!accel.begin())
    {
        /* There was a problem detecting the ADXL345 ... check your connections */
        while(1) {
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
            delay(100);
        }
    }
    
    /* Set the range to whatever is appropriate for your project */
    accel.setRange(ADXL345_RANGE_2_G);
    
    // x, y, z enable
//    accel.writeRegister(ADXL345_REG_TAP_AXES, 0x07);
//    accel.writeRegister(ADXL345_REG_INT_MAP,0); // all interrupts map to INT1
//    
//    accel.writeRegister(ADXL345_REG_ACT_INACT_CTL,0x70); // active control
//    accel.writeRegister(ADXL345_REG_THRESH_ACT,20); // active threshold = 50
//    
//
//    uint8_t reg = accel.readRegister(ADXL345_REG_INT_ENABLE);
//    reg |= 0x10; // Ativity
//    accel.writeRegister(ADXL345_REG_INT_ENABLE, reg);
    
    sState = stateIdle;
    sTimer = new J1ClockKit::ElapsedTimer();
    sTimer -> start();

//    pinMode(3,INPUT);
//    cli();
//    GIMSK |= 0x20; // Genenral Interrupt Mask Register PCIE = 1
//    PCMSK |= 0x08; // Pin Chnage Mask Regiter PCINT3 = 1
//    sei();
    
    setup_watchdog(5); // approximately 0.5 seconds sleep
    
    sMillis = new J1ClockKit::WDTEnabledTimer( 500 );
}

void loop() {
    // Get an event
    event_t event = getEvent( sState );
    
    // Select a transition from the event in the state
    transition_t transition = selectTransition( event, sState );
    
    // Do the state transition and get a next state
    sState = doTransition( transition, sState );
    
    // Do the action in the state
    doAction( sState );
 
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
    f_wdt=1;  // set global flag
}

//ISR(PCINT3_vect)
//{
//    f_wdt=1;  // set global flag
//}
//
//
