/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/resource.h>

#include <immintrin.h>

#include "metrics.h"
#include "perf_utils.h"
#include "pmu.h"

#include "fields.h"

static uint64_t starttsc;
static FILE *outfile;
static uint64_t prevnreads = 0, prevnwrites = 0;

static char *hdrLabels[nfields];
static int hdrIndexes[nfields];

void
setup_output()
{
    char hdr[1024];
    hdr[sizeof(hdr)-1] = 0;

    pmu_init();
    pmu_start();
    for (int i = 0; i < NGBOXES; ++i)
        for (int j = 0; j < 2; ++j)
        {
            prevnreads += pmu_rdctr(i, j, 0);
            prevnwrites += pmu_rdctr(i, j, 1);
        }
    double ibw = (gbl.ncores-1) * 2 * gbl.hz;
    labels[fInst].max = ibw;
    labels[fVPU].max = ibw;
    double vop = (gbl.ncores-1) * 8 * gbl.hz;
    labels[fVpuSP].max = 2*vop;
    labels[fVpuDP].max = vop;

    // order doesn't really matter but we're used to this and it's better
    // for Excel
    hdrLabels[0] = "Time";
    hdrLabels[1] = "Threads";
    hdrIndexes[0] = fTime;
    hdrIndexes[1] = fThreads;
    int j = 2;
    for (int i = 0; i < nfields; ++i)
        if (i != fTime && i != fThreads)
        {
            hdrLabels[j] = labels[i].name;
            hdrIndexes[j] = i;
            ++j;
        }
    hdr[0] = 0;
    hdr[sizeof(hdr)-1] = 0;
    bool first = true;
    for (int i = 0; i < nfields; ++i)
    {
        if (first)
            first = false;
        else
            strncat(hdr, ",", sizeof(hdr)-1);
        strncat(hdr, hdrLabels[i], sizeof(hdr)-1);
    }
    strncat(hdr, "\n", sizeof(hdr)-1);

    if (gbl.server)
    {
        setup_server();
    }
    else if (gbl.outfile)
    {
        outfile = fopen(gbl.outfile, "w");
        if (outfile == NULL)
            err(1, "create %s", gbl.outfile);
    }

    gbl.hdr[0] = 0;
    gbl.hdr[sizeof(gbl.hdr)-1] = 0;
    if (gbl.server)
    {
        for (int i = 0; i < nfields; ++i)
            if (i != fTime)
                snprintf(gbl.hdr+strlen(gbl.hdr), sizeof(gbl.hdr)-1,
                    "%s=%g,%s,%g\n",
                    labels[i].name,
                    labels[i].max,
                    labels[i].units,
                    labels[i].factor);
        strncat(gbl.hdr, hdr, sizeof(gbl.hdr)-1);
    }
    else if (outfile)
    {
        for (int i = 0; i < nfields; ++i)
            if (i != fTime)
                fprintf(outfile, "%s=%g,%s,%g\n",
                    labels[i].name,
                    labels[i].max,
                    labels[i].units,
                    labels[i].factor);
        fprintf(outfile, hdr);
    }
    starttsc = _rdtsc();
}

struct metrics
{
    int nthreads;
    int ncores;
    uint64_t timestamp;
    double rbytes;
    double wbytes;
    double instrs;
    double vinstrs;
    double vpu_ea;
    double duration;
};

static double sevents[NEVENTS];

static struct summary
{
    double tserial;
    double tparallel;
    int pcount;
    uint64_t ncores;
    uint64_t threads;
    double instrs;
    double vinstrs;
    double vpu_ea;
    double rbytes;
    double wbytes;
    double clocks;
} summ;


void
sample(const struct metrics *s)
{
    double timestamp = (s->timestamp - starttsc) / gbl.hz;
    double duration = s->duration / gbl.hz;
    double ibw = s->instrs / duration;
    double vibw = s->vinstrs / duration;
    double rbw = s->rbytes / duration;
    double wbw = s->wbytes / duration;
    double vbw = s->vpu_ea / duration;
    double data[nfields], outdata[nfields];


    if (duration < .01)
        return;
    data[fTime] = timestamp;
    data[fThreads] = s->nthreads;
    data[fInst] = ibw;
    data[fVPU] = vibw;
    data[fMem] = rbw + wbw;
    data[fVpuSP] = vbw;
    data[fVpuDP] = vbw;

    char buffer[1024];
    buffer[1023] = 0;
    buffer[0] = 0;
    bool first = true;
    for (int i = 0; i < nfields; ++i)
    {
        if (first)
            first = false;
        else
            strncat(buffer, ",", sizeof(buffer)-1);
        snprintf(buffer+strlen(buffer), sizeof(buffer)-1, "%g", data[i]);
    }
    strncat(buffer, "\n", sizeof(buffer)-1);
    if (outfile)
        fputs(buffer, outfile);
    else if (gbl.server)
        send_server("%s", buffer);

    /* Compute summary information */
    if (s->nthreads <= 4)
        summ.tserial += s->duration / gbl.hz;
    else
    {
        summ.pcount += 1;
        summ.threads += s->nthreads;
        summ.tparallel += duration;
        summ.rbytes += s->rbytes;
        summ.wbytes += s->wbytes;
        summ.vpu_ea += s->vpu_ea;
        summ.instrs += s->instrs;
        summ.vinstrs += s->vinstrs;
        summ.ncores += s->ncores;
    }
}

static inline void
printlab(int f, double val)
{
    char ident[32];
    snprintf(ident, sizeof(ident), "  %-7s (%s)",
        labels[f].units, labels[f].name);
    printf( "%10.2f %-23s (%5.1f %% peak)\n",
        val*labels[f].factor, ident,
        (val/labels[f].max)*100.0);
}

void
printsummary(int pid, struct timeval *forktime, struct timeval *jointime)
{
    double elapsed;
    long utime, stime, nthreads;
    double dutime, dstime;
    struct rusage ru;

    elapsed = ((double)jointime->tv_sec + (double)jointime->tv_usec * 1e-6) -
              ((double)forktime->tv_sec + (double)forktime->tv_usec * 1e-6);

    getrusage(RUSAGE_CHILDREN, &ru);

    double t = summ.ncores;
    if (summ.pcount)
        t /= summ.pcount;
    int ncores = round(t);

    dutime = (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec * 1e-6;
    dstime = (double)ru.ru_stime.tv_sec + (double)ru.ru_stime.tv_usec * 1e-6;

    printf("\n");

    printf("Usage summary\n");

    double denom = dutime + dstime;
    if (denom == 0) denom = 1;
    printf(
    "%10.2f %-23s (%4.1f%% System, %4.1f%% User )\n",
        elapsed, "seconds Elapsed",
        (dstime / denom) * 100,
        (dutime / denom) * 100
        );

    printf("\n");

    printf(
    "%10.2f %-23s (%5d cores)\n",
        summ.tparallel, "seconds Parallel", ncores);
    if (summ.tparallel)
    {
        const double peakbw = 170.0;
        const double bw = (summ.rbytes+summ.wbytes)/summ.tparallel;
        const double ibw = summ.instrs/summ.tparallel;
        const double vibw = summ.vinstrs/summ.tparallel;
        const double vbw = summ.vpu_ea/summ.tparallel;

        printlab(fMem, bw);
        printlab(fInst, ibw);
        printlab(fVPU, vibw);
        printlab(fVpuSP, vbw);
        printlab(fVpuDP, vbw);
    }


    printf(
    "%10.2f %-23s\n",
        summ.tserial, "seconds Serial");

    if (gbl.verbose)
    {
        for (int i = 0; i < NEVENTS; ++i)
            printf("%20.0f raw 0x%x\n", sevents[i], evtcodes[i]);
    }
}

void
printcounters(struct counter *ctrs, uint64_t duration)
{
    struct metrics s = {0};

    s.timestamp = _rdtsc();
    s.duration = duration;
    // We skip the last core
    int corethreads =0;
    for (int cpu = 1; cpu < gbl.ncpus-3; ++cpu)
    {
        double delta[NEVENTS];
        // volatile because another thread is changing it.
        volatile struct counter *p = &ctrs[cpu];

        for (int i = 0; i < NEVENTS; ++i)
        {
            union {
                __m512d c;
                uint64_t values[8];
            } t;
            t.c = _mm512_load_pd((void *)&p->counts[i][0]);
            delta[i] = perf_scale_delta(t.values, lastctr[cpu].counts[i]);
            _mm512_storenrngo_pd((void *)&lastctr[cpu].counts[i][0], t.c);
            if (delta[i] < 0)
                delta[i] = 0;
            sevents[i] += delta[i];
        }

        if (2*delta[clocks1] > duration)
        {
            s.nthreads += 1;
            corethreads += 1;
        }

        if ((cpu % 4) == 0) // Last thread on this core
        {
            if (corethreads)
                s.ncores += 1;
            corethreads = 0;
        }

        s.vpu_ea += delta[vpu_ea];
        s.instrs += delta[instrs];
        s.vinstrs += delta[vpu_ie];
    }
    uint64_t nreads = 0, nwrites = 0;
    for (int i = 0; i < NGBOXES; ++i)
        for (int j = 0; j < 2; ++j)
        {
            nreads += pmu_rdctr(i, j, 0);
            nwrites += pmu_rdctr(i, j, 1);
        }
    s.rbytes = (nreads - prevnreads) * 64;
    s.wbytes = (nwrites - prevnwrites)* 64;
    prevnreads = nreads;
    prevnwrites = nwrites;

    sample(&s);
}

void
close_output()
{
    pmu_stop();
    pmu_fini();
    if (outfile)
        fclose(outfile);
}
