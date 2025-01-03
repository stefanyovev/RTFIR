Realtime FIR Filter Convolver for Windows

todos:
 - ui:
	- correction msg to be 'replayed'|'skipped'
	- dash: move load label to bottom; show SR1/SR2
	- empty filter combo sets k=0 to memcopy (not to default filter)
    - configure ui available before start / show conf
 - filters: 
	- untie threads from convolve; job/args struct; untie convolve_prepare from set_filter; int convolve_prepare(k,kn)
	- convolve to separate file with tests; padded default filter shuold produce the same as memcopy
 - timing:
    - startup seq. 100ms 100ms then 800ms correct x10. last corrrection also corrects history then start copy at 1sec
    - average ports min/max (because it suddenly changes)
	- output buffers - so input callback can threads_submit; why every channel waits others
    - one jobs queue for all threads. (InterlockedIncrement) jobid/jobrow/end_address/cursor_processed. input submits non-blocking. Output waits the jobrow.
	- profile/measure/clock_stopwatch/duty_cycles
    - QueryUnbiasedInterruptTimePrecise, QuerySystemTimePreciseAsFileTime
 - build:
	- version inputbox / put version in exe
	- why two entires(cmakelistst and build.sh) / dont requrie new files to be added to buildlist
 - features:
    - delay
    - filters on inputs
	- reload filters button
