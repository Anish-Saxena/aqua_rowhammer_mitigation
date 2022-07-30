# Copyright (c) 2012-2020 ARM Limited
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
# Copyright (c) 2013 Amin Farmahini-Farahani
# Copyright (c) 2015 University of Kaiserslautern
# Copyright (c) 2015 The University of Bologna
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

from m5.params import *
from m5.proxy import *
from m5.objects.QoSMemCtrl import *

# Enum for memory scheduling algorithms, currently First-Come
# First-Served and a First-Row Hit then First-Come First-Served
class MemSched(Enum): vals = ['fcfs', 'frfcfs']

# MemCtrl is a single-channel single-ported Memory controller model
# that aims to model the most important system-level performance
# effects of a memory controller, interfacing with media specific
# interfaces
class MemCtrl(QoSMemCtrl):
    type = 'MemCtrl'
    cxx_header = "mem/mem_ctrl.hh"

    # single-ported on the system interface side, instantiate with a
    # bus in front of the controller for multiple ports
    port = ResponsePort("This port responds to memory requests")

    # Interface to volatile, DRAM media
    dram = Param.DRAMInterface(NULL, "DRAM interface")

    # Interface to non-volatile media
    nvm = Param.NVMInterface(NULL, "NVM interface")

    # read and write buffer depths are set in the interface
    # the controller will read these values when instantiated

    # threshold in percent for when to forcefully trigger writes and
    # start emptying the write buffer
    write_high_thresh_perc = Param.Percent(85, "Threshold to force writes")

    # threshold in percentage for when to start writes if the read
    # queue is empty
    write_low_thresh_perc = Param.Percent(50, "Threshold to start writes")

    # minimum write bursts to schedule before switching back to reads
    min_writes_per_switch = Param.Unsigned(16, "Minimum write bursts before "
                                           "switching to reads")

    # scheduler, address map and page policy
    mem_sched_policy = Param.MemSched('fcfs', "Memory scheduling policy")

    # pipeline latency of the controller and PHY, split into a
    # frontend part and a backend part, with reads and writes serviced
    # by the queues only seeing the frontend contribution, and reads
    # serviced by the memory seeing the sum of the two
    static_frontend_latency = Param.Latency("10ns", "Static frontend latency")
    static_backend_latency = Param.Latency("10ns", "Static backend latency")

    command_window = Param.Latency("10ns", "Static backend latency")

    # Rowhammer defense: Detection + Mitigation
    rh_defense = Param.Bool("False", "Enable Rowhammer defense")
    rh_detector = Param.String("MG", "Select Rowhammer detector. Choices: MG")
    rh_mitigation = Param.String("RRS", "Select Rowhammer mitigation. Choices: RRS, RQ")
    rh_threshold = Param.Unsigned(167, "Internal Rowhammer threshold")
    rh_actual_threshold = Param.Unsigned(1000, "Actual (reported) Rowhammer threshold")
    rh_mg_entries = Param.Unsigned(8144, "Number of MG entries per bank")
    rh_rrs_tuples = Param.Unsigned(16288, "Number of RRS RIT entries per bank")
    rh_rq_qr_size = Param.Unsigned(23053, "Quarantine region size in rows")
    rh_rrs_swap_delay = Param.Latency("2740ns", "Single swap delay in ns")
    rh_rit_acc_delay = Param.Latency("1.25ns", "RIT access latency in ns")
    rh_rq_virtualize_rit = Param.Bool("True", "Virtualize RIT in RQ")
    rh_rq_rows_per_btv_bit = Param.Unsigned(16, "Rows per bitvector bit")
    rh_rq_cache_sets = Param.Unsigned(256, "Row-Quarantine cache sets")
    rh_rq_drain_threshold = Param.Unsigned(1048576, "Row-Quarantine drain cache miss threshold")
    rh_bh_black_list_threshold = Param.Percent(50, "Blockhammer blacklist threshold in percentage")