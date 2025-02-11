Realtime Sound Crossover for Windows   

![screenshot](https://rtfir.com/screenshot5.png)   

Todos:

- timing:
	- startup seq.
		- dont consider any graphs first 100ms for min/max,L,G; require 100ms available to calc min/max,L,G; 200ms graph boot;
		- 100ms 100ms then 800ms correct x10. last corrrection also corrects history then start copy at 1sec
	- average ports min/max (because it suddenly changes)

- code:
	- frontend as object; create, add_device.. mainloop
	- modules: include or link; private, public, symbols; .h/.c/.o; init, self, instances
	- threadpool object; return instance; threads_submit( self, f, arg, argsize )  (NOTE)
	- device objects; generate names; hide portaudio from frontend	
	- buffers object; read write readpos writepos; input_buffers/output_buffers	 (NOTE)
		- input_buffers/output_buffers; input callback submits, output callback waits; tasks id
	- DeviceStream object; has pointer to inp_buffers/outp_buffers 
	- (NOTE) because if there are more than 1 DeviceStreams, threads_wait will fail
	- profile/measure/clock_stopwatch/duty_cycles
	- QueryUnbiasedInterruptTimePrecise, QuerySystemTimePreciseAsFileTime

- features:
	- delay
	- filters on inputs
	- reload filters button
	- mix
