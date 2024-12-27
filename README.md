todos:
 - timing:
    - startup seq. 100ms 100ms then 800ms correct x10. start copy at 1sec
    - average ports min/max (because it suddenly changes)
    - one jobs queue for all threads. (InterlockedIncrement) jobid/jobrow/end_address/cursor_processed. input submits non-blocking. Output waits the jobrow.
    - QueryUnbiasedInterruptTimePrecise, QuerySystemTimePreciseAsFileTime
 - no-filter:
    - empty combo
    - sets k=0 to memcopy
    - padded default filter shuold produce the same as memcopy
 - profile/measure/clock_stopwatch/duty_cycles
 - configure ui available before start
 - delay
 - reload filters
 - filters on inputs
 
