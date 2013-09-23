/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * Memory mapped IO PMUs from B 006A 0000 through 7D CC24
 * We'll say 8006A0000 through 8007E0000-1
 */

#define REGOFFS(name) ((name)-BASE)
#define BASE 0x8006A0000L
#define PMU_SIZE 1114112

#define NGBOXES 8

/* Use UIO to map each of these to a different address for security (so
 * we can't access anything else
 */

#define GBOX_MMIO_BASE0 0x8007A0000L
#define GBOX_MMIO_BASE1 0x800790000L
#define GBOX_MMIO_BASE2 0x800700000L
#define GBOX_MMIO_BASE3 0x8006F0000L
#define GBOX_MMIO_BASE4 0x8006D0000L
#define GBOX_MMIO_BASE5 0x8006C0000L
#define GBOX_MMIO_BASE6 0x8006B0000L
#define GBOX_MMIO_BASE7 0x8006A0000L

static uint64_t gbox_base[] = {
    GBOX_MMIO_BASE0,
    GBOX_MMIO_BASE1,
    GBOX_MMIO_BASE2,
    GBOX_MMIO_BASE3,
    GBOX_MMIO_BASE4,
    GBOX_MMIO_BASE5,
    GBOX_MMIO_BASE6,
    GBOX_MMIO_BASE7,
};


/* Gbox layout */
#define GBOX_FBOX_CONFIG 0xC
#define GBOX_FBOX_RESET(i)      (1<<i)
#define GBOX_FBOX_RESET_ALL     0xFF
#define GBOX_FBOX_IE(i)         (1<<(i+8))
#define GBOX_FBOX_INT           (1<<16)
#define GBOX_FBOX_ENABLE        (1<<17)

/* Events 0-4 */
#define GBOX_FBOX_CONFIG2 0xC8
union gbox_fbox_evtsel {
    struct {
        int evtsel0: 6;
        int evtsel1: 6;
        int evtsel2: 6;
        int evtsel3: 6;
        int evtsel4: 6;
    } s;
    int all;
};
/* Events 5-7 */
#define GBOX_FBOX_CONFIG3 0xCC
#define GBOX_FBOX_EMON0 0x14
#define GBOX_FBOX_EMON1 0x1C
#define GBOX_FBOX_EMON2 0x20
#define GBOX_FBOX_EMON3 0x24
#define GBOX_FBOX_EMON4 0x28
#define GBOX_FBOX_EMON5 0x2C
#define GBOX_FBOX_EMON6 0x30
#define GBOX_FBOX_EMON7 0x34

#define GBOX_FBOX_EMON_STAT 0x38

#define CH0_NORMAL_WRITE 0x02
#define CH0_NORMAL_READ 0x04
#define CH1_NORMAL_WRITE 0x0A
#define CH1_NORMAL_READ 0x0C

/*
 * Write config with 0xFF to clear and disable everything
 * Write config2 with 4 events CH[0|1][READ|WRITE]
 * Write config with bit 17 (only) to enable counting
 * Read EMON0-3 to get counts
 */


#ifdef __cplusplus
extern "C" {
#endif
void pmu_init();

void pmu_fini();

uint64_t pmu_rdctr(int controller, int channel, int type);

void pmu_start();

void pmu_stop();
#ifdef __cplusplus
}
#endif

