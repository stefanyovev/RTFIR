Realtime Sound Crossover for Windows

Todos:
 
 - find out why default filter differs from memcopy / slightly displaced / lowpass pops
	- padded default filter should produce the same as memcopy
	- visual analyzer

 - generate filters
 - filters/44100/name.txt

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
