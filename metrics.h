/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "perf_utils.h"
#ifndef METRICS_H
#define METRICS_H

struct gbl
{
    int server; // is it a server
    int ncpus;
    int ncores;
    int nsockets;
    int corespersocket;
    int verbose;
    char *outfile;
    int port;
    double hz;
    char hdr[4096];
};

extern struct gbl gbl;

#if defined(KNC)
/*
 * Events for KNC
 * These are arranged so that they fit in groups. I suppose we could use 
 * PERF_FORMAT_GROUP to get a little more consistency. If we have an extra
 * whole, we fill it in with clocks to improve that event a little.
 *
 * KNC Metrics:
 *    CPI = INSTRUCTIONS_EXECUTED / CPU_CLK_UHALTED
 *    Vector Intensity = VPU_ELEMENTS_ACTIVE/VPU_INSTRUCTIONS_EXECUTED
 *    Bytes read = (L2_READ_MISS - L2_WEAKLY_ORDERED_STREAMING_VSTORE_MISS) * 64
 *    Bytes written = L2_VICTIM_REQ_WITH_DATA * 64
 *    Threads active = CPU_CLK_UNHALTED / Elapsed Time
 *
 *    This is made vastly more complicated, because
 *    L2_VICTIM_REQ_WITH_DATA is per core so need to divide it by the number
 *    of threads per core active. This is the elapsed time divided by the
 *    cycle count per core. So we need to collect counts on all CPUs.
 */

#if 0
#define NEVENTS 8
static int evtcodes[NEVENTS] =
{
    0x2016,     // VPU_INSTRUCTIONS_EXECUTED
    0x2018,     // VPU_ELEMENTS_ACTIVE
    0x10CB,     // L2_READ_MISS
    0x0016,     // INSTRUCTIONS_EXECUTED
    0x10D7,     // L2_VICTIM_REQ_WITH_DATA
    0x002A,     // CPU_CLK_UNHALTED
    0x10CF,     // L2_WEAKLY_ORDERED_STREAMING_VSTORE_MISS
    0x002A,     // CPU_CLK_UNHALTED
};


enum evtnames {
    vpu_ie,
    vpu_ea,
    l2_miss,
    instrs,
    l2_vic,
    clocks1,
    vstore,
    clocks2,
};
#else
#define NEVENTS 4
static int evtcodes[NEVENTS] =
{
    0x2016,     // VPU_INSTRUCTIONS_EXECUTED
    0x2018,     // VPU_ELEMENTS_ACTIVE
    0x002A,     // CPU_CLK_UNHALTED
    0x0016,     // INSTRUCTIONS_EXECUTED
};


enum evtnames {
    vpu_ie,
    vpu_ea,
    clocks1,
    instrs,
};
#endif
#endif

#if defined(INTEL64)
#define NEVENTS 8
#define HEADER   "Time,threads,instrs,dflops,sflops,rbw,wbw"

/* These are SNB only, need a switch table */

static struct {
    int type;
    int code;
} evcodes[NEVENTS] =
{
    {PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS},
    {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES},
    {PERF_TYPE_RAW, 0x0111},      //SIMD_FP_256.PACKED_SINGLE
    {PERF_TYPE_RAW, 0x0211},      //SIMD_FP_256.PACKED_DOUBLE
    {PERF_TYPE_RAW, 0x1010},      //FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE
    {PERF_TYPE_RAW, 0x2010},      //FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE
    {PERF_TYPE_RAW, 0x4010},      //FP_COMP_OPS_EXE.SSE_PACKED_SINGLE
    {PERF_TYPE_RAW, 0x8010},      //FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE
};

enum evtnames {
    instrs,
    clocks,
    simd_sp,
    simd_dp,
    sse_dp,
    scalar_sp,
    sse_sp,
    scalar_dp
};



#endif

// For now this is readonly after allocation. If it changes, should
// pad it to 64 bytes
struct evinfo
{
    int fd;
    void *buf;
};

// Keep this at multiple of 64 bytes
struct counter
{
     uint64_t counts[NEVENTS][8];
};


#ifdef __cplusplus
extern "C" {
#endif
void setup(int pid);
int monitor(int pid, struct timeval *jointime);
void printsummary(int pid, struct timeval *forktime, struct timeval *jointime);
void jointhreads(void);
void closefiles();
void setup_server();
void send_server(const char *format, ...);
#ifdef __cplusplus
}
#endif

void printcounters(struct counter *ctrs, uint64_t duration);
void printtotals();
void setup_output();
void close_output();

extern struct counter *lastctr;
extern int *coremap;

#endif
