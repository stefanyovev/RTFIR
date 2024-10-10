todos:
 - detect avx, fallback to sse, with createprocess
 - click/pops on correction (correct on a minute not second)
 - delay
 - reload filters
 - callback times measure; own load measure; score the cpu
 - timestamp on lines; details window;
 - 192/262
 - SR in conf
 - configure ui available before start

cpu usage:
 - make threads stoppable
 - when no load use 1 thread; make dsp load and system load grow side by side
 - waitforsingleobject, timerobject to min time 100ns (5-6 samples) to sleep thread; its in "if work if work if work .."

timing:
 - rdtsc on a single core || QueryUnbiasedInterruptTimePrecise
 - correct on a minute not second
 - dont consider first 10 callbacks. if priming dont aftermath; static inport_countdown, outport_countdown
 - consider last 3 sec for min/max
 - inc latency
 - create an output buffer then input callback to do half of the processing and output cb the other half - jobsperchannel to be always even
   or drive the canvas independently from callbacks
 - one queue for all jobs; input cb queues; cursor - processed
 
