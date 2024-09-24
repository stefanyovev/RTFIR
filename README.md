todos:
 - detect avx, fallback to sse, with createprocess
 - warn about maximum filter length, dont load it
 - click/pops on correction (correct on a minute not second)
 - delay
 - reload filters
 - callback times measure; own load measure; score the cpu
 - log to file not ui or details window; timestamp on lines; label for load;
 - 192/262
 - link to sound control panel
 - configure ui available before start

cpu usage:
 - make threads stoppable
 - when no load use 1 thread; make dsp load and system load grow side by side
 - waitforsingleobject, timerobject to min time 100ns (5-6 samples) to sleep thread; its in "if work if work if work .."

timing:
 - correct on a minute not second
 - dont consider first 10 callbacks. if priming dont aftermath; static inport_countdown, outport_countdown
 - consider last 3 sec for min/max
 - inc latency
 - create an output buffer then input callback to do half of the processing and output cb the other half - jobsperchannel to be always even
   or drive the canvas independently from callbacks
 - one queue for all jobs; input cb queues; cursor - processed
 
