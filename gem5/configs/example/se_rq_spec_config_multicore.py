# Copyright (c) 2012-2013 ARM Limited
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
# Copyright (c) 2006-2008 The Regents of The University of Michigan
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

# Simple test script
#
# "m5 test.py"

from __future__ import print_function
from __future__ import absolute_import

# import user_defined_benchmarks_spec06
import user_defined_benchmarks_spec17
# import user_defined_benchmarks_gap
import argparse
import optparse
import sys
import os

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.params import NULL
from m5.util import addToPath, fatal, warn

addToPath('../')

from ruby import Ruby

from common import Options
from common import Simulation
from common import CacheConfig
from common import CpuConfig
from common import ObjectList
from common import MemConfig
from common.FileSystemConfig import config_filesystem
from common.Caches import *
from common.cpu2000 import *

parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)
Options.addMirageOptions(parser)
Options.addMultiprogramOptions(parser)

parser.add_option("-b", "--benchmark", type="string", default="", help="The SPEC benchmark to be loaded.")
parser.add_option("--benchmark_stdout", type="string", default="", help="Absolute path for stdout redirection for the benchmark.")
parser.add_option("--benchmark_stderr", type="string", default="", help="Absolute path for stderr redirection for the benchmark.")
parser.add_option("--spec-version",type="string",default="2006", help="Version of SPEC benchmarks to use")

parser.add_option("--rh_defense", action="store_true", help="Enable Rowhammer defense")
parser.add_option("--rh_detector", action="store", type="string", help="Specify RH detector. Default is MG")
parser.add_option("--rh_mitigation", action="store", type="string", help="Specify RH mitigation. Default is RRS")
parser.add_option("--rh_threshold", action="store",type="int", help="Specify internal RH threshold. Default is 800")
parser.add_option("--rh_actual_threshold", action="store",type="int", help="Actual RH threshold. Default is 4800")
parser.add_option("--rh_mg_entries", action="store",type="int", help="MG per-bank tracker entries. Default is 1700")
parser.add_option("--rh_rrs_tuples", action="store",type="int", help="RRS per-bank RIT entries. Default is 3400")
parser.add_option("--rh_rq_qr_size", action="store",type="int", help="RQ global QR size in rows. Default is 23,052")
parser.add_option("--rh_rrs_swap_delay", action="store", type="string", help="RRS single swap delay. Default is 2740ns")
parser.add_option("--rh_rrs_acc_delay", action="store", type="string", help="RRS RIT access delay. Default is 1.25ns")
parser.add_option("--rh_rq_disable_rit_virt", action="store_true", help="Disable RQ RIT virtualization")
parser.add_option("--rh_rq_rows_per_btv_bit", action="store",type="int", help="RQ rows per btv bit. Default is 16")
parser.add_option("--rh_rq_cache_sets", action="store",type="int", help="RQ cache sets")
parser.add_option("--rh_rq_drain_threshold", action="store",type="int", help="RQ caches misses to trigger drain")

if '--ruby' in sys.argv:
    Ruby.define_options(parser)

(options, args) = parser.parse_args()

if args:
    print(' '.join(args))
    print("Error: script doesn't take any positional arguments")
    sys.exit(1)

multiprocesses = []
numThreads = 1
numProcesses = options.num_cpus

for i in range(numProcesses):
    # if options.spec_version == "2006":
    #     proc = user_defined_benchmarks_spec06.create_proc(options.benchmark,i)
    if options.spec_version == "2017":
        proc = user_defined_benchmarks_spec17.create_proc(options.benchmark,i)
    # elif options.spec_version == "gap":
    #     proc = user_defined_benchmarks_gap.create_proc(options.benchmark,i)
    else :
        print('Error No Benchmark Suite Found')
        sys.exit(1)
    # Set process stdout/stderr
    if options.benchmark_stdout:
        proc.output = options.benchmark_stdout + "_"+str(i)
        print( "Process stdout file: " + proc.output)
    if options.benchmark_stderr:
        proc.errout = options.benchmark_stderr + "_"+str(i)
        print( "Process stderr file: " + proc.errout)
    multiprocesses.append(proc)

(CPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)
CPUClass.numThreads = numThreads

# Check -- do not allow SMT with multiple CPUs
if options.smt and options.num_cpus > 1:
    fatal("You cannot use SMT with multiple CPUs!")

np = options.num_cpus
system = System(cpu = [CPUClass(cpu_id=i) for i in range(np)],
                mem_mode = test_mem_mode,
                mem_ranges = [AddrRange(options.mem_size)],
                cache_line_size = options.cacheline_size,
                workload = NULL)

if numThreads > 1:
    system.multi_thread = True

# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

# Create a source clock for the system and set the clock period
system.clk_domain = SrcClockDomain(clock =  '3GHz',
                                   voltage_domain = system.voltage_domain)

# Create a CPU voltage domain
system.cpu_voltage_domain = VoltageDomain()

# Create a separate clock domain for the CPUs
system.cpu_clk_domain = SrcClockDomain(clock = '3GHz',
                                       voltage_domain =
                                       system.cpu_voltage_domain)

# If elastic tracing is enabled, then configure the cpu and attach the elastic
# trace probe
if options.elastic_trace_en:
    CpuConfig.config_etrace(CPUClass, system.cpu, options)

# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
for cpu in system.cpu:
    cpu.clk_domain = system.cpu_clk_domain

if ObjectList.is_kvm_cpu(CPUClass) or ObjectList.is_kvm_cpu(FutureClass):
    if buildEnv['TARGET_ISA'] == 'x86':
        system.kvm_vm = KvmVM()
        for process in multiprocesses:
            process.useArchPT = True
            process.kvmInSE = True
    else:
        fatal("KvmCPU can only be used in SE mode with x86")

# Sanity check
if options.simpoint_profile:
    if not ObjectList.is_noncaching_cpu(CPUClass):
        fatal("SimPoint/BPProbe should be done with an atomic cpu")
    if np > 1:
        fatal("SimPoint generation not supported with more than one CPUs")

for i in range(np):
    system.cpu[i].workload = multiprocesses[i]                                                                                                       
    print("Workload-"+str(i)+": "),
    print(multiprocesses[i].cmd)

    # if options.smt:
    #     system.cpu[i].workload = multiprocesses
    # elif len(multiprocesses) == 1:
    #     system.cpu[i].workload = multiprocesses[0]
    # else:
    #     system.cpu[i].workload = multiprocesses[i]

    if options.simpoint_profile:
        system.cpu[i].addSimPointProbe(options.simpoint_interval)

    if options.checker:
        system.cpu[i].addCheckerCpu()

    if options.bp_type:
        bpClass = ObjectList.bp_list.get(options.bp_type)
        system.cpu[i].branchPred = bpClass()

    if options.indirect_bp_type:
        indirectBPClass = \
            ObjectList.indirect_bp_list.get(options.indirect_bp_type)
        system.cpu[i].branchPred.indirectBranchPred = indirectBPClass()

    system.cpu[i].createThreads()

if options.ruby:
    Ruby.create_system(options, False, system)
    assert(options.num_cpus == len(system.ruby._cpu_ports))

    system.ruby.clk_domain = SrcClockDomain(clock = options.ruby_clock,
                                        voltage_domain = system.voltage_domain)
    for i in range(np):
        ruby_port = system.ruby._cpu_ports[i]

        # Create the interrupt controller and connect its ports to Ruby
        # Note that the interrupt controller is always present but only
        # in x86 does it have message ports that need to be connected
        system.cpu[i].createInterruptController()

        # Connect the cpu's cache ports to Ruby
        system.cpu[i].icache_port = ruby_port.slave
        system.cpu[i].dcache_port = ruby_port.slave
        if buildEnv['TARGET_ISA'] == 'x86':
            system.cpu[i].interrupts[0].pio = ruby_port.master
            system.cpu[i].interrupts[0].int_master = ruby_port.slave
            system.cpu[i].interrupts[0].int_slave = ruby_port.master
            system.cpu[i].itb.walker.port = ruby_port.slave
            system.cpu[i].dtb.walker.port = ruby_port.slave
else:
    MemClass = Simulation.setMemClass(options)
    system.membus = SystemXBar()
    system.system_port = system.membus.cpu_side_ports
    CacheConfig.create_RQ_Cache_Hierarchy(options, system)
    MemConfig.config_mem(options, system)
    config_filesystem(system, options)

if options.wait_gdb:
    for cpu in system.cpu:
        cpu.wait_for_remote_gdb = True

root = Root(full_system = False, system = system)
Simulation.run(options, root, system, FutureClass)
