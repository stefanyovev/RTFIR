todos:
 - detect avx, fallback to sse, with createprocess
 - delay
 - reload filters
 - default filter highpass or play dimmed
 - callback times measure; own load measure; score the cpu
 - details window;
 - configure ui available before start
 - test convolve; sizes

cpu usage:
 - make threads stoppable
 - when no load use 1 thread; make dsp load and system load grow side by side
 - waitforsingleobject, timerobject to min time 100ns (5-6 samples) to sleep thread; its in "if work if work if work .."

timing:
 - inc latency
 - if priming dont aftermath
 - dont consider first second
 - consider last 3 seconds only
 - correct on a minute not second
 - rdtsc on a single core || QueryUnbiasedInterruptTimePrecise
 - create an output buffer then input callback to do half of the processing and output cb the other half - jobsperchannel to be always even
   or drive the canvas independently from callbacks
 - one queue for all jobs; input cb queues; cursor - processed
 
