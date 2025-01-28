Realtime Loudspeaker Crossover for Windows

Todos:

 - find out why default filter differs from memcopy / lowpass pops
 - unset filter / switch to copy
 - filters: 
	- untie convolve_prepare from set_filter; int convolve_prepare(k,kn)
	- convolve to separate file with tests
	- padded default filter shuold produce the same as memcopy
 - ui:
    - empty filter combo; sets k=0 to memcopy (not to default filter) +default filter +mid filter
    - dash: move load label to bottom; show SR1/SR2
    - configure ui available before start / show conf
    - widgets layer; row widget; create on wm_create in wndproc maybe
    - fileopendlg/comctl6 for filters dir
 - build:
    - version inputbox / put version in exe
    - comctl

 - timing:
    - startup seq.
        - dont consider any graphs first 100ms for min/max,L,G; require 100ms available to calc min/max,L,G; 200ms graph boot;
		- 100ms 100ms then 800ms correct x10. last corrrection also corrects history then start copy at 1sec
    - average ports min/max (because it suddenly changes)
	- input_buffers/output_buffers; input callback submits, output callback waits; tasks id
	- profile/measure/clock_stopwatch/duty_cycles
    - QueryUnbiasedInterruptTimePrecise, QuerySystemTimePreciseAsFileTime
 - features:
    - delay
    - filters on inputs
	- reload filters button
	- mix
