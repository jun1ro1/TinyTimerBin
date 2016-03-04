//
//  typedef.h
//  
//
//  Created by OKU Junichirou on 2015/10/28.
//
//

#ifndef typedef_h
#define typedef_h

// type definitions
typedef enum {
    eventNothing,
    eventTimedOut,
    eventUpsideDown,
    eventTilt,
    eventEnd,
} event_t;

typedef enum {
    transitionNothing,
    transitionToIdle,
    transitionToSleep,
    transitionToElapsed,
    transitionToMarked,
    transitionnEnd,
} transition_t;

typedef enum {
    stateIdle,
    stateSleep,
    stateElapsed,
    stateMarked,
    stateEnd,
} state_t;

#endif /* typedef_h */
