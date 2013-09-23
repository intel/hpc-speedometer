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

#include "pcm.h"
#include "metrics.h"
#include "perf_utils.h"

#include "fields.h"

static uint64_t starttsc;

static FILE *outfile;

static int *threadspercore; 

extern PCM *pcm;

static uint64_t lastBytesRead, lastBytesWritten;

static char *hdrLabels[nfields];
static int hdrIndexes[nfields];

void
setup_output()
{
    char hdr[1024];
    hdr[sizeof(hdr)-1] = 0;
    hdr[0] = 0;

    lastBytesRead = pcm->bytesRead();
    lastBytesWritten = pcm->bytesWritten();
    threadspercore = new int[gbl.ncores];
    labels[fThreads].max = gbl.ncpus;
    labels[fInst].max = gbl.ncores * 4 * gbl.hz;
    labels[fFlopsSP].max = gbl.ncores * 8 * 2 * gbl.hz;
    labels[fFlopsDP].max = gbl.ncores * 4 * 2 * gbl.hz;

    hdrLabels[0] = "Time";
    hdrLabels[1] = "Threads";
    hdrIndexes[0] - fTime;
    hdrIndexes[1] = fThreads;
    int j = 2;
    for (int i = 0; i < nfields; ++i)
        if (i != fTime && i != fThreads)
        {
            hdrLabels[j] = labels[i].name;
            hdrIndexes[j] = i;
            ++j;
        }
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
    double instrs;
    double dsimd, dsse, dscalar;
    double ssimd, ssse, sscalar;
    double rbytes;
    double wbytes;
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
    double dsimd, dsse, dscalar;
    double ssimd, ssse, sscalar;
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
    double rbw = s->rbytes / duration;
    double wbw = s->wbytes / duration;
    double dflops, sflops;
    double data[nfields];

    if (duration < .01)
        return;
    dflops =  (s->dsimd * 4 + s->dsse * 2 + s->dscalar) / duration;
    sflops =  (s->ssimd * 8 + s->ssse * 4 + s->sscalar) / duration;
    data[fTime] = timestamp;
    data[fThreads] = s->nthreads;
    data[fInst] = ibw;
    data[fMem] = rbw + wbw;
    data[fFlopsSP] = sflops;
    data[fFlopsDP] = dflops;

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
        int field = hdrIndexes[i];
        if (field != fTime && field != fThreads &&
            data[field] > 1.5*labels[field].max)
            data[i] = 0;        // saw this on SNB with FP...
        snprintf(buffer+strlen(buffer), sizeof(buffer)-1, "%g", data[i]);
    }
    strncat(buffer, "\n", sizeof(buffer)-1);
    if (outfile)
        fputs(buffer, outfile);
    else if (gbl.server)
        send_server("%s", buffer);

    /* Compute summary information */
    if (s->nthreads <= 2)
        summ.tserial += s->duration / gbl.hz;
    else
    {
        summ.pcount += 1;
        summ.threads += s->nthreads;
        summ.tparallel += duration;
        summ.instrs += s->instrs;
        summ.dsimd += s->dsimd;
        summ.dsse += s->dsse;
        summ.dscalar += s->dscalar;
        summ.ssimd += s->ssimd;
        summ.ssse += s->ssse;
        summ.sscalar += s->sscalar;
        summ.rbytes += s->rbytes;
        summ.wbytes += s->wbytes;
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

    printf(
    "%10.2f %-23s (%4.1f%% System, %4.1f%% User )\n",
        elapsed, "seconds Elapsed",
        (dstime / (dutime+dstime)) * 100,
        (dutime / (dutime+dstime)) * 100
        );

    printf("\n");

    printf(
    "%10.2f %-23s (%5d cores)\n",
        summ.tparallel, "seconds Parallel", ncores);
    if (summ.tparallel)
    {
        const double peakbw = 70.0;
        const double bw = (summ.rbytes+summ.wbytes)/summ.tparallel;
        const double ibw = summ.instrs/summ.tparallel;
        double dflops, sflops;
        dflops =  (summ.dsimd * 4 + summ.dsse * 2 + summ.dscalar);
        dflops = dflops / summ.tparallel;
        sflops =  (summ.ssimd * 8 + summ.ssse * 4 + summ.sscalar);
        sflops = sflops / summ.tparallel;

        printlab(fMem, bw);
        printlab(fInst, ibw);
        printlab(fFlopsDP, dflops);
        printf(
    "          %10.2f AVX\n", summ.dsimd*4/summ.tparallel*1e-9);
        printf(
    "          %10.2f SSE\n", summ.dsse*2/summ.tparallel*1e-9);
        printf(
    "          %10.2f Scalar\n", summ.dscalar*1/summ.tparallel*1e-9);
        printlab(fFlopsSP, sflops);
        printf(
    "          %10.2f AVX\n", summ.ssimd*8/summ.tparallel*1e-9);
        printf(
    "          %10.2f SSE\n", summ.ssse*4/summ.tparallel*1e-9);
        printf(
    "          %10.2f Scalar\n", summ.sscalar*1/summ.tparallel*1e-9);
    }


    printf(
    "%10.2f %-23s\n",
        summ.tserial, "seconds Serial");

    if (gbl.verbose)
    {
        for (int i = 0; i < NEVENTS; ++i)
            printf("%20.0f 0x%x\n", sevents[i], evcodes[i].code);
    }
}

void
printcounters(struct counter *ctrs, uint64_t duration)
{
    struct metrics s = {0};

    uint64_t thisBytesWritten = pcm->bytesWritten();
    uint64_t thisBytesRead = pcm->bytesRead();
    memset(threadspercore, 0, gbl.ncores * sizeof(int));
    s.timestamp = _rdtsc();
    s.duration = duration;
    for (int cpu = 0; cpu < gbl.ncpus; ++cpu)
    {
        double delta[NEVENTS];
        // volatile because another thread is changing it.
        volatile struct counter *p = &ctrs[cpu];

        for (int i = 0; i < NEVENTS; ++i)
        {
            union {
                __m256d c;
                uint64_t values[4];
            } t;
            t.c = _mm256_load_pd((const double *)&p->counts[i][0]);
            delta[i] = perf_scale_delta(t.values, lastctr[cpu].counts[i]);
            _mm256_store_pd((double *)&lastctr[cpu].counts[i][0], t.c);
            if (delta[i] < 0)
                delta[i] = 0;
            sevents[i] += delta[i];
        }

        //printf("clocks %g duration %lu\n", delta[clocks], duration);
        if (2*delta[clocks] > duration)
        {
            int thiscore = pcm->getSocketId(cpu)  * gbl.corespersocket +
                pcm->getCoreId(cpu);
            ++s.nthreads;
            ++threadspercore[thiscore];
        }
        s.dsimd += delta[simd_dp];
        s.dsse += delta[sse_dp];
        s.dscalar += delta[scalar_dp];
        s.ssimd += delta[simd_sp];
        s.ssse += delta[sse_sp];
        s.sscalar += delta[scalar_sp];
        s.instrs += delta[instrs];
    }
    s.rbytes = thisBytesRead - lastBytesRead;
    s.wbytes = thisBytesWritten - lastBytesWritten;
    lastBytesRead = thisBytesRead;
    lastBytesWritten = thisBytesWritten;
    for (int i = 0; i < gbl.ncores; ++i)
        if (threadspercore[i])
            ++s.ncores;

    sample(&s);
}

void
close_output()
{
    if (outfile)
        fclose(outfile);
}
