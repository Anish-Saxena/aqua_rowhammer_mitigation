# Copyright (c) 2012 ARM Limited
# Copyright (c) 2020 Barkhausen Institut
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2007 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function
from __future__ import absolute_import

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.params import AddrRange, AllMemory, MemorySize
from m5.util.convert import toMemorySize

############################################################################
# RQ's cache config
############################################################################

class PrefetchCache(Cache):

    def __init__(self, options = None):
        super(PrefetchCache, self).__init__()
        if not options or options.no_prefetchers:
            return
        self.prefetcher = StridePrefetcher()

class RQ_L1Cache(PrefetchCache):
    """Simple L1 Cache with default values"""

    assoc = 8
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 16
    tgts_per_mshr = 20
    writeback_clean = True

    def __init__(self):
        super(RQ_L1Cache, self).__init__()

    def connectBus(self, bus):
        """Connect this cache to a memory-side bus"""
        self.mem_side = bus.cpu_side_ports

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU-side port
           This must be defined in a subclass"""
        raise NotImplementedError

class RQ_L1ICache(RQ_L1Cache):
    """Simple L1 instruction cache with default values"""

    # Set the default size
    size = '32kB'

    def __init__(self):
        super(RQ_L1ICache, self).__init__()

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU icache port"""
        self.cpu_side = cpu.icache_port

class RQ_L1DCache(RQ_L1Cache):
    """Simple L1 data cache with default values"""

    # Set the default size
    size = '32kB'

    def __init__(self):
        super(RQ_L1DCache, self).__init__()

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU dcache port"""
        self.cpu_side = cpu.dcache_port

class RQ_MMUCache(Cache):
    # Default parameters
    size = '8kB'
    assoc = 4
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 20
    tgts_per_mshr = 12
    writeback_clean = True

    def __init__(self):
        super(RQ_MMUCache, self).__init__()

    def connectCPU(self, cpu):
        """Connect the CPU itb and dtb to the cache
           Note: This creates a new crossbar
        """
        self.mmubus = L2XBar()
        self.cpu_side = self.mmubus.mem_side_ports
        for tlb in [cpu.itb, cpu.dtb]:
            self.mmubus.cpu_side_ports = tlb.walker.port

    def connectBus(self, bus):
        """Connect this cache to a memory-side bus"""
        self.mem_side = bus.cpu_side_ports

class RQ_L2Cache(PrefetchCache):
    """Simple L2 Cache with default values"""

    # Default parameters
    size = '256kB'
    assoc = 16
    tag_latency = 10
    data_latency = 10
    response_latency = 1
    mshrs = 20
    tgts_per_mshr = 12
    writeback_clean = True

    def __init__(self):
        super(RQ_L2Cache, self).__init__()

    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports

class RQ_L3Cache(Cache):
    """Simple L3 Cache bank with default values
       This assumes that the L3 is made up of multiple banks. This cannot
       be used as a standalone L3 cache.
    """

    # Default parameters
    assoc = 16
    tag_latency = 40
    data_latency = 40
    response_latency = 20
    mshrs = 256
    tgts_per_mshr = 12
    clusivity = 'mostly_excl'

    size = '4MB'

    def __init__(self):
        super(RQ_L3Cache, self).__init__()

    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports

# Base implementations of L1, L2, IO and TLB-walker caches. There are
# used in the regressions and also as base components in the
# system-configuration scripts. The values are meant to serve as a
# starting point, and specific parameters can be overridden in the
# specific instantiations.

class L1Cache(Cache):
    assoc = 2
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20

class L1_ICache(L1Cache):
    is_read_only = True
    # Writeback clean lines as well
    writeback_clean = True

class L1_DCache(L1Cache):
    pass

class L2Cache(Cache):
    assoc = 8
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    mshrs = 20
    tgts_per_mshr = 12
    write_buffers = 8

class IOCache(Cache):
    assoc = 8
    tag_latency = 50
    data_latency = 50
    response_latency = 50
    mshrs = 20
    size = '1kB'
    tgts_per_mshr = 12

class PageTableWalkerCache(Cache):
    assoc = 2
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 10
    size = '1kB'
    tgts_per_mshr = 12

    # the x86 table walker actually writes to the table-walker cache
    if buildEnv['TARGET_ISA'] in ['x86', 'riscv']:
        is_read_only = False
    else:
        is_read_only = True
        # Writeback clean lines as well
        writeback_clean = True
