/*
 * Copyright (c) 2010-2020 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2013 Amin Farmahini-Farahani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/mem_ctrl.hh"

#include "base/trace.hh"
#include "debug/DRAM.hh"
#include "debug/Drain.hh"
#include "debug/MemCtrl.hh"
#include "debug/NVM.hh"
#include "debug/QOS.hh"
#include "debug/RH_DEFENSE.hh"
#include "mem/mem_interface.hh"
#include "sim/system.hh"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <string>

using namespace std;

MemCtrl::MemCtrl(const MemCtrlParams* p) :
    QoS::MemCtrl(p),
    port(name() + ".port", *this), isTimingMode(false),
    retryRdReq(false), retryWrReq(false),
    nextReqEvent([this]{ processNextReqEvent(); }, name()),
    respondEvent([this]{ processRespondEvent(); }, name()),
    dram(p->dram), nvm(p->nvm),
    readBufferSize((dram ? dram->readBufferSize : 0) +
                   (nvm ? nvm->readBufferSize : 0)),
    writeBufferSize((dram ? dram->writeBufferSize : 0) +
                    (nvm ? nvm->writeBufferSize : 0)),
    writeHighThreshold(writeBufferSize * p->write_high_thresh_perc / 100.0),
    writeLowThreshold(writeBufferSize * p->write_low_thresh_perc / 100.0),
    minWritesPerSwitch(p->min_writes_per_switch),
    writesThisTime(0), readsThisTime(0),
    memSchedPolicy(p->mem_sched_policy),
    frontendLatency(p->static_frontend_latency),
    backendLatency(p->static_backend_latency),
    commandWindow(p->command_window),
    nextBurstAt(0), prevArrival(0),
    nextReqTime(0),
    rh_defense_enable(p->rh_defense),
    rh_detector(p->rh_detector),
    rh_mitigation(p->rh_mitigation),
    rh_threshold(p->rh_threshold),
    rh_actual_threshold(p->rh_actual_threshold),
    rh_mg_entries(p->rh_mg_entries),
    rh_rrs_tuples(p->rh_rrs_tuples),
    rh_rrs_swap_delay(p->rh_rrs_swap_delay),
    rh_rit_acc_delay(p->rh_rit_acc_delay),
    stats(*this)
{
    DPRINTF(MemCtrl, "Setting up controller\n");
    readQueue.resize(p->qos_priorities);
    writeQueue.resize(p->qos_priorities);

    // Hook up interfaces to the controller
    if (dram)
        dram->setCtrl(this, commandWindow);
    if (nvm)
        nvm->setCtrl(this, commandWindow);

    fatal_if(!dram && !nvm, "Memory controller must have an interface");

    // perform a basic check of the write thresholds
    if (p->write_low_thresh_perc >= p->write_high_thresh_perc)
        fatal("Write buffer low threshold %d must be smaller than the "
              "high threshold %d\n", p->write_low_thresh_perc,
              p->write_high_thresh_perc);

    if (rh_defense_enable) {
        if (rh_mitigation == "RRS") {
            rh_threshold = rh_actual_threshold/6;
            rh_mg_entries = 1360000/rh_threshold;
            rh_rrs_tuples = 2*rh_mg_entries;
            rh_defense = new Randomized_Row_Swap(rh_rrs_tuples, dram->getRowsPerBank(), 
                                rh_threshold, dram->getBanksPerRank(), dram->getRanksPerChannel(), 1);
            rh_defense->swapDelay = rh_rrs_swap_delay;
            rh_defense->virtualize_RIT = false;
            rh_defense->SRAM_RIT_accDelay = rh_rit_acc_delay;
            printf("RH Defense config: Detector: %s, Mitigation: %s, Effective Threshold: %d, "
                "MG Entries: %d\n",
                rh_detector.c_str(), rh_mitigation.c_str(), rh_threshold, rh_mg_entries);
            rh_defense->detector = new Misra_Gries(rh_mg_entries, rh_threshold, dram->getRowsPerBank(),
                                    dram->getBanksPerRank(), dram->getRanksPerChannel(), 1);
        }
        else if (rh_mitigation == "RQ") {
            rh_threshold = rh_actual_threshold/2;
            rh_mg_entries = 1360000/rh_threshold;
            rh_defense = new Row_Quarantine(p->rh_rq_qr_size, dram->getRowsPerBank(), rh_threshold, 
                                dram->getBanksPerRank(), dram->getRanksPerChannel(), 1);
            rh_defense->filter = NULL;
            rh_defense->cache = new Functional_Cache(p->rh_rq_cache_sets, 16); 
            rh_defense->cache->partial_lookup_factor = p->rh_rq_rows_per_btv_bit;
            rh_defense->moveDelay = rh_rrs_swap_delay/2;
            rh_defense->accDelay = 45000; // 45ns
            rh_defense->rows_per_btv_bit = p->rh_rq_rows_per_btv_bit; 
            rh_defense->drain_cache_miss_threshold = p->rh_rq_drain_threshold;

            uint32_t delay_ps_cacti[] = {136, 167, 227, 316, 431, 628}; // 8KB, 16KB, 32KB, 64KB, 128KB, 256KB

            int cache_delay_index = p->rh_rq_cache_sets/256;
            rh_defense->SRAM_Cache_accDelay = delay_ps_cacti[cache_delay_index];

            int cbf_delay_index = log2(32/p->rh_rq_rows_per_btv_bit);
            rh_defense->SRAM_CBF_accDelay = delay_ps_cacti[cbf_delay_index];

            rh_defense->virtualize_RIT = p->rh_rq_virtualize_rit;
            if (rh_defense->virtualize_RIT == false) {
                rh_defense->SRAM_RQT_accDelay = rh_rit_acc_delay; 
                rh_defense->SRAM_QRT_accDelay = 0; // off-the critical path
                rh_defense->SRAM_Cache_accDelay = 0;
                rh_defense->SRAM_CBF_accDelay = 0;
            }
            printf("RH Defense config: Detector: %s, Mitigation: %s, Effective Threshold: %d, "
                "Rows per BTV bit: %d, MG Entries: %d, is RIT virtualized: %d, RQ Cache Sets: %d\n"
                "RQ size: %d, drain threshold: %d, CBF delay: %dps, Cache delay: %dps\n",
                rh_detector.c_str(), rh_mitigation.c_str(), rh_threshold, rh_defense->rows_per_btv_bit, 
                rh_mg_entries, rh_defense->virtualize_RIT, p->rh_rq_cache_sets, p->rh_rq_qr_size, 
                p->rh_rq_drain_threshold, rh_defense->SRAM_CBF_accDelay, rh_defense->SRAM_Cache_accDelay);
            rh_defense->detector = new Misra_Gries(rh_mg_entries, rh_threshold, dram->getRowsPerBank(),
                                    dram->getBanksPerRank(), dram->getRanksPerChannel(), 1);
        }
        else { // Blockhammer
            rh_threshold = rh_actual_threshold/2; // To account for reset mismatch in CBF and DRAM row
            uint32_t black_list_threshold = (rh_threshold * p->rh_bh_black_list_threshold / 100.0);

            rh_defense = new BlockHammer(rh_threshold, dram->getRowsPerBank(), black_list_threshold, 
                                            dram->getBanksPerRank(), dram->getRanksPerChannel(), 1);
            rh_defense->filter = NULL;
            rh_defense->cache = NULL;
            rh_defense->SRAM_CBF_accDelay = rh_rit_acc_delay;
            printf("RH Defense config: Detector: %s, Mitigation: %s, Effective Threshold: %d, "
                "blacklist threshold: %d, CBF delay: %dps\n",
                rh_detector.c_str(), rh_mitigation.c_str(), rh_threshold, black_list_threshold, 
                rh_defense->SRAM_CBF_accDelay);
        }
    }
    printf("DRAM config: RowsPerBank: %d, BanksPerRank: %d, RanksPerChannel: %d\n",
            dram->getRowsPerBank(), dram->getBanksPerRank(), dram->getRanksPerChannel());
}

void
MemCtrl::init()
{
   if (!port.isConnected()) {
        fatal("MemCtrl %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
    }
}

void
MemCtrl::startup()
{
    // remember the memory system mode of operation
    isTimingMode = system()->isTimingMode();

    if (isTimingMode) {
        // shift the bus busy time sufficiently far ahead that we never
        // have to worry about negative values when computing the time for
        // the next request, this will add an insignificant bubble at the
        // start of simulation
        nextBurstAt = curTick() + (dram ? dram->commandOffset() :
                                          nvm->commandOffset());
    }
}

Tick
MemCtrl::recvAtomic(PacketPtr pkt)
{
    DPRINTF(MemCtrl, "recvAtomic: %s 0x%x\n",
                     pkt->cmdString(), pkt->getAddr());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    Tick latency = 0;
    // do the actual memory access and turn the packet into a response
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = dram->accessLatency();
        }
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        nvm->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = nvm->accessLatency();
        }
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    return latency;
}

bool
MemCtrl::readQueueFull(unsigned int neededEntries) const
{
    DPRINTF(MemCtrl,
            "Read queue limit %d, current size %d, entries needed %d\n",
            readBufferSize, totalReadQueueSize + respQueue.size(),
            neededEntries);

    auto rdsize_new = totalReadQueueSize + respQueue.size() + neededEntries;
    return rdsize_new > readBufferSize;
}

bool
MemCtrl::writeQueueFull(unsigned int neededEntries) const
{
    DPRINTF(MemCtrl,
            "Write queue limit %d, current size %d, entries needed %d\n",
            writeBufferSize, totalWriteQueueSize, neededEntries);

    auto wrsize_new = (totalWriteQueueSize + neededEntries);
    return  wrsize_new > writeBufferSize;
}

void
MemCtrl::addToReadQueue(PacketPtr pkt, unsigned int pkt_count, bool is_dram)
{
    // only add to the read queue here. whenever the request is
    // eventually done, set the readyTime, and call schedule()
    assert(!pkt->isWrite());

    assert(pkt_count != 0);

    // if the request size is larger than burst size, the pkt is split into
    // multiple packets
    // Note if the pkt starting address is not aligened to burst size, the
    // address of first packet is kept unaliged. Subsequent packets
    // are aligned to burst size boundaries. This is to ensure we accurately
    // check read packets against packets in write queue.
    const Addr base_addr = pkt->getAddr();
    Addr addr = base_addr;
    unsigned pktsServicedByWrQ = 0;
    BurstHelper* burst_helper = NULL;

    uint32_t burst_size = is_dram ? dram->bytesPerBurst() :
                                    nvm->bytesPerBurst();
    for (int cnt = 0; cnt < pkt_count; ++cnt) {
        unsigned size = std::min((addr | (burst_size - 1)) + 1,
                        base_addr + pkt->getSize()) - addr;
        stats.readPktSize[ceilLog2(size)]++;
        stats.readBursts++;
        stats.requestorReadAccesses[pkt->requestorId()]++;

        // First check write buffer to see if the data is already at
        // the controller
        bool foundInWrQ = false;
        Addr burst_addr = burstAlign(addr, is_dram);
        // if the burst address is not present then there is no need
        // looking any further
        if (isInWriteQueue.find(burst_addr) != isInWriteQueue.end()) {
            for (const auto& vec : writeQueue) {
                for (const auto& p : vec) {
                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (p->addr <= addr &&
                       ((addr + size) <= (p->addr + p->size))) {

                        foundInWrQ = true;
                        stats.servicedByWrQ++;
                        pktsServicedByWrQ++;
                        DPRINTF(MemCtrl,
                                "Read to addr %lld with size %d serviced by "
                                "write queue\n",
                                addr, size);
                        stats.bytesReadWrQ += burst_size;
                        break;
                    }
                }
            }
        }

        // If not found in the write q, make a memory packet and
        // push it onto the read queue
        if (!foundInWrQ) {

            // Make the burst helper for split packets
            if (pkt_count > 1 && burst_helper == NULL) {
                DPRINTF(MemCtrl, "Read to addr %lld translates to %d "
                        "memory requests\n", pkt->getAddr(), pkt_count);
                burst_helper = new BurstHelper(pkt_count);
            }

            MemPacket* mem_pkt;
            if (is_dram) {
                mem_pkt = dram->decodePacket(pkt, addr, size, true, true);

                //*********** Rowhammer Defense ***********
                if (rh_defense_enable) {
                    // DPRINTF(RH_DEFENSE, "Checking HRT and RIT for READ to row, bank, rank %lld, %lld, %lld\n",
                    //                         mem_pkt->row, mem_pkt->bank, mem_pkt->rank);
                    Decision rh_decision;
                    if (rh_mitigation == "RRS" || rh_mitigation == "BH") {
                        rh_decision = rh_defense->access(curTick(), mem_pkt->row, mem_pkt->bank, mem_pkt->rank, 0);
                    }
                    else {
                        rh_decision = rh_defense->access(curTick(), mem_pkt->addr, 0, 0, 0);
                    }
                    // Inject bubble before putting the request in memory bus to simulate delay due to Aqua
                    mem_pkt->rh_injectDelay = true;
                    mem_pkt->rh_bubbleSize = rh_decision.delay;
                    mem_pkt->rh_SRAMDelay = rh_decision.SRAM_delay + rh_decision.MC_delay;
                    if (rh_decision.mitigate) {
                        DPRINTF(RH_DEFENSE, "Mitigation request for row, bank, rank %lld, %lld, %lld: delay %lld\n",
                                            mem_pkt->row, mem_pkt->bank, mem_pkt->rank, mem_pkt->rh_bubbleSize);
                    }
                }
                //*****************************************

                // increment read entries of the rank
                dram->setupRank(mem_pkt->rank, true);
            } else {
                mem_pkt = nvm->decodePacket(pkt, addr, size, true, false);
                // Increment count to trigger issue of non-deterministic read
                nvm->setupRank(mem_pkt->rank, true);
                // Default readyTime to Max; will be reset once read is issued
                mem_pkt->readyTime = MaxTick;
            }
            mem_pkt->burstHelper = burst_helper;

            assert(!readQueueFull(1));
            stats.rdQLenPdf[totalReadQueueSize + respQueue.size()]++;

            DPRINTF(MemCtrl, "Adding to read queue\n");

            readQueue[mem_pkt->qosValue()].push_back(mem_pkt);

            // log packet
            logRequest(MemCtrl::READ, pkt->requestorId(), pkt->qosValue(),
                       mem_pkt->addr, 1);

            // Update stats
            stats.avgRdQLen = totalReadQueueSize + respQueue.size();
        }

        // Starting address of next memory pkt (aligned to burst boundary)
        addr = (addr | (burst_size - 1)) + 1;
    }

    // If all packets are serviced by write queue, we send the repsonse back
    if (pktsServicedByWrQ == pkt_count) {
        accessAndRespond(pkt, frontendLatency);
        return;
    }
    // Update how many split packets are serviced by write queue
    if (burst_helper != NULL)
        burst_helper->burstsServiced = pktsServicedByWrQ;

    // If we are not already scheduled to get a request out of the
    // queue, do so now
    if (!nextReqEvent.scheduled()) {
        DPRINTF(MemCtrl, "Request scheduled immediately\n");
        schedule(nextReqEvent, curTick());
    }
}

void
MemCtrl::addToWriteQueue(PacketPtr pkt, unsigned int pkt_count, bool is_dram)
{
    // only add to the write queue here. whenever the request is
    // eventually done, set the readyTime, and call schedule()
    assert(pkt->isWrite());

    // if the request size is larger than burst size, the pkt is split into
    // multiple packets
    const Addr base_addr = pkt->getAddr();
    Addr addr = base_addr;
    uint32_t burst_size = is_dram ? dram->bytesPerBurst() :
                                    nvm->bytesPerBurst();
    for (int cnt = 0; cnt < pkt_count; ++cnt) {
        unsigned size = std::min((addr | (burst_size - 1)) + 1,
                        base_addr + pkt->getSize()) - addr;
        stats.writePktSize[ceilLog2(size)]++;
        stats.writeBursts++;
        stats.requestorWriteAccesses[pkt->requestorId()]++;

        // see if we can merge with an existing item in the write
        // queue and keep track of whether we have merged or not
        bool merged = isInWriteQueue.find(burstAlign(addr, is_dram)) !=
            isInWriteQueue.end();

        // if the item was not merged we need to create a new write
        // and enqueue it
        if (!merged) {
            MemPacket* mem_pkt;
            if (is_dram) {
                mem_pkt = dram->decodePacket(pkt, addr, size, false, true);

                //*********** Rowhammer Defense ***********
                if (rh_defense_enable) {
                    // DPRINTF(RH_DEFENSE, "Checking HRT and RIT for WRITE to row, bank, rank %lld, %lld, %lld\n",
                    //                         mem_pkt->row, mem_pkt->bank, mem_pkt->rank);
                    Decision rh_decision;
                    if (rh_mitigation == "RRS" || rh_mitigation == "BH") {
                        rh_decision = rh_defense->access(curTick(), mem_pkt->row, mem_pkt->bank, mem_pkt->rank, 0);
                    }
                    else {
                        rh_decision = rh_defense->access(curTick(), mem_pkt->addr, 0, 0, 0);
                    }
                    // Inject bubble before putting the request in memory bus to simulate delay due to Aqua
                    mem_pkt->rh_injectDelay = true;
                    mem_pkt->rh_bubbleSize = rh_decision.delay;
                    mem_pkt->rh_SRAMDelay = rh_decision.SRAM_delay + rh_decision.MC_delay;
                    if (rh_decision.mitigate) {
                        DPRINTF(RH_DEFENSE, "Mitigation request for row, bank, rank %lld, %lld, %lld: delay %lld\n",
                                            mem_pkt->row, mem_pkt->bank, mem_pkt->rank, mem_pkt->rh_bubbleSize);
                    }
                }
                //*****************************************

                dram->setupRank(mem_pkt->rank, false);
            } else {
                mem_pkt = nvm->decodePacket(pkt, addr, size, false, false);
                nvm->setupRank(mem_pkt->rank, false);
            }
            assert(totalWriteQueueSize < writeBufferSize);
            stats.wrQLenPdf[totalWriteQueueSize]++;

            DPRINTF(MemCtrl, "Adding to write queue\n");

            writeQueue[mem_pkt->qosValue()].push_back(mem_pkt);
            isInWriteQueue.insert(burstAlign(addr, is_dram));

            // log packet
            logRequest(MemCtrl::WRITE, pkt->requestorId(), pkt->qosValue(),
                       mem_pkt->addr, 1);

            assert(totalWriteQueueSize == isInWriteQueue.size());

            // Update stats
            stats.avgWrQLen = totalWriteQueueSize;

        } else {
            DPRINTF(MemCtrl,
                    "Merging write burst with existing queue entry\n");

            // keep track of the fact that this burst effectively
            // disappeared as it was merged with an existing one
            stats.mergedWrBursts++;
        }

        // Starting address of next memory pkt (aligned to burst_size boundary)
        addr = (addr | (burst_size - 1)) + 1;
    }
    
    // we do not wait for the writes to be send to the actual memory,
    // but instead take responsibility for the consistency here and
    // snoop the write queue for any upcoming reads
    // @todo, if a pkt size is larger than burst size, we might need a
    // different front end latency
    accessAndRespond(pkt, frontendLatency);

    // If we are not already scheduled to get a request out of the
    // queue, do so now
    if (!nextReqEvent.scheduled()) {
        DPRINTF(MemCtrl, "Request scheduled immediately\n");
        schedule(nextReqEvent, curTick());
    }
}

void
MemCtrl::printQs() const
{
#if TRACING_ON
    DPRINTF(MemCtrl, "===READ QUEUE===\n\n");
    for (const auto& queue : readQueue) {
        for (const auto& packet : queue) {
            DPRINTF(MemCtrl, "Read %lu\n", packet->addr);
        }
    }

    DPRINTF(MemCtrl, "\n===RESP QUEUE===\n\n");
    for (const auto& packet : respQueue) {
        DPRINTF(MemCtrl, "Response %lu\n", packet->addr);
    }

    DPRINTF(MemCtrl, "\n===WRITE QUEUE===\n\n");
    for (const auto& queue : writeQueue) {
        for (const auto& packet : queue) {
            DPRINTF(MemCtrl, "Write %lu\n", packet->addr);
        }
    }
#endif // TRACING_ON
}

bool
MemCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    DPRINTF(MemCtrl, "recvTimingReq: request %s addr %lld size %d\n",
            pkt->cmdString(), pkt->getAddr(), pkt->getSize());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller\n");

    // Calc avg gap between requests
    if (prevArrival != 0) {
        stats.totGap += curTick() - prevArrival;
    }
    prevArrival = curTick();

    // What type of media does this packet access?
    bool is_dram;
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        is_dram = true;
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        is_dram = false;
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }


    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets
    unsigned size = pkt->getSize();
    uint32_t burst_size = is_dram ? dram->bytesPerBurst() :
                                    nvm->bytesPerBurst();
    unsigned offset = pkt->getAddr() & (burst_size - 1);
    unsigned int pkt_count = divCeil(offset + size, burst_size);

    // run the QoS scheduler and assign a QoS priority value to the packet
    qosSchedule( { &readQueue, &writeQueue }, burst_size, pkt);

    // check local buffers and do not accept if full
    if (pkt->isWrite()) {
        assert(size != 0);
        if (writeQueueFull(pkt_count)) {
            DPRINTF(MemCtrl, "Write queue full, not accepting\n");
            // remember that we have to retry this port
            retryWrReq = true;
            stats.numWrRetry++;
            return false;
        } else {
            addToWriteQueue(pkt, pkt_count, is_dram);
            stats.writeReqs++;
            stats.bytesWrittenSys += size;
        }
    } else {
        assert(pkt->isRead());
        assert(size != 0);
        if (readQueueFull(pkt_count)) {
            DPRINTF(MemCtrl, "Read queue full, not accepting\n");
            // remember that we have to retry this port
            retryRdReq = true;
            stats.numRdRetry++;
            return false;
        } else {
            addToReadQueue(pkt, pkt_count, is_dram);
            stats.readReqs++;
            stats.bytesReadSys += size;
        }
    }

    return true;
}

void
MemCtrl::processRespondEvent()
{
    DPRINTF(MemCtrl,
            "processRespondEvent(): Some req has reached its readyTime\n");

    MemPacket* mem_pkt = respQueue.front();

    if (mem_pkt->isDram()) {
        // media specific checks and functions when read response is complete
        dram->respondEvent(mem_pkt->rank);
    }

    if (mem_pkt->burstHelper) {
        // it is a split packet
        mem_pkt->burstHelper->burstsServiced++;
        if (mem_pkt->burstHelper->burstsServiced ==
            mem_pkt->burstHelper->burstCount) {
            // we have now serviced all children packets of a system packet
            // so we can now respond to the requestor
            // @todo we probably want to have a different front end and back
            // end latency for split packets
            if (rh_defense_enable && mem_pkt->pkt->isRequest()) {
                accessAndRespond(mem_pkt->pkt, mem_pkt->rh_SRAMDelay +
                                                frontendLatency + backendLatency);
            }
            else {
                accessAndRespond(mem_pkt->pkt, frontendLatency + backendLatency);
            }
            delete mem_pkt->burstHelper;
            mem_pkt->burstHelper = NULL;
        }
    } else {
        // it is not a split packet 
        if (rh_defense_enable && mem_pkt->pkt->isRequest()) {
            accessAndRespond(mem_pkt->pkt, mem_pkt->rh_SRAMDelay +
                                            frontendLatency + backendLatency);
        }
        else {
            accessAndRespond(mem_pkt->pkt, frontendLatency + backendLatency);
        }
    }

    delete respQueue.front();
    respQueue.pop_front();

    if (!respQueue.empty()) {
        assert(respQueue.front()->readyTime >= curTick());
        assert(!respondEvent.scheduled());
        schedule(respondEvent, respQueue.front()->readyTime);
    } else {
        // if there is nothing left in any queue, signal a drain
        if (drainState() == DrainState::Draining &&
            !totalWriteQueueSize && !totalReadQueueSize &&
            allIntfDrained()) {

            DPRINTF(Drain, "Controller done draining\n");
            signalDrainDone();
        } else if (mem_pkt->isDram()) {
            // check the refresh state and kick the refresh event loop
            // into action again if banks already closed and just waiting
            // for read to complete
            dram->checkRefreshState(mem_pkt->rank);
        }
    }

    // We have made a location in the queue available at this point,
    // so if there is a read that was forced to wait, retry now
    if (retryRdReq) {
        retryRdReq = false;
        port.sendRetryReq();
    }
}

MemPacketQueue::iterator
MemCtrl::chooseNext(MemPacketQueue& queue, Tick extra_col_delay)
{
    // This method does the arbitration between requests.

    MemPacketQueue::iterator ret = queue.end();

    if (!queue.empty()) {
        if (queue.size() == 1) {
            // available rank corresponds to state refresh idle
            MemPacket* mem_pkt = *(queue.begin());
            if (packetReady(mem_pkt)) {
                ret = queue.begin();
                DPRINTF(MemCtrl, "Single request, going to a free rank\n");
            } else {
                DPRINTF(MemCtrl, "Single request, going to a busy rank\n");
            }
        } else if (memSchedPolicy == Enums::fcfs) {
            // check if there is a packet going to a free rank
            for (auto i = queue.begin(); i != queue.end(); ++i) {
                MemPacket* mem_pkt = *i;
                if (packetReady(mem_pkt)) {
                    ret = i;
                    break;
                }
            }
        } else if (memSchedPolicy == Enums::frfcfs) {
            ret = chooseNextFRFCFS(queue, extra_col_delay);
        } else {
            panic("No scheduling policy chosen\n");
        }
    }
    return ret;
}

MemPacketQueue::iterator
MemCtrl::chooseNextFRFCFS(MemPacketQueue& queue, Tick extra_col_delay)
{
    auto selected_pkt_it = queue.end();
    Tick col_allowed_at = MaxTick;

    // time we need to issue a column command to be seamless
    const Tick min_col_at = std::max(nextBurstAt + extra_col_delay, curTick());

    // find optimal packet for each interface
    if (dram && nvm) {
        // create 2nd set of parameters for NVM
        auto nvm_pkt_it = queue.end();
        Tick nvm_col_at = MaxTick;

        // Select packet by default to give priority if both
        // can issue at the same time or seamlessly
        std::tie(selected_pkt_it, col_allowed_at) =
                 dram->chooseNextFRFCFS(queue, min_col_at);
        std::tie(nvm_pkt_it, nvm_col_at) =
                 nvm->chooseNextFRFCFS(queue, min_col_at);

        // Compare DRAM and NVM and select NVM if it can issue
        // earlier than the DRAM packet
        if (col_allowed_at > nvm_col_at) {
            selected_pkt_it = nvm_pkt_it;
        }
    } else if (dram) {
        std::tie(selected_pkt_it, col_allowed_at) =
                 dram->chooseNextFRFCFS(queue, min_col_at);
    } else if (nvm) {
        std::tie(selected_pkt_it, col_allowed_at) =
                 nvm->chooseNextFRFCFS(queue, min_col_at);
    }

    if (selected_pkt_it == queue.end()) {
        DPRINTF(MemCtrl, "%s no available packets found\n", __func__);
    }

    return selected_pkt_it;
}

void
MemCtrl::accessAndRespond(PacketPtr pkt, Tick static_latency)
{
    DPRINTF(MemCtrl, "Responding to Address %lld.. \n",pkt->getAddr());

    bool needsResponse = pkt->needsResponse();
    // do the actual memory access which also turns the packet into a
    // response
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        nvm->access(pkt);
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    // turn packet around to go back to requestor if response expected
    if (needsResponse) {
        // access already turned the packet into a response
        assert(pkt->isResponse());
        // response_time consumes the static latency and is charged also
        // with headerDelay that takes into account the delay provided by
        // the xbar and also the payloadDelay that takes into account the
        // number of data beats.
        Tick response_time = curTick() + static_latency + pkt->headerDelay +
                             pkt->payloadDelay;
        // Here we reset the timing of the packet before sending it out.
        pkt->headerDelay = pkt->payloadDelay = 0;

        // queue the packet in the response queue to be sent out after
        // the static latency has passed
        port.schedTimingResp(pkt, response_time);
    } else {
        // @todo the packet is going to be deleted, and the MemPacket
        // is still having a pointer to it
        pendingDelete.reset(pkt);
    }

    DPRINTF(MemCtrl, "Done\n");

    return;
}

void
MemCtrl::pruneBurstTick()
{
    auto it = burstTicks.begin();
    while (it != burstTicks.end()) {
        auto current_it = it++;
        if (curTick() > *current_it) {
            DPRINTF(MemCtrl, "Removing burstTick for %d\n", *current_it);
            burstTicks.erase(current_it);
        }
    }
}

Tick
MemCtrl::getBurstWindow(Tick cmd_tick)
{
    // get tick aligned to burst window
    Tick burst_offset = cmd_tick % commandWindow;
    return (cmd_tick - burst_offset);
}

Tick
MemCtrl::verifySingleCmd(Tick cmd_tick, Tick max_cmds_per_burst)
{
    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = getBurstWindow(cmd_tick);

    // verify that we have command bandwidth to issue the command
    // if not, iterate over next window(s) until slot found
    while (burstTicks.count(burst_tick) >= max_cmds_per_burst) {
        DPRINTF(MemCtrl, "Contention found on command bus at %d\n",
                burst_tick);
        burst_tick += commandWindow;
        cmd_at = burst_tick;
    }

    // add command into burst window and return corresponding Tick
    burstTicks.insert(burst_tick);
    return cmd_at;
}

Tick
MemCtrl::verifyMultiCmd(Tick cmd_tick, Tick max_cmds_per_burst,
                         Tick max_multi_cmd_split)
{
    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = getBurstWindow(cmd_tick);

    // Command timing requirements are from 2nd command
    // Start with assumption that 2nd command will issue at cmd_at and
    // find prior slot for 1st command to issue
    // Given a maximum latency of max_multi_cmd_split between the commands,
    // find the burst at the maximum latency prior to cmd_at
    Tick burst_offset = 0;
    Tick first_cmd_offset = cmd_tick % commandWindow;
    while (max_multi_cmd_split > (first_cmd_offset + burst_offset)) {
        burst_offset += commandWindow;
    }
    // get the earliest burst aligned address for first command
    // ensure that the time does not go negative
    Tick first_cmd_tick = burst_tick - std::min(burst_offset, burst_tick);

    // Can required commands issue?
    bool first_can_issue = false;
    bool second_can_issue = false;
    // verify that we have command bandwidth to issue the command(s)
    while (!first_can_issue || !second_can_issue) {
        bool same_burst = (burst_tick == first_cmd_tick);
        auto first_cmd_count = burstTicks.count(first_cmd_tick);
        auto second_cmd_count = same_burst ? first_cmd_count + 1 :
                                   burstTicks.count(burst_tick);

        first_can_issue = first_cmd_count < max_cmds_per_burst;
        second_can_issue = second_cmd_count < max_cmds_per_burst;

        if (!second_can_issue) {
            DPRINTF(MemCtrl, "Contention (cmd2) found on command bus at %d\n",
                    burst_tick);
            burst_tick += commandWindow;
            cmd_at = burst_tick;
        }

        // Verify max_multi_cmd_split isn't violated when command 2 is shifted
        // If commands initially were issued in same burst, they are
        // now in consecutive bursts and can still issue B2B
        bool gap_violated = !same_burst &&
             ((burst_tick - first_cmd_tick) > max_multi_cmd_split);

        if (!first_can_issue || (!second_can_issue && gap_violated)) {
            DPRINTF(MemCtrl, "Contention (cmd1) found on command bus at %d\n",
                    first_cmd_tick);
            first_cmd_tick += commandWindow;
        }
    }

    // Add command to burstTicks
    burstTicks.insert(burst_tick);
    burstTicks.insert(first_cmd_tick);

    return cmd_at;
}

bool
MemCtrl::inReadBusState(bool next_state) const
{
    // check the bus state
    if (next_state) {
        // use busStateNext to get the state that will be used
        // for the next burst
        return (busStateNext == MemCtrl::READ);
    } else {
        return (busState == MemCtrl::READ);
    }
}

bool
MemCtrl::inWriteBusState(bool next_state) const
{
    // check the bus state
    if (next_state) {
        // use busStateNext to get the state that will be used
        // for the next burst
        return (busStateNext == MemCtrl::WRITE);
    } else {
        return (busState == MemCtrl::WRITE);
    }
}

void
MemCtrl::doBurstAccess(MemPacket* mem_pkt)
{
    // first clean up the burstTick set, removing old entries
    // before adding new entries for next burst
    pruneBurstTick();

    // When was command issued?
    Tick cmd_at;

    // Issue the next burst and update bus state to reflect
    // when previous command was issued
    if (mem_pkt->isDram()) {
        //*********** Rowhammer Defense ***********
        // Introduce bubble the size of swap delay
        if (rh_defense_enable && mem_pkt->rh_injectDelay && mem_pkt->rh_bubbleSize > 0) {
            DPRINTF(RH_DEFENSE, "Adding delay of %lld to row, rank, bank %lld, %lld, %lld\n",
                        mem_pkt->rh_bubbleSize, mem_pkt->row, mem_pkt->bank, mem_pkt->rank);
            // 3*swapDelay is greater than 7.8us, which is the refresh interval
            // so to ensure QoS, we limit the bubble size
            nextBurstAt += (mem_pkt->rh_bubbleSize >= 7800*1000 ? 7000*1000 : mem_pkt->rh_bubbleSize);
        }
        if (rh_defense_enable && rh_defense->s_num_accesses%100 == 0) {
            copy_rh_stats();
        }
        //*****************************************

        std::vector<MemPacketQueue>& queue = selQueue(mem_pkt->isRead());
        std::tie(cmd_at, nextBurstAt) =
                 dram->doBurstAccess(mem_pkt, nextBurstAt, queue);

        // Update timing for NVM ranks if NVM is configured on this channel
        if (nvm)
            nvm->addRankToRankDelay(cmd_at);

    } else {
        std::tie(cmd_at, nextBurstAt) =
                 nvm->doBurstAccess(mem_pkt, nextBurstAt);

        // Update timing for NVM ranks if NVM is configured on this channel
        if (dram)
            dram->addRankToRankDelay(cmd_at);

    }

    DPRINTF(MemCtrl, "Access to %lld, ready at %lld next burst at %lld.\n",
            mem_pkt->addr, mem_pkt->readyTime, nextBurstAt);

    // Update the minimum timing between the requests, this is a
    // conservative estimate of when we have to schedule the next
    // request to not introduce any unecessary bubbles. In most cases
    // we will wake up sooner than we have to.
    nextReqTime = nextBurstAt - (dram ? dram->commandOffset() :
                                        nvm->commandOffset());


    // Update the common bus stats
    if (mem_pkt->isRead()) {
        ++readsThisTime;
        // Update latency stats
        stats.requestorReadTotalLat[mem_pkt->requestorId()] +=
            mem_pkt->readyTime - mem_pkt->entryTime;
        stats.requestorReadBytes[mem_pkt->requestorId()] += mem_pkt->size;
    } else {
        ++writesThisTime;
        stats.requestorWriteBytes[mem_pkt->requestorId()] += mem_pkt->size;
        stats.requestorWriteTotalLat[mem_pkt->requestorId()] +=
            mem_pkt->readyTime - mem_pkt->entryTime;
    }
}

void
MemCtrl::processNextReqEvent()
{
    // transition is handled by QoS algorithm if enabled
    if (turnPolicy) {
        // select bus state - only done if QoS algorithms are in use
        busStateNext = selectNextBusState();
    }

    // detect bus state change
    bool switched_cmd_type = (busState != busStateNext);
    // record stats
    recordTurnaroundStats();

    DPRINTF(MemCtrl, "QoS Turnarounds selected state %s %s\n",
            (busState==MemCtrl::READ)?"READ":"WRITE",
            switched_cmd_type?"[turnaround triggered]":"");

    if (switched_cmd_type) {
        if (busState == MemCtrl::READ) {
            DPRINTF(MemCtrl,
                    "Switching to writes after %d reads with %d reads "
                    "waiting\n", readsThisTime, totalReadQueueSize);
            stats.rdPerTurnAround.sample(readsThisTime);
            readsThisTime = 0;
        } else {
            DPRINTF(MemCtrl,
                    "Switching to reads after %d writes with %d writes "
                    "waiting\n", writesThisTime, totalWriteQueueSize);
            stats.wrPerTurnAround.sample(writesThisTime);
            writesThisTime = 0;
        }
    }

    // updates current state
    busState = busStateNext;

    if (nvm) {
        for (auto queue = readQueue.rbegin();
             queue != readQueue.rend(); ++queue) {
             // select non-deterministic NVM read to issue
             // assume that we have the command bandwidth to issue this along
             // with additional RD/WR burst with needed bank operations
             if (nvm->readsWaitingToIssue()) {
                 // select non-deterministic NVM read to issue
                 nvm->chooseRead(*queue);
             }
        }
    }

    // check ranks for refresh/wakeup - uses busStateNext, so done after
    // turnaround decisions
    // Default to busy status and update based on interface specifics
    bool dram_busy = dram ? dram->isBusy() : true;
    bool nvm_busy = true;
    bool all_writes_nvm = false;
    if (nvm) {
        all_writes_nvm = nvm->numWritesQueued == totalWriteQueueSize;
        bool read_queue_empty = totalReadQueueSize == 0;
        nvm_busy = nvm->isBusy(read_queue_empty, all_writes_nvm);
    }
    // Default state of unused interface is 'true'
    // Simply AND the busy signals to determine if system is busy
    if (dram_busy && nvm_busy) {
        // if all ranks are refreshing wait for them to finish
        // and stall this state machine without taking any further
        // action, and do not schedule a new nextReqEvent
        return;
    }

    // when we get here it is either a read or a write
    if (busState == READ) {

        // track if we should switch or not
        bool switch_to_writes = false;

        if (totalReadQueueSize == 0) {
            // In the case there is no read request to go next,
            // trigger writes if we have passed the low threshold (or
            // if we are draining)
            if (!(totalWriteQueueSize == 0) &&
                (drainState() == DrainState::Draining ||
                 totalWriteQueueSize > writeLowThreshold)) {

                DPRINTF(MemCtrl,
                        "Switching to writes due to read queue empty\n");
                switch_to_writes = true;
            } else {
                // check if we are drained
                // not done draining until in PWR_IDLE state
                // ensuring all banks are closed and
                // have exited low power states
                if (drainState() == DrainState::Draining &&
                    respQueue.empty() && allIntfDrained()) {

                    DPRINTF(Drain, "MemCtrl controller done draining\n");
                    signalDrainDone();
                }

                // nothing to do, not even any point in scheduling an
                // event for the next request
                return;
            }
        } else {

            bool read_found = false;
            MemPacketQueue::iterator to_read;
            uint8_t prio = numPriorities();

            for (auto queue = readQueue.rbegin();
                 queue != readQueue.rend(); ++queue) {

                prio--;

                DPRINTF(QOS,
                        "Checking READ queue [%d] priority [%d elements]\n",
                        prio, queue->size());

                // Figure out which read request goes next
                // If we are changing command type, incorporate the minimum
                // bus turnaround delay which will be rank to rank delay
                to_read = chooseNext((*queue), switched_cmd_type ?
                                               minWriteToReadDataGap() : 0);

                if (to_read != queue->end()) {
                    // candidate read found
                    read_found = true;
                    break;
                }
            }

            // if no read to an available rank is found then return
            // at this point. There could be writes to the available ranks
            // which are above the required threshold. However, to
            // avoid adding more complexity to the code, return and wait
            // for a refresh event to kick things into action again.
            if (!read_found) {
                DPRINTF(MemCtrl, "No Reads Found - exiting\n");
                return;
            }

            auto mem_pkt = *to_read;

            doBurstAccess(mem_pkt);

            // sanity check
            assert(mem_pkt->size <= (mem_pkt->isDram() ?
                                      dram->bytesPerBurst() :
                                      nvm->bytesPerBurst()) );
            assert(mem_pkt->readyTime >= curTick());

            // log the response
            logResponse(MemCtrl::READ, (*to_read)->requestorId(),
                        mem_pkt->qosValue(), mem_pkt->getAddr(), 1,
                        mem_pkt->readyTime - mem_pkt->entryTime);


            // Insert into response queue. It will be sent back to the
            // requestor at its readyTime
            if (respQueue.empty()) {
                assert(!respondEvent.scheduled());
                schedule(respondEvent, mem_pkt->readyTime);
            } else {
                assert(respQueue.back()->readyTime <= mem_pkt->readyTime);
                assert(respondEvent.scheduled());
            }

            respQueue.push_back(mem_pkt);

            // we have so many writes that we have to transition
            // don't transition if the writeRespQueue is full and
            // there are no other writes that can issue
            if ((totalWriteQueueSize > writeHighThreshold) &&
               !(nvm && all_writes_nvm && nvm->writeRespQueueFull())) {
                switch_to_writes = true;
            }

            // remove the request from the queue
            // the iterator is no longer valid .
            readQueue[mem_pkt->qosValue()].erase(to_read);
        }

        // switching to writes, either because the read queue is empty
        // and the writes have passed the low threshold (or we are
        // draining), or because the writes hit the hight threshold
        if (switch_to_writes) {
            // transition to writing
            busStateNext = WRITE;
        }
    } else {

        bool write_found = false;
        MemPacketQueue::iterator to_write;
        uint8_t prio = numPriorities();

        for (auto queue = writeQueue.rbegin();
             queue != writeQueue.rend(); ++queue) {

            prio--;

            DPRINTF(QOS,
                    "Checking WRITE queue [%d] priority [%d elements]\n",
                    prio, queue->size());

            // If we are changing command type, incorporate the minimum
            // bus turnaround delay
            to_write = chooseNext((*queue),
                     switched_cmd_type ? minReadToWriteDataGap() : 0);

            if (to_write != queue->end()) {
                write_found = true;
                break;
            }
        }

        // if there are no writes to a rank that is available to service
        // requests (i.e. rank is in refresh idle state) are found then
        // return. There could be reads to the available ranks. However, to
        // avoid adding more complexity to the code, return at this point and
        // wait for a refresh event to kick things into action again.
        if (!write_found) {
            DPRINTF(MemCtrl, "No Writes Found - exiting\n");
            return;
        }

        auto mem_pkt = *to_write;

        // sanity check
        assert(mem_pkt->size <= (mem_pkt->isDram() ?
                                  dram->bytesPerBurst() :
                                  nvm->bytesPerBurst()) );

        doBurstAccess(mem_pkt);

        isInWriteQueue.erase(burstAlign(mem_pkt->addr, mem_pkt->isDram()));

        // log the response
        logResponse(MemCtrl::WRITE, mem_pkt->requestorId(),
                    mem_pkt->qosValue(), mem_pkt->getAddr(), 1,
                    mem_pkt->readyTime - mem_pkt->entryTime);


        // remove the request from the queue - the iterator is no longer valid
        writeQueue[mem_pkt->qosValue()].erase(to_write);

        delete mem_pkt;

        // If we emptied the write queue, or got sufficiently below the
        // threshold (using the minWritesPerSwitch as the hysteresis) and
        // are not draining, or we have reads waiting and have done enough
        // writes, then switch to reads.
        // If we are interfacing to NVM and have filled the writeRespQueue,
        // with only NVM writes in Q, then switch to reads
        bool below_threshold =
            totalWriteQueueSize + minWritesPerSwitch < writeLowThreshold;

        if (totalWriteQueueSize == 0 ||
            (below_threshold && drainState() != DrainState::Draining) ||
            (totalReadQueueSize && writesThisTime >= minWritesPerSwitch) ||
            (totalReadQueueSize && nvm && nvm->writeRespQueueFull() &&
             all_writes_nvm)) {

            // turn the bus back around for reads again
            busStateNext = MemCtrl::READ;

            // note that the we switch back to reads also in the idle
            // case, which eventually will check for any draining and
            // also pause any further scheduling if there is really
            // nothing to do
        }
    }
    // It is possible that a refresh to another rank kicks things back into
    // action before reaching this point.
    if (!nextReqEvent.scheduled())
        schedule(nextReqEvent, std::max(nextReqTime, curTick()));

    // If there is space available and we have writes waiting then let
    // them retry. This is done here to ensure that the retry does not
    // cause a nextReqEvent to be scheduled before we do so as part of
    // the next request processing
    if (retryWrReq && totalWriteQueueSize < writeBufferSize) {
        retryWrReq = false;
        port.sendRetryReq();
    }
}

bool
MemCtrl::packetReady(MemPacket* pkt)
{
    return (pkt->isDram() ?
        dram->burstReady(pkt) : nvm->burstReady(pkt));
}

Tick
MemCtrl::minReadToWriteDataGap()
{
    Tick dram_min = dram ?  dram->minReadToWriteDataGap() : MaxTick;
    Tick nvm_min = nvm ?  nvm->minReadToWriteDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

Tick
MemCtrl::minWriteToReadDataGap()
{
    Tick dram_min = dram ? dram->minWriteToReadDataGap() : MaxTick;
    Tick nvm_min = nvm ?  nvm->minWriteToReadDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

Addr
MemCtrl::burstAlign(Addr addr, bool is_dram) const
{
    if (is_dram)
        return (addr & ~(Addr(dram->bytesPerBurst() - 1)));
    else
        return (addr & ~(Addr(nvm->bytesPerBurst() - 1)));
}

MemCtrl::CtrlStats::CtrlStats(MemCtrl &_ctrl)
    : Stats::Group(&_ctrl),
    ctrl(_ctrl),

    ADD_STAT(readReqs, "Number of read requests accepted"),
    ADD_STAT(writeReqs, "Number of write requests accepted"),

    ADD_STAT(readBursts,
             "Number of controller read bursts, "
             "including those serviced by the write queue"),
    ADD_STAT(writeBursts,
             "Number of controller write bursts, "
             "including those merged in the write queue"),
    ADD_STAT(servicedByWrQ,
             "Number of controller read bursts serviced by the write queue"),
    ADD_STAT(mergedWrBursts,
             "Number of controller write bursts merged with an existing one"),

    ADD_STAT(neitherReadNorWriteReqs,
             "Number of requests that are neither read nor write"),

    ADD_STAT(avgRdQLen, "Average read queue length when enqueuing"),
    ADD_STAT(avgWrQLen, "Average write queue length when enqueuing"),

    ADD_STAT(numRdRetry, "Number of times read queue was full causing retry"),
    ADD_STAT(numWrRetry, "Number of times write queue was full causing retry"),

    ADD_STAT(readPktSize, "Read request sizes (log2)"),
    ADD_STAT(writePktSize, "Write request sizes (log2)"),

    ADD_STAT(rdQLenPdf, "What read queue length does an incoming req see"),
    ADD_STAT(wrQLenPdf, "What write queue length does an incoming req see"),

    ADD_STAT(rdPerTurnAround,
             "Reads before turning the bus around for writes"),
    ADD_STAT(wrPerTurnAround,
             "Writes before turning the bus around for reads"),

    ADD_STAT(bytesReadWrQ, "Total number of bytes read from write queue"),
    ADD_STAT(bytesReadSys, "Total read bytes from the system interface side"),
    ADD_STAT(bytesWrittenSys,
             "Total written bytes from the system interface side"),

    ADD_STAT(avgRdBWSys, "Average system read bandwidth in MiByte/s"),
    ADD_STAT(avgWrBWSys, "Average system write bandwidth in MiByte/s"),

    ADD_STAT(totGap, "Total gap between requests"),
    ADD_STAT(avgGap, "Average gap between requests"),

    ADD_STAT(requestorReadBytes, "Per-requestor bytes read from memory"),
    ADD_STAT(requestorWriteBytes, "Per-requestor bytes write to memory"),
    ADD_STAT(requestorReadRate,
             "Per-requestor bytes read from memory rate (Bytes/sec)"),
    ADD_STAT(requestorWriteRate,
             "Per-requestor bytes write to memory rate (Bytes/sec)"),
    ADD_STAT(requestorReadAccesses,
             "Per-requestor read serviced memory accesses"),
    ADD_STAT(requestorWriteAccesses,
             "Per-requestor write serviced memory accesses"),
    ADD_STAT(requestorReadTotalLat,
             "Per-requestor read total memory access latency"),
    ADD_STAT(requestorWriteTotalLat,
             "Per-requestor write total memory access latency"),
    ADD_STAT(requestorReadAvgLat,
             "Per-requestor read average memory access latency"),
    ADD_STAT(requestorWriteAvgLat,
             "Per-requestor write average memory access latency"),
    ADD_STAT(rh_rrs_num_accesses,
            "RRS memory accesses to defense"),
    ADD_STAT(rh_rrs_remapped_row_accessed,
            "RRS remapped row accessed"),
    ADD_STAT(rh_rrs_clean_install,
        "RRS swaps inserted without eviction"),
    ADD_STAT(rh_rrs_only_unswap,
        "RRS swaps inserted with eviction"),
    ADD_STAT(rh_rrs_clean_reswap,
        "RRS swaps inserted using re-swap without eviction"),
    ADD_STAT(rh_rrs_dirty_reswap,
        "RRS swaps inserted using re-swap with eviction"),
    ADD_STAT(rh_rrs_rand_row_gen_failed,
        "RRS rand row generation failed for 10 trials"),
    ADD_STAT(rh_rrs_num_resets,
        "RRS number of total resets"),
    ADD_STAT(rh_mg_numUniqRows,
        "MG number of unique rows flagged as aggressor"),
    ADD_STAT(rh_move_to_qr,
        "RQ move from outside to within QR"),
    ADD_STAT(rh_move_within_qr,
        "RQ move within QR"),
    ADD_STAT(rh_move_to_qr_remove,
        "RQ move from outside to within QR with remove"),
    ADD_STAT(rh_move_within_qr_remove,
        "RQ move within QR with remove"),
    ADD_STAT(rh_drain_qr,
        "RQ rows drained from QR"),
    ADD_STAT(rh_cbf_true_pos,
        "RQ CBF true positives"),  
    ADD_STAT(rh_cbf_false_pos,
        "RQ CBF false positives"),
    ADD_STAT(rh_cbf_true_neg,
        "RQ CBF true negatives"),
    ADD_STAT(rh_btv_true_pos,
        "RQ BTV true positives"),
    ADD_STAT(rh_btv_false_pos,
        "RQ BTV false positives"),
    ADD_STAT(rh_btv_true_neg,  
        "RQ BTV true negatives"),
    ADD_STAT(rh_btv_occupancy_0,
        "RQ BTV with 0 remapped entries"),
    ADD_STAT(rh_btv_occupancy_1,
        "RQ BTV with 1 remapped entries"),
    ADD_STAT(rh_btv_occupancy_2,
        "RQ BTV with 2 remapped entries"),
    ADD_STAT(rh_btv_occupancy_3,
        "RQ BTV with 3+ remapped entries"),
    ADD_STAT(rh_cache_hit,
        "RQ cache hit"),
    ADD_STAT(rh_cache_partial_hit_orr_set,
        "RQ cache partial hit ORR set"),
    ADD_STAT(rh_cache_partial_hit_orr_unset,
        "RQ cache partial hit ORR unset"),
    ADD_STAT(rh_cache_miss,
        "RQ cache miss"),
    ADD_STAT(rh_cache_miss_entry_inserts,
        "RQ cache filled with missed entry"),
    ADD_STAT(rh_cache_miss_line_entry_inserts,
        "RQ cache filled with ORR line entry"),
    ADD_STAT(rh_cache_clean_evicts,
        "RQ cache clean evictions"),
    ADD_STAT(rh_cache_dirty_evicts,
        "RQ cache dirty evictions"),
    ADD_STAT(rh_rq_occupancy,
        "RQ quarantine region occupancy per 64ms"),
    ADD_STAT(rh_num_access_to_row_1,
        "RQ rows with 1 to 10 accesses per 64ms"),
    ADD_STAT(rh_num_access_to_row_10,
        "RQ rows with 10 to 100 accesses per 64ms"),
    ADD_STAT(rh_num_access_to_row_100,
        "RQ rows with 100 to 1K accesses per 64ms"),
    ADD_STAT(rh_num_access_to_row_1000,
        "RQ rows with 1K+ accesses per 64ms"),  
    ADD_STAT(rh_num_access_over_rh,
        "RQ rows with RTH+ accesses per 64ms")
{
    ;
}

void
MemCtrl::CtrlStats::regStats()
{
    using namespace Stats;

    assert(ctrl.system());
    const auto max_requestors = ctrl.system()->maxRequestors();

    avgRdQLen.precision(2);
    avgWrQLen.precision(2);

    readPktSize.init(ceilLog2(ctrl.system()->cacheLineSize()) + 1);
    writePktSize.init(ceilLog2(ctrl.system()->cacheLineSize()) + 1);

    rdQLenPdf.init(ctrl.readBufferSize);
    wrQLenPdf.init(ctrl.writeBufferSize);

    rdPerTurnAround
        .init(ctrl.readBufferSize)
        .flags(nozero);
    wrPerTurnAround
        .init(ctrl.writeBufferSize)
        .flags(nozero);

    avgRdBWSys.precision(2);
    avgWrBWSys.precision(2);
    avgGap.precision(2);

    // per-requestor bytes read and written to memory
    requestorReadBytes
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorWriteBytes
        .init(max_requestors)
        .flags(nozero | nonan);

    // per-requestor bytes read and written to memory rate
    requestorReadRate
        .flags(nozero | nonan)
        .precision(12);

    requestorReadAccesses
        .init(max_requestors)
        .flags(nozero);

    requestorWriteAccesses
        .init(max_requestors)
        .flags(nozero);

    requestorReadTotalLat
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorReadAvgLat
        .flags(nonan)
        .precision(2);

    requestorWriteRate
        .flags(nozero | nonan)
        .precision(12);

    requestorWriteTotalLat
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorWriteAvgLat
        .flags(nonan)
        .precision(2);

    for (int i = 0; i < max_requestors; i++) {
        const std::string requestor = ctrl.system()->getRequestorName(i);
        requestorReadBytes.subname(i, requestor);
        requestorReadRate.subname(i, requestor);
        requestorWriteBytes.subname(i, requestor);
        requestorWriteRate.subname(i, requestor);
        requestorReadAccesses.subname(i, requestor);
        requestorWriteAccesses.subname(i, requestor);
        requestorReadTotalLat.subname(i, requestor);
        requestorReadAvgLat.subname(i, requestor);
        requestorWriteTotalLat.subname(i, requestor);
        requestorWriteAvgLat.subname(i, requestor);
    }

    // Formula stats
    avgRdBWSys = (bytesReadSys / 1000000) / simSeconds;
    avgWrBWSys = (bytesWrittenSys / 1000000) / simSeconds;

    avgGap = totGap / (readReqs + writeReqs);

    requestorReadRate = requestorReadBytes / simSeconds;
    requestorWriteRate = requestorWriteBytes / simSeconds;
    requestorReadAvgLat = requestorReadTotalLat / requestorReadAccesses;
    requestorWriteAvgLat = requestorWriteTotalLat / requestorWriteAccesses;
}

void
MemCtrl::recvFunctional(PacketPtr pkt)
{
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        dram->functionalAccess(pkt);
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        nvm->functionalAccess(pkt);
   } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
   }
}

Port &
MemCtrl::getPort(const string &if_name, PortID idx)
{
    if (if_name != "port") {
        return QoS::MemCtrl::getPort(if_name, idx);
    } else {
        return port;
    }
}

bool
MemCtrl::allIntfDrained() const
{
   // ensure dram is in power down and refresh IDLE states
   bool dram_drained = !dram || dram->allRanksDrained();
   // No outstanding NVM writes
   // All other queues verified as needed with calling logic
   bool nvm_drained = !nvm || nvm->allRanksDrained();
   return (dram_drained && nvm_drained);
}

DrainState
MemCtrl::drain()
{
    // if there is anything in any of our internal queues, keep track
    // of that as well
    if (!(!totalWriteQueueSize && !totalReadQueueSize && respQueue.empty() &&
          allIntfDrained())) {

        DPRINTF(Drain, "Memory controller not drained, write: %d, read: %d,"
                " resp: %d\n", totalWriteQueueSize, totalReadQueueSize,
                respQueue.size());

        // the only queue that is not drained automatically over time
        // is the write queue, thus kick things into action if needed
        if (!totalWriteQueueSize && !nextReqEvent.scheduled()) {
            schedule(nextReqEvent, curTick());
        }

        if (dram)
            dram->drainRanks();

        return DrainState::Draining;
    } else {
        return DrainState::Drained;
    }
}

void
MemCtrl::drainResume()
{
    if (!isTimingMode && system()->isTimingMode()) {
        // if we switched to timing mode, kick things into action,
        // and behave as if we restored from a checkpoint
        startup();
        dram->startup();
    } else if (isTimingMode && !system()->isTimingMode()) {
        // if we switch from timing mode, stop the refresh events to
        // not cause issues with KVM
        if (dram)
            dram->suspend();
    }

    // update the mode
    isTimingMode = system()->isTimingMode();
}

MemCtrl::MemoryPort::MemoryPort(const std::string& name, MemCtrl& _ctrl)
    : QueuedResponsePort(name, &_ctrl, queue), queue(_ctrl, *this, true),
      ctrl(_ctrl)
{ }

AddrRangeList
MemCtrl::MemoryPort::getAddrRanges() const
{
    AddrRangeList ranges;
    if (ctrl.dram) {
        DPRINTF(DRAM, "Pushing DRAM ranges to port\n");
        ranges.push_back(ctrl.dram->getAddrRange());
    }
    if (ctrl.nvm) {
        DPRINTF(NVM, "Pushing NVM ranges to port\n");
        ranges.push_back(ctrl.nvm->getAddrRange());
    }
    return ranges;
}

void
MemCtrl::MemoryPort::recvFunctional(PacketPtr pkt)
{
    pkt->pushLabel(ctrl.name());

    if (!queue.trySatisfyFunctional(pkt)) {
        // Default implementation of SimpleTimingPort::recvFunctional()
        // calls recvAtomic() and throws away the latency; we can save a
        // little here by just not calculating the latency.
        ctrl.recvFunctional(pkt);
    }

    pkt->popLabel();
}

Tick
MemCtrl::MemoryPort::recvAtomic(PacketPtr pkt)
{
    return ctrl.recvAtomic(pkt);
}

bool
MemCtrl::MemoryPort::recvTimingReq(PacketPtr pkt)
{
    // pass it to the memory controller
    return ctrl.recvTimingReq(pkt);
}

MemCtrl*
MemCtrlParams::create()
{
    return new MemCtrl(this);
}

void MemCtrl::copy_rh_stats() {
    stats.rh_rrs_remapped_row_accessed = rh_defense->s_remapped_row_accessed;
    stats.rh_rrs_clean_install = rh_defense->s_clean_install;
    stats.rh_rrs_only_unswap = rh_defense->s_only_unswap;
    stats.rh_rrs_clean_reswap = rh_defense->s_clean_reswap;
    stats.rh_rrs_dirty_reswap = rh_defense->s_dirty_reswap;
    stats.rh_rrs_num_resets = rh_defense->s_num_resets;
    stats.rh_rrs_num_accesses = rh_defense->s_num_accesses;
    stats.rh_rrs_rand_row_gen_failed = rh_defense->s_randRowGenerationFailed;

    stats.rh_move_to_qr = rh_defense->s_move_to_qr;
    stats.rh_move_within_qr = rh_defense->s_move_within_qr;
    stats.rh_move_to_qr_remove = rh_defense->s_move_to_qr_remove;
    stats.rh_move_within_qr_remove = rh_defense->s_move_within_qr_remove;
    stats.rh_drain_qr = rh_defense->s_drain_qr;

    uint32_t eff_resets = (rh_defense->s_num_resets == 0 ? 1 : rh_defense->s_num_resets);
    
    stats.rh_num_access_to_row_1 = rh_defense->s_numLogRowAccesses[0]/eff_resets;
    stats.rh_num_access_to_row_10 = rh_defense->s_numLogRowAccesses[1]/eff_resets;
    stats.rh_num_access_to_row_100 = rh_defense->s_numLogRowAccesses[2]/eff_resets;
    stats.rh_num_access_to_row_1000 = rh_defense->s_numLogRowAccesses[3]/eff_resets;
    stats.rh_num_access_over_rh = rh_defense->s_numRowAccesses_RH/eff_resets;
    stats.rh_btv_occupancy_0 = rh_defense->s_btv_residency[0]/eff_resets;
    stats.rh_btv_occupancy_1 = rh_defense->s_btv_residency[1]/eff_resets;
    stats.rh_btv_occupancy_2 = rh_defense->s_btv_residency[2]/eff_resets;
    stats.rh_btv_occupancy_3 = rh_defense->s_btv_residency[3]/eff_resets;

    if (rh_mitigation == "RQ") {
        if (rh_defense->filter) {
            stats.rh_cbf_true_pos = rh_defense->filter->s_truePositives;
            stats.rh_cbf_false_pos = rh_defense->filter->s_falsePositives;
            stats.rh_cbf_true_neg = rh_defense->filter->s_trueNegatives;
        }
        else {
            stats.rh_btv_true_pos = rh_defense->s_btv_true_positives;
            stats.rh_btv_false_pos = rh_defense->s_btv_false_positives;
            stats.rh_btv_true_neg = rh_defense->s_btv_true_negatives;
        }
        stats.rh_cache_hit = rh_defense->s_cache_hit;
        stats.rh_cache_partial_hit_orr_set = rh_defense->s_cache_partial_hit_orr_set;
        stats.rh_cache_partial_hit_orr_unset = rh_defense->s_cache_partial_hit_orr_unset;
        stats.rh_cache_miss = rh_defense->s_cache_miss;
        stats.rh_cache_miss_entry_inserts = rh_defense->s_cache_miss_entry_inserts;
        stats.rh_cache_miss_line_entry_inserts = rh_defense->s_cache_miss_line_entry_inserts;
        stats.rh_cache_clean_evicts = rh_defense->s_cache_clean_evicts;
        stats.rh_cache_dirty_evicts = rh_defense->s_cache_dirty_evicts;
        stats.rh_rq_occupancy = rh_defense->s_QR_CumulativeOccupancy/eff_resets;
    }
}