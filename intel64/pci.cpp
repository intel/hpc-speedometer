/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// written by Roman Dementiev,
//            Pat Fay
//	      Austen Ott
//            Jim Harris (FreeBSD)

// Hacked by Larry Meadows for Speedometer

#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "pci.h"

#include <sys/mman.h>
#include <errno.h>

// mmaped I/O version

uint64_t read_base_addr(int mcfg_handle, uint32_t group_, uint32_t bus)
{
    MCFGRecord record;
    int32_t result = ::pread(mcfg_handle, (void *)&record, sizeof(MCFGRecord), sizeof(MCFGHeader) + sizeof(MCFGRecord)*group_ );

    if (result != sizeof(MCFGRecord))
    {
        ::close(mcfg_handle);
        throw std::exception();
    }

    const unsigned max_bus = (unsigned)record.endBusNumber ;
    if(bus > max_bus)
    {
       std::cout << "ERROR: Requested bus number "<< bus<< " is larger than the max bus number " << max_bus << std::endl;
       ::close(mcfg_handle);
       throw std::exception();
    }

    // std::cout << "DEBUG: PCI segment group="<<group_<<" number="<< record.PCISegmentGroupNumber << std::endl;

    return record.baseAddress;
}

PciHandleMM::PciHandleMM(uint32_t groupnr_, uint32_t bus_, uint32_t device_, uint32_t function_) :
    fd(-1),
    mmapAddr(NULL),
    bus(bus_),
    device(device_),
    function(function_),
    base_addr(0)
{
    int handle = ::open("/dev/mem", O_RDWR);
    if (handle < 0) throw std::exception();
    fd = handle;

    int mcfg_handle = ::open("/sys/firmware/acpi/tables/MCFG", O_RDONLY);

    if (mcfg_handle < 0) throw std::exception();

    if(groupnr_ == 0)
    {
        base_addr = read_base_addr(mcfg_handle, 0, bus);
    }
    else if(groupnr_ >= 0x1000)
    {
        // for SGI UV2
        MCFGHeader header;
        ::read(mcfg_handle, (void *)&header, sizeof(MCFGHeader));
        const unsigned segments = header.nrecords();
        for(unsigned int i=0; i<segments;++i)
        {
            MCFGRecord record;
            ::read(mcfg_handle, (void *)&record, sizeof(MCFGRecord));
            if(record.PCISegmentGroupNumber == 0x1000)
            {
                base_addr = record.baseAddress + (groupnr_ - 0x1000)*(0x4000000);
                break;
            }
        }

        if(base_addr == 0)
        {
            std::cout << "ERROR: PCI Segment 0x1000 not found." << std::endl;
            throw std::exception();
        }

    } else
    {
        std::cout << "ERROR: Unsupported PCI segment group number "<< groupnr_ << std::endl;
        throw std::exception();
    }

    // std::cout << "DEBUG: PCI config base addr: 0x"<< std::hex << base_addr<< " for groupnr " << std::dec << groupnr_ << std::endl;

    ::close(mcfg_handle);

    base_addr += (bus * 1024 * 1024 + device * 32 * 1024 + function * 4 * 1024);
    
    mmapAddr = (char*) mmap(NULL, 4096, PROT_READ| PROT_WRITE, MAP_SHARED , fd, base_addr);
    
    if(mmapAddr == MAP_FAILED)
    {
      std::cout << "mmap failed: errno is "<< errno<<  std::endl;
      throw std::exception();
    }
}

bool PciHandleMM::exists(uint32_t bus_, uint32_t device_, uint32_t function_)
{
    int handle = ::open("/dev/mem", O_RDWR);

    if (handle < 0) return false;

    ::close(handle);

    handle = ::open("/sys/firmware/acpi/tables/MCFG", O_RDONLY);

    if (handle < 0) return false;

    ::close(handle);

    return true;
}

int32_t PciHandleMM::read32(uint64_t offset, uint32_t * value)
{
    *value = *((uint32_t*)(mmapAddr+offset));

    return sizeof(uint32_t);
}

int32_t PciHandleMM::write32(uint64_t offset, uint32_t value)
{
    *((uint32_t*)(mmapAddr+offset)) = value;

    return sizeof(uint32_t);
}

int32_t PciHandleMM::read64(uint64_t offset, uint64_t * value)
{
    *value = *((uint64_t*)(mmapAddr+offset));

    return sizeof(uint64_t);
}

int32_t PciHandleMM::write64(uint64_t offset, uint64_t value)
{
    *((uint64_t*)(mmapAddr+offset)) = value;

    return sizeof(uint64_t);
}

PciHandleMM::~PciHandleMM()
{
    if (mmapAddr) munmap(mmapAddr, 4096);
    if (fd >= 0) ::close(fd);
}

