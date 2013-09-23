/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <err.h>
#include <sys/resource.h>
#include <assert.h>`
#include <immintrin.h>

#include "pcm.h"
#include "perf_utils.h"
#include "metrics.h"

static void readcounters(int mycpu);
static void *thrstart(void *);


static pthread_t *threads;
static volatile int done;
static pthread_barrier_t barrier;

static struct evinfo (*events)[NEVENTS];    // [nthreads][NEVENTS]
static struct counter *counters;
struct counter *lastctr;

static double starttime;

PCM *pcm;
extern char *CPUName(int model);


void
setup(int pid)
{
    struct perf_event_attr hw;
    struct timeval tv;
    int ret;

    pcm = PCM::getInstance();
    PCM::ErrorCode status = pcm->program();
    if (status != PCM::Success)
    {
        fprintf(stderr, "Error: no access to PMU; try running as root\n");
        exit(1);
    }
    if (gbl.verbose)
        printf("CPU Model %s\n", CPUName(pcm->getModel()));
    memset(&hw, 0, sizeof(hw));
    hw.size = sizeof(hw);
    hw.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_TOTAL_TIME_RUNNING;
    //hw.inherit = 1;
    //hw.enable_on_exec = 1;
    //hw.disabled = 1;

    gbl.ncpus = pcm->getNumProcs();
    gbl.ncores = gbl.ncpus / pcm->getThreadsPerCore();
    gbl.nsockets = pcm->getNumSockets();
    gbl.corespersocket = gbl.ncores / gbl.nsockets;

    events = (evinfo (*)[NEVENTS])malloc(gbl.ncpus * NEVENTS * sizeof(struct evinfo));
    if (events == NULL)
        err(1, "Can't malloc events");
    threads = (pthread_t *)malloc(gbl.ncpus*sizeof(pthread_t));
    if (threads == NULL)
        err(1, "Can't malloc threads");
    memset(events, 0, gbl.ncpus*NEVENTS*sizeof(struct evinfo));
    counters = (counter *)_mm_malloc(gbl.ncpus * sizeof(struct counter), 64);
    if (counters == NULL)
        err(1, "Can't malloc counters");
    memset(counters, 0, gbl.ncpus * sizeof(struct counter));
    lastctr = (counter *)_mm_malloc(gbl.ncpus * sizeof(struct counter), 64);
    if (lastctr == NULL)
        err(1, "Can't malloc lastctr");
    memset(lastctr, 0, gbl.ncpus * sizeof(struct counter));
    /* Could do this per thread */
    for (int cpu = 0; cpu < gbl.ncpus; ++cpu)
        for (int i = 0; i < NEVENTS; ++i)
        {
            struct evinfo *ep = &events[cpu][i];
            hw.type = evcodes[i].type;
            hw.config = evcodes[i].code;
            ep->fd = perf_event_open(&hw, -1, cpu, -1, 0);
            if (ep->fd == -1)
                err(1, "CPU %d, event 0x%lx", cpu, hw.config);
#if 0
            // We'd like to use user-mode rdpmc pattern but need
            // newer kernel that exports even non-running counters
            ep->buf = mmap(NULL, 2*4096, PROT_READ|PROT_WRITE, MAP_SHARED,
                ep->fd, 0);
            if (ep->buf == MAP_FAILED)
                err(1, "CPU %d, event 0x%lx: cannot mmap buffer",
                    cpu, hw.config);
#endif
        }

    pthread_barrier_init(&barrier, NULL, gbl.ncpus+1);
    /* Now spawn a pthread on every processor */
    for (int i = 0; i < gbl.ncpus; ++i)
    {
        pthread_attr_t attr;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_attr_init(&attr);
        pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
        ret = pthread_create(&threads[i], &attr, thrstart, (void *)i);
        if (ret)
            err(1, "pthread_create returns %d", ret);
        pthread_attr_destroy(&attr);
    }

    setup_output();
    ret = gettimeofday(&tv, 0);
    if (ret != 0)
        err(1, "gettimeofday");
    starttime = tv.tv_sec + (double)tv.tv_usec * 1e-6;
    //pthread_barrier_wait(&barrier);
}

void
jointhreads()
{
    for (int i = 1; i < gbl.ncpus; ++i)
        pthread_join(threads[i], NULL);
}

// master thread timeout >> slave threads
static void
timeout()
{
    static struct timespec t = {0, 200000000};       // 5 times per second
    nanosleep(&t, 0);
}

static void
slavetimeout()
{
    static struct timespec t = {0, 33333333};   // 30 times per second
    nanosleep(&t, 0);
}

// This should tick faster than the master.
static void *
thrstart(void *arg)
{
    int mycpu = (int)((uint64_t)arg) & 0xFFFF;

    pthread_barrier_wait(&barrier);
    for (;;)
    {
        readcounters(mycpu);
        slavetimeout();

        if (done)
            break;
    }
    return 0;
}

static void
readctr(struct perf_event_mmap_page *pc, int fd, uint64_t val[3])
{
    int64_t s;
#if 0
    uint32_t seq, idx;
    uint64_t count, enabled, running;

    /* do-while is in case the counter gets swapped out while we're reading
     * the count. Note: need memory fence on big cores but not KNC.
     */
/* We'd like to use this when the newer version is supported (that supports
 * mux'd counters that aren't swapped in.
 */
    do
    {
	seq = pc->lock;
        // mfence();
	enabled = pc->time_enabled;
	running = pc->time_running;
        idx = pc->index;
	if (idx)
	{
	    count = _rdpmc(idx - 1);
            count += pc->offset;
	}
        else
            goto regular_read;
        // mfence();
    }
    while (pc->lock != seq);

    val[0] = count;
    val[1] = enabled;
    val[2] = running;
    goto done;
#endif
//regular_read:
    s = read(fd, val, 3*sizeof(uint64_t));
    if (s != 3*sizeof(uint64_t))
        err(1, "read counter");
//done:
    ;
}

static void
readcounters(int cpu)
{

    struct counter *cp = &counters[cpu];
    for (int i = 0; i < NEVENTS; ++i)
    {
        struct evinfo *ep = &events[cpu][i];
        union {
            __m256d c;
            uint64_t values[4];
        } u;

        readctr((perf_event_mmap_page *)ep->buf, ep->fd, u.values);
        if (u.values[0])
            _mm256_store_pd((double *)&cp->counts[i][0], u.c);
    }
}

void
closefiles()
{
    for (int cpu = 0; cpu < gbl.ncpus; ++cpu)
        for (int i = 0; i < NEVENTS; ++i)
            close(events[cpu][i].fd);
}

int
monitor(int pid, struct timeval *jointime)
{
    int ret, status = -1;
    uint64_t tsc, ovhd, start, duration, t;
    struct rusage usage;
    int sense = 1;

    pthread_barrier_wait(&barrier);
    start = _rdtsc();
    for (;;)
    {
        t = _rdtsc();
        printcounters(counters, t - start);
        if (!gbl.server)
        {
            if (waitpid(pid, &status, WNOHANG))
            {
                gettimeofday(jointime, 0);
                done = 1;
                break;
            }
        }
        start = _rdtsc();
        timeout();
    }
    close_output();
    return status;
}
