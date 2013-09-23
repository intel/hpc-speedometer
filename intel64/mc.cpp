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
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "pci.h"
#include "jkt.h"
#include "pcm.h"

char *CPUName(int);


// Jaketown per-socket memory read/write counter
class SocketMemCtrJKT
{
    PciHandleMM *handles[4];    // one per channel
    int m_bus;    // bus for this socket
    uint64_t lastvalRd[4], lastvalWr[4];
    uint64_t counterRd[4], counterWr[4];
public:
    SocketMemCtrJKT(int bus);
    ~SocketMemCtrJKT();
    void start();
    uint64_t bytesRead();
    uint64_t bytesWritten();
    static bool exists(int bus);
};


// derive from this to get read/write counters for specific cpu models
class AbstractMemCtr
{
public:
    virtual void init(int) = 0;
    virtual void start() = 0;
    virtual uint64_t bytesRead() = 0;
    virtual uint64_t bytesWritten() = 0;
};

class JKTMemCtr : public AbstractMemCtr
{
    SocketMemCtrJKT *memCtrs[4];
    int m_nsockets;
public:
    JKTMemCtr() : m_nsockets(0) {}
    ~JKTMemCtr()
    {
        for (int i = 0; i < m_nsockets; ++i)
            delete memCtrs[i];
    }
    void init(int nsockets)
    {
        static int busses[] = {0x3f, 0x7f, 0xbf, 0xff};
        m_nsockets = nsockets;
        assert(m_nsockets <= 4);
        int j = 0;
        for (int i = 0; i < 4; ++i)
            if (SocketMemCtrJKT::exists(busses[i]))
            {
                memCtrs[j] = new SocketMemCtrJKT(busses[i]);
                ++j;
            }
        assert(j == m_nsockets);
    }
    void start()
    {
        for (int i = 0; i < m_nsockets; ++i)
            memCtrs[i]->start();
    }
    uint64_t bytesRead()
    {
        uint64_t totalRead = 0;
        for (int i = 0; i < m_nsockets; ++i)
            totalRead += memCtrs[i]->bytesRead();
        return totalRead;
    }
    uint64_t bytesWritten()
    {
        uint64_t totalWritten = 0;
        for (int i = 0; i < m_nsockets; ++i)
            totalWritten += memCtrs[i]->bytesWritten();
        return totalWritten;
    }
};

PCM *PCM::instance = NULL;

uint64_t
PCM::bytesRead()
{
    return mem->bytesRead();
}

uint64_t
PCM::bytesWritten()
{
    return mem->bytesWritten();
}

PCM::ErrorCode
PCM::program()
{
    char buffer[1024];
    PCM_CPUID_INFO cpuinfo;

    pcm_cpuid(0, cpuinfo);
    memset(buffer, 0, 1024);
    ((int *)buffer)[0] = cpuinfo.array[1];
    ((int *)buffer)[1] = cpuinfo.array[3];
    ((int *)buffer)[2] = cpuinfo.array[2];
    if (strncmp(buffer, "GenuineIntel", 4 * 3) != 0)
    {
        fprintf(stderr, "Genuine Intel processor required\n");
        return Failure;
    }
    max_cpuid = cpuinfo.array[0];

    pcm_cpuid(1, cpuinfo);
    cpu_family = (((cpuinfo.array[0]) >> 8) & 0xf) | ((cpuinfo.array[0] & 0xf00000) >> 16);
    cpu_model = (((cpuinfo.array[0]) & 0xf0) >> 4) | ((cpuinfo.array[0] & 0xf0000) >> 12);
    if (cpu_family != 6)
    {
        fprintf(stderr, "cpu_family != 6\n");
        return Failure;
    }
    parseCpuinfo();
    switch (cpu_model)
    {
    case JAKETOWN:
        mem = new JKTMemCtr();
        mem->init(g_nsockets);
        mem->start();
        break;
    default:
        fprintf(stderr, "CPU Model %s not currently supported\n",
            CPUName(cpu_model));
        return Failure;
    }
    return Success;
}


static int
int_after_colon(const char *str)
{
    const char *p = strchr(str, ':');
    assert(p != NULL);

    char *q;
    int i = strtol(p+1, &q, 10);
    assert(q != p+1);

    return i;
}

int
PCM::findcore(int physcore, int package)
{
    for (int i = 0; i < g_ncores; ++i)
        if (g_cores[i].physical_core == physcore &&
            g_cores[i].package == package)
            return i;
    int core = g_ncores++;
    assert(core < maxCores);
    g_cores[core].physical_core = physcore;
    g_cores[core].package = package;
    return core;
}

void
PCM::parseCpuinfo()
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    char buf[1024];
    const char *procstr = "processor";
    const char *packagestr = "physical id";
    const char *corestr = "core id";
    int proc, package, physcore;
    int core;
    assert(fp != 0);

    g_nsockets = 0;
    g_nprocs = 0;
    g_ncores = 0;
    memset(g_cores, 0, sizeof(g_cores));
    memset(g_procs, 0, sizeof(g_procs));
    /* We assume that /proc/cpuinfo is in processor order. */
    buf[sizeof(buf)-1] = 0;
    while (fgets(buf, sizeof(buf)-2, fp) != 0)
    {
        if (strncmp(buf, procstr, sizeof(procstr)-1) == 0)
        {
            proc = int_after_colon(buf);
            assert(proc == g_nprocs);
        }
        else if (strncmp(buf, packagestr, sizeof(packagestr)-1) == 0)
        {
            package = int_after_colon(buf);
            if (g_nsockets < package+1)
                g_nsockets = package+1;
        }
        else if (strncmp(buf, corestr, sizeof(corestr)-1) == 0)
        {
            physcore = int_after_colon(buf);
            core = findcore(physcore, package);
            assert(g_nprocs < maxProcs);
            assert(g_cores[core].nprocs < contextsPerCore);
            g_procs[g_nprocs++] = core;
            g_cores[core].procs[g_cores[core].nprocs++] = proc;
        }
    }
    fclose(fp);

#if 0
    printf("%d procs, %d cores\n", g_nprocs, g_ncores);
    for (int i = 0; i < g_nprocs; ++i)
    {
        int core = g_procs[i];
        printf("proc %3d: core %2d physcore %2d package %d procs:",
                i, core, g_cores[core].physical_core, g_cores[core].package);
        for (int j = 0; j < g_cores[core].nprocs; ++j)
            printf(" %d", g_cores[core].procs[j]);
        printf("\n");
    }
#endif

}




bool
SocketMemCtrJKT::exists(int bus)
{
    char buffer[1024];
    struct stat buf;
    int ret;

    sprintf(buffer, "/sys/devices/pci0000:%02x", bus);
    ret = stat(buffer, &buf);
    if (ret != 0)
        return false;
    if (!S_ISDIR(buf.st_mode))
        return false;
    return true;
}

SocketMemCtrJKT::SocketMemCtrJKT(int bus)
{
    m_bus = bus;
    handles[0] = new PciHandleMM(0, bus, MC_CH0_REGISTER_DEV,
        MC_CH0_REGISTER_FUNC);
    handles[1] = new PciHandleMM(0, bus, MC_CH1_REGISTER_DEV,
        MC_CH1_REGISTER_FUNC);
    handles[2] = new PciHandleMM(0, bus, MC_CH2_REGISTER_DEV,
        MC_CH2_REGISTER_FUNC);
    handles[3] = new PciHandleMM(0, bus, MC_CH3_REGISTER_DEV,
        MC_CH3_REGISTER_FUNC);
    for (int i = 0; i < 4; ++i)
    {
        lastvalRd[i] = 0;
        lastvalWr[i] = 0;
        counterRd[i] = 0;
        counterWr[i] = 0;
    }
}

SocketMemCtrJKT::~SocketMemCtrJKT()
{
    for (int i = 0; i < 4; ++i)
        delete handles[i];
}

void
SocketMemCtrJKT::start()
{
    for (int i = 0; i < 4; ++i)
    {
	// imc PMU

	// freeze enable
	handles[i]->write32(MC_CH_PCI_PMON_BOX_CTL,
			    MC_CH_PCI_PMON_BOX_CTL_FRZ_EN);
	// freeze
	handles[i]->write32(MC_CH_PCI_PMON_BOX_CTL,
			    MC_CH_PCI_PMON_BOX_CTL_FRZ_EN +
			    MC_CH_PCI_PMON_BOX_CTL_FRZ);

	uint32_t val = 0;
	handles[i]->read32(MC_CH_PCI_PMON_BOX_CTL, &val);
	if (val !=
	    (MC_CH_PCI_PMON_BOX_CTL_FRZ_EN + MC_CH_PCI_PMON_BOX_CTL_FRZ))
	{
            fprintf(stderr,
		"ERROR: IMC counter programming seems not to work. MC_CH%d"
		"_PCI_PMON_BOX_CTL=0x%x\n",
                i, val);
            fprintf(stderr,
		"       Please see BIOS options to enable the export of performance monitoring devices.\n");
	}

	// monitor reads on counter 0
	handles[i]->write32(MC_CH_PCI_PMON_CTL0,
			    MC_CH_PCI_PMON_CTL_EN +
			    MC_CH_PCI_PMON_CTL_EVENT(0x04) +
			    MC_CH_PCI_PMON_CTL_UMASK(3));

	// monitor writes on counter 1
	handles[i]->write32(MC_CH_PCI_PMON_CTL1,
			    MC_CH_PCI_PMON_CTL_EN +
			    MC_CH_PCI_PMON_CTL_EVENT(0x04) +
			    MC_CH_PCI_PMON_CTL_UMASK(12));

        // zero the values
	handles[i]->write64(MC_CH_PCI_PMON_CTR0, 0L);
	handles[i]->write64(MC_CH_PCI_PMON_CTR1, 0L);

	// unfreeze counters
	handles[i]->write32(MC_CH_PCI_PMON_BOX_CTL,
			    MC_CH_PCI_PMON_BOX_CTL_FRZ_EN);
    }
}


static inline uint64_t
readctr(uint64_t &last, uint64_t &ctr, PciHandleMM *h, int reg)
{
    // handle 48-bit overflow
    uint64_t top = (1L << 48) - 1;
    uint64_t t, u;
    h->read64(reg, &t);
    u = last;
    if (t >= u)
        ctr += t - u;
    else
    {
        ctr += top - u;
        ctr += t;
    }
    last = ctr;
    return ctr;
}

uint64_t 
SocketMemCtrJKT::bytesRead()
{
    uint64_t result = 0;
    for (int i = 0; i < 4; ++i)
        result += readctr(lastvalRd[i], counterRd[i],
            handles[i], MC_CH_PCI_PMON_CTR0);
    return result * 64;
}

uint64_t 
SocketMemCtrJKT::bytesWritten()
{
    uint64_t result = 0;
    for (int i = 0; i < 4; ++i)
        result += readctr(lastvalWr[i], counterWr[i],
            handles[i], MC_CH_PCI_PMON_CTR1);
    return result * 64;
}

char *
CPUName(int model)
{
    switch (model)
    {
    case PCM::NEHALEM_EP: return "Nehalem EP";
    case PCM::NEHALEM_EP_2: return "Nehalem EP 2";
    case PCM::ATOM: return "Atom";
    case PCM::ATOM_2: return "Atom 2";
    case PCM::ATOM_3: return "Centerton";
    case PCM::CLARKDALE: return "Clarkdale";
    case PCM::WESTMERE_EP: return "Westmere EP";
    case PCM::NEHALEM_EX: return "Nehalem EX";
    case PCM::WESTMERE_EX: return "Westmere EX";
    case PCM::SANDY_BRIDGE: return "Sandybridge";
    case PCM::JAKETOWN: return "Jaketown";
    case PCM::IVY_BRIDGE: return "Ivybridge";
    case PCM::HASWELL: return "Haswell";
    case PCM::HASWELL_2: return "Haswell 2";
    default: return "Unknown";
    };
}

#if 0
int
main(int argc, char **argv)
{
    PCM *pcm = PCM::getInstance();
    PCM::ErrorCode status = pcm->program();
    if (status != PCM::Success)
    {
        fprintf(stderr, "Failed to program PMU\n");
    }
    printf("#cpus: %d; #cores: %d; #sockets: %d; #tpc: %d\n",
        pcm->getNumProcs(),
        pcm->getNumCores(),
        pcm->getNumSockets(),
        pcm->getThreadsPerCore());
    for (int i = 0; i <= 10; ++i)
    {
        uint64_t nr, nw;
        nr = pcm->bytesRead();
        nw = pcm->bytesWritten();
        double nrm = (double)nr * 1e-6;
        double nwm = (double)nw * 1e-6;
        printf("%d: nr %.2f nw %.2f\n", i, nrm, nwm);
        sleep(1);
    }
}
#endif
