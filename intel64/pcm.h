/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _PCM_H
#define _PCM_H

class AbstractMemCtr;
class PCM
{

    union PCM_CPUID_INFO
    {
        int array[4];
        struct
        {
            int eax, ebx, ecx, edx;
        } reg;
    };

    enum
    {
        contextsPerCore = 4,
        maxCores = 512,
        maxProcs = 1024,
    };

    static void
    pcm_cpuid(int leaf, PCM_CPUID_INFO & info)
    {
        __asm__ __volatile__("cpuid":"=a"(info.reg.eax), "=b"(info.reg.ebx),
                             "=c"(info.reg.ecx), "=d"(info.reg.edx):"a"(leaf));
    }
    int cpu_family, cpu_model;
    int max_cpuid;
    // Sorry for the 'g_', I stole this code from another library
    int g_ncores;
    int g_nprocs;
    int g_nsockets;
    struct coreInfo
    {
        int procs[contextsPerCore];
        int nprocs;
        int physical_core;
        int package;
    };

    coreInfo g_cores[maxCores];
    int g_procs[maxProcs];
    static PCM *instance;
    int findcore(int physcore, int package);
    void parseCpuinfo();
    AbstractMemCtr *mem;
public:
    enum SupportedCPUModels
    {
        NEHALEM_EP = 26,
        NEHALEM_EP_2 = 30,
        ATOM = 28,
        ATOM_2 = 53,
        ATOM_3 = 54, // Centerton
        CLARKDALE = 37,
        WESTMERE_EP = 44,
        NEHALEM_EX = 46,
        WESTMERE_EX = 47,
        SANDY_BRIDGE = 42,
        JAKETOWN = 45,
        IVY_BRIDGE = 58,
        HASWELL = 60,
        HASWELL_2 = 70,
        END_OF_MODEL_LIST
    };
    enum ErrorCode {Success, Failure};
    PCM() : cpu_family(0), cpu_model(0) {}
    static PCM *getInstance()
    {
        if (instance)
            return instance;
        return instance = new PCM();
    }
    ErrorCode program();
    int getCPUModel() { return cpu_model; }
    int getNumProcs() { return g_nprocs; }
    int getNumCores() { return g_ncores; }
    int getThreadsPerCore() { return g_nprocs / g_ncores; }
    int getNumSockets() { return g_nsockets; }
    int getSocketId(int cpu) { return g_cores[g_procs[cpu]].package; }
    int getCoreId(int cpu) { return g_cores[g_procs[cpu]].physical_core; }
    int getModel() { return cpu_model; }
    uint64_t bytesRead();
    uint64_t bytesWritten();
};
#endif
