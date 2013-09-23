/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef PERF_UTILS_H
#define PERF_UTILS_H
#include <syscall.h>
#include <linux/perf_event.h>

static int
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

/*
 * values[0] = raw count
 * values[1] = TIME_ENABLED
 * values[2] = TIME_RUNNING
 */
static inline uint64_t
perf_scale(uint64_t * values)
{
    uint64_t res = 0;

    if (!values[2] && !values[1] && values[0])
	warnx
	    ("WARNING: time_running = 0 = time_enabled, raw count not zero");

    if (values[2] > values[1])
	warnx("WARNING: time_running > time_enabled");

    if (values[2])
	res = (uint64_t) ((double) values[0] * values[1] / values[2]);
    return res;
}

static inline uint64_t
perf_scale_delta(uint64_t * values, uint64_t * prev_values)
{
    uint64_t res = 0;

    if (!values[2] && !values[1] && values[0])
	warnx
	    ("WARNING: time_running = 0 = time_enabled, raw count not zero");

    if (values[2] > values[1])
	warnx("WARNING: time_running > time_enabled");

    if (values[2] - prev_values[2])
	res =
	    (uint64_t) ((double)
			((values[0] - prev_values[0]) * (values[1] -
							 prev_values[1]) /
			 (values[2] - prev_values[2])));
    return res;
}


/*
 * TIME_RUNNING/TIME_ENABLED
 */
static inline double
perf_scale_ratio(uint64_t * values)
{
    if (!values[1])
	return 0.0;

    return values[2] * 1.0 / values[1];
}
#endif
