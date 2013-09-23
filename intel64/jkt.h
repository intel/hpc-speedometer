/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _JKT_H
#define _JKT_H

// JKT uncore counters

#define MC_CH0_REGISTER_DEV (16)
#define MC_CH1_REGISTER_DEV (16)
#define MC_CH2_REGISTER_DEV (16)
#define MC_CH3_REGISTER_DEV (16)
#define MC_CH0_REGISTER_FUNC (0)
#define MC_CH1_REGISTER_FUNC (1)
#define MC_CH2_REGISTER_FUNC (4)
#define MC_CH3_REGISTER_FUNC (5)

#define MC_CH_PCI_PMON_BOX_CTL (0x0F4)

#define MC_CH_PCI_PMON_BOX_CTL_RST_CONTROL 	(1<<0)
#define MC_CH_PCI_PMON_BOX_CTL_RST_COUNTERS 	(1<<1)
#define MC_CH_PCI_PMON_BOX_CTL_FRZ 	(1<<8)
#define MC_CH_PCI_PMON_BOX_CTL_FRZ_EN 	(1<<16)

#define MC_CH_PCI_PMON_FIXED_CTL (0x0F0)

#define MC_CH_PCI_PMON_FIXED_CTL_RST (1<<19) 
#define MC_CH_PCI_PMON_FIXED_CTL_EN (1<<22) 

#define MC_CH_PCI_PMON_CTL3 (0x0E4)
#define MC_CH_PCI_PMON_CTL2 (0x0E0)
#define MC_CH_PCI_PMON_CTL1 (0x0DC)
#define MC_CH_PCI_PMON_CTL0 (0x0D8)

#define MC_CH_PCI_PMON_CTL_EVENT(x) (x<<0)
#define MC_CH_PCI_PMON_CTL_UMASK(x) (x<<8)
#define MC_CH_PCI_PMON_CTL_RST (1<<17)
#define MC_CH_PCI_PMON_CTL_EDGE_DET (1<<18)
#define MC_CH_PCI_PMON_CTL_EN (1<<22)
#define MC_CH_PCI_PMON_CTL_INVERT (1<<23)
#define MC_CH_PCI_PMON_CTL_THRESH(x) (x<<24UL)

#define MC_CH_PCI_PMON_FIXED_CTR (0x0D0)

#define MC_CH_PCI_PMON_CTR3 (0x0B8)
#define MC_CH_PCI_PMON_CTR2 (0x0B0)
#define MC_CH_PCI_PMON_CTR1 (0x0A8)
#define MC_CH_PCI_PMON_CTR0 (0x0A0)
#endif
