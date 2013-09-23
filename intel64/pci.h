/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// written by Roman Dementiev
//            Pat Fay
//            Jim Harris (FreeBSD)


// hacked by Larry Meadows for speedometer

#ifndef CPUCounters_PCI_H
#define CPUCounters_PCI_H

/*!     \file pci.h
        \brief Low level interface to access PCI configuration space

*/

#include <unistd.h> 
#include <stdint.h>


struct MCFGRecord
{
    unsigned long long baseAddress;
    unsigned short PCISegmentGroupNumber;
    unsigned char startBusNumber;
    unsigned char endBusNumber;
    char reserved[4];
};

struct MCFGHeader
{
    char signature[4];
    unsigned length;
    unsigned char revision;
    unsigned char checksum;
    char OEMID[6];
    char OEMTableID[8];
    unsigned OEMRevision;
    unsigned creatorID;
    unsigned creatorRevision;
    char reserved[8];

    unsigned nrecords() const
    {
        return (length - sizeof(MCFGHeader))/sizeof(MCFGRecord);
    }

};

// read/write PCI config space using physical memory using mmaped file I/O
class PciHandleMM
{
    int32_t fd;
    char * mmapAddr;

    uint32_t bus;
    uint32_t device;
    uint32_t function;
    uint64_t base_addr;

    PciHandleMM();             // forbidden
    PciHandleMM(PciHandleMM &); // forbidden

public:
    PciHandleMM(uint32_t groupnr_, uint32_t bus_, uint32_t device_, uint32_t function_);

    static bool exists(uint32_t bus_, uint32_t device_, uint32_t function_);

    int32_t read32(uint64_t offset, uint32_t * value);
    int32_t write32(uint64_t offset, uint32_t value);

    int32_t read64(uint64_t offset, uint64_t * value);
    int32_t write64(uint64_t offset, uint64_t value);

    virtual ~PciHandleMM();
};


#endif
