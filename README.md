Realtime Loudspeaker Crossover for Windows

Todos:

 - find out why default filter differs from memcopy 
 - filters: 
	- untie convolve_prepare from set_filter; int convolve_prepare(k,kn)
	- convolve to separate file with tests
	- padded default filter shuold produce the same as memcopy
	- untie threads from convolve; job/args struct
 - ui:
    - empty filter combo; sets k=0 to memcopy (not to default filter) +default filter +mid filter
    - correction msg to be 'replayed'|'skipped'
    - dash: move load label to bottom; show SR1/SR2
    - configure ui available before start / show conf
    - widgets layer; create on wm_create in wndproc
    - fileopendlg/comctl6 for filters dir
 - build:
    - version inputbox / put version in exe
    - comctl

 - timing:
    - startup seq.
        - dont consider any graphs first 100ms for min/max,L,G; require 100ms available to calc min/max,L,G; 200ms graph boot;
		- 100ms 100ms then 800ms correct x10. last corrrection also corrects history then start copy at 1sec
    - average ports min/max (because it suddenly changes)
	- output buffers - so input callback can threads_submit; why every channel waits others
    - one jobs queue for all threads. (InterlockedIncrement) jobid/jobrow/end_address/cursor_processed. input submits non-blocking. Output waits the jobrow.
	- profile/measure/clock_stopwatch/duty_cycles
    - QueryUnbiasedInterruptTimePrecise, QuerySystemTimePreciseAsFileTime
 - features:
    - delay
    - filters on inputs
	- reload filters button
	- mix
