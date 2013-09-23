/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>

#include "pmu.h"

static char *iomem;
static int iofd;

static uint64_t counters[NGBOXES][2][2];
static uint32_t lastval[NGBOXES][2][2];

void
pmu_init()
{
#if 0
    iofd = open("/dev/uio0", O_RDWR|O_SYNC);
    if (iofd < 0)
        err(1, "failed to open /dev/uio0");
    /* Note offset has a special meaning with uio devices */
    iomem = mmap(NULL, PMU_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, iofd,
                 0);
    if (iomem == MAP_FAILED)
        err(1, "mmap");
#else
    iofd = open("/dev/mem", O_RDWR|O_SYNC);
    if (iofd < 0)
        err(1, "failed to open /dev/mem");
    /* Note offset has a special meaning with uio devices */
    iomem = mmap(NULL, PMU_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, iofd,
                BASE);
    if (iomem == MAP_FAILED)
        err(1, "mmap");
#endif
    memset(counters, 0, sizeof(counters));
    memset(lastval, 0, sizeof(lastval));
}

void
pmu_fini()
{
    munmap(iomem, PMU_SIZE);
    close(iofd);
}

static void
wrgbox(int gbox, int reg, int val)
{
    uint64_t base = REGOFFS(gbox_base[gbox]);
    int *p = (int *)(iomem + base + reg);
    *p = val;
}

static uint32_t
rdgbox(int gbox, int reg)
{
    uint64_t base = REGOFFS(gbox_base[gbox]);
    int *p = (int *)(iomem + base + reg);
    return *p;
}


uint64_t
pmu_rdctr(int controller, int channel, int type)
{
    static int ctrnum[2][2] = {
        {GBOX_FBOX_EMON0,GBOX_FBOX_EMON1},
        {GBOX_FBOX_EMON2,GBOX_FBOX_EMON3}
    };
    int ctr = ctrnum[channel][type];
    uint32_t t = rdgbox(controller, ctr);
    uint32_t u = lastval[controller][channel][type];
    if (t >= u)
        counters[controller][channel][type] += t - u;
    else
    {
        counters[controller][channel][type] += 4294967295u - u;
        counters[controller][channel][type] += t;
    }
    lastval[controller][channel][type] = t;
    return counters[controller][channel][type];
}

void
pmu_start()
{
    for (int i = 0; i < NGBOXES; ++i)
    {
        union gbox_fbox_evtsel e;
        wrgbox(i, GBOX_FBOX_CONFIG, GBOX_FBOX_RESET_ALL);
        e.all = 0;
        e.s.evtsel0 = CH0_NORMAL_READ;
        e.s.evtsel1 = CH0_NORMAL_WRITE;
        e.s.evtsel2 = CH1_NORMAL_READ;
        e.s.evtsel3 = CH1_NORMAL_WRITE;
        wrgbox(i, GBOX_FBOX_CONFIG2, e.all);
        wrgbox(i, GBOX_FBOX_CONFIG, GBOX_FBOX_ENABLE);
    }
}

void
pmu_stop()
{
    for (int i = 0; i < NGBOXES; ++i)
    {
        wrgbox(i, GBOX_FBOX_CONFIG, GBOX_FBOX_RESET_ALL);
    }
    pmu_fini();
}

#if DEBUG
main()
{
    pmu_init();
    pmu_start();
    uint64_t reads = 0, writes = 0;
    uint64_t prevreads=0, prevwrites=0;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 2; ++j)
        {
            prevreads += pmu_rdctr(i, j, 0);
            prevwrites += pmu_rdctr(i, j, 1);
        }
    for (int i = 0; i < 100; ++i)
    {
        sleep(1);
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 2; ++j)
            {
                reads += pmu_rdctr(i, j, 0);
                writes += pmu_rdctr(i, j, 1);
            }
        printf("%lu %lu\n", reads-prevreads, writes-prevwrites);
        prevreads = reads;
        prevwrites = writes;
        reads = 0;
        writes = 0;
    }
    pmu_stop();
    pmu_fini();
}
#endif
