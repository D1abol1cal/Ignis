#pragma once
#include "defines.h"

typedef struct clock {
    f64 start_time;
    f64 elapsed;
} clock;


//has no affect on non-started clocks.
void clock_update(clock* clock);

//starts the provided clock. Resets elapsed time to zero.
void clock_start(clock* clock);

//stops the provided clock. Does not reset elapsed time.
void clock_stop(clock* clock);