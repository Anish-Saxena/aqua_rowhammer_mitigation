#include "mem/rowhammer_mitigation.hh"

Randomized_Row_Swap::Randomized_Row_Swap(uint32_t demandTuples, uint32_t __numRows, uint32_t __threshold,
                                        uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels) {
    tables  = (RIT ***) calloc (__numChannels, sizeof(RIT **));
    threshold = __threshold;
    numRows = __numRows;
    maxValidTuples = demandTuples;
    numTuples = uint32_t(1.428571*float(demandTuples)); // 6 extra ways for 14 demand ways, 42.8% overhead
    numBanks = __numBanks;
    numRanks = __numRanks;
    numChannels = __numChannels;
    last_reset = 0;
    cache = NULL;

    for (int ii = 0; ii < __numChannels; ii++) {
        tables[ii] = (RIT **) calloc (__numRanks, sizeof(RIT *));
        for (int jj = 0; jj < __numRanks; jj++) {
            tables[ii][jj] = (RIT *) calloc (__numBanks, sizeof(RIT));
            for (int kk = 0; kk < __numBanks; kk++) {
                tables[ii][jj][kk].tuples = (RIT_Entry *) calloc (numTuples, sizeof(RIT_Entry));
                tables[ii][jj][kk].rowHasEntry = (uint32_t *) calloc (numRows, sizeof(uint32_t));
            }
        }
    }
    srand(0xdeadbeef);

    uint32_t numDRAMRows = numRows*numBanks*numRanks*numChannels;
    numAccessesToRow = (uint32_t *)calloc(numDRAMRows, sizeof(uint32_t));

    s_numRowAccesses_RH = 0;
    for (int i = 0; i < 4; i++) {
        s_numLogRowAccesses[i] = 0;
    }

    s_remapped_row_accessed = 0;
    s_clean_install = 0;
    s_only_unswap = 0;
    s_clean_reswap = 0;
    s_dirty_reswap = 0;
    s_num_resets = 0;
    s_num_accesses = 0;
}

Addr Randomized_Row_Swap::getRemappedAddr(Addr rowAddr, uint8_t bankID, uint8_t rankID, 
                                            uint8_t channelID, bool isFuncLookup = false) {
    RIT *table = &(tables[channelID][rankID][bankID]);
    Addr remappedAddr = rowAddr;

    if (table->rowHasEntry[rowAddr]) {
        uint32_t entryID = table->rowHasEntry[rowAddr] - 1;
        assert(table->tuples[entryID].valid && 
                (table->tuples[entryID].origRowID == rowAddr || 
                table->tuples[entryID].remappedRowID == rowAddr));
        remappedAddr = (table->tuples[entryID].origRowID == rowAddr) ? 
                        table->tuples[entryID].remappedRowID : table->tuples[entryID].origRowID;
        if (isFuncLookup == false) {
            s_remapped_row_accessed++;
        }
    }

    return remappedAddr;
}

void Randomized_Row_Swap::insertTuple(Addr __origRowID, uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    RIT *table = &(tables[channelID][rankID][bankID]);
    uint8_t swap_op = CLEAN_INSTALL;
    uint32_t reswap_tuple_ID = numTuples;

    decision.isActionSwap[decision.validAddrs] = true;
    decision.addrs[decision.validAddrs] = __origRowID;
    decision.validAddrs++;

    Addr remapped_rowAddr = generateRandomRowAddr(__origRowID, bankID, rankID, channelID); // Get row Y

    decision.isActionSwap[decision.validAddrs] = true;
    decision.addrs[decision.validAddrs] = remapped_rowAddr;
    decision.validAddrs++;

    // printf("Installing for row %lld, <X, Y> is <%lld, %lld>\n", 
    //         __origRowID, __origRowID, remapped_rowAddr);

    // For swap-tuple <X, Y>, only X can potentially exist in RIT. Search if it is already in RIT
    if (table->rowHasEntry[__origRowID]) {
        uint32_t entryID = table->rowHasEntry[__origRowID] - 1;
        assert(table->tuples[entryID].valid && 
                (table->tuples[entryID].origRowID == __origRowID || 
                table->tuples[entryID].remappedRowID == __origRowID));
        reswap_tuple_ID = entryID;
    }

    if (reswap_tuple_ID < numTuples) {
        // To re-swap existing <X, A> to <X, Y>, <A, B>
        swap_op |= RESWAP;
        // Install <X, Y> in place of <X, A>
        Addr prev_swap_rowAddr;
        table->tuples[reswap_tuple_ID].locked = true;
        if (table->tuples[reswap_tuple_ID].origRowID == __origRowID) {
            prev_swap_rowAddr = table->tuples[reswap_tuple_ID].remappedRowID;
            table->tuples[reswap_tuple_ID].remappedRowID = remapped_rowAddr; 
            table->rowHasEntry[prev_swap_rowAddr] = 0;
            table->rowHasEntry[remapped_rowAddr] = reswap_tuple_ID + 1;
        }
        else {
            prev_swap_rowAddr = table->tuples[reswap_tuple_ID].origRowID;
            table->tuples[reswap_tuple_ID].origRowID = remapped_rowAddr;
            table->rowHasEntry[prev_swap_rowAddr] = 0;
            table->rowHasEntry[remapped_rowAddr] = reswap_tuple_ID + 1;
        }
        remapped_rowAddr = generateRandomRowAddr(prev_swap_rowAddr, bankID, rankID, channelID); // Get row B

        decision.isActionSwap[decision.validAddrs] = true;
        decision.addrs[decision.validAddrs] = prev_swap_rowAddr;
        decision.validAddrs++;
        decision.isActionSwap[decision.validAddrs] = true;
        decision.addrs[decision.validAddrs] = remapped_rowAddr;
        decision.validAddrs++;
        // printf("Installing for row %lld, <A, B> is <%lld, %lld>\n", 
        //         __origRowID, prev_swap_rowAddr, remapped_rowAddr);

        // Install <A, B>
        for (uint32_t ii = 0; ii < numTuples; ii++) {
            if (!table->tuples[ii].valid) {
                table->tuples[ii].valid = true;
                table->tuples[ii].locked = true;
                table->tuples[ii].origRowID = prev_swap_rowAddr;
                table->tuples[ii].remappedRowID = remapped_rowAddr;
                table->rowHasEntry[prev_swap_rowAddr] = ii + 1;
                table->rowHasEntry[remapped_rowAddr] = ii + 1;
                table->valid_tuples++;
                break;
            }
        }
    }
    else {
        // No re-swapping required, install <X, Y>
        for (uint32_t ii = 0; ii < numTuples; ii++) {
            if (!table->tuples[ii].valid) {
                table->tuples[ii].valid = true;
                table->tuples[ii].locked = true;
                table->tuples[ii].origRowID = __origRowID;
                table->tuples[ii].remappedRowID = remapped_rowAddr;
                table->rowHasEntry[__origRowID] = ii + 1;
                table->rowHasEntry[remapped_rowAddr] = ii + 1;
                table->valid_tuples++;
                break;
            }
        }
    }

    if (table->valid_tuples > maxValidTuples) {
        // Need to evict a valid but unlocked tuple, that is, un-swap
        swap_op |= UNSWAP;
        uint32_t ii;
        for (ii = 0; ii < numTuples; ii++) {
            if (table->tuples[ii].valid && !table->tuples[ii].locked) {
                table->tuples[ii].valid = false;
                table->tuples[ii].locked = false;

                decision.isActionSwap[decision.validAddrs] = true;
                decision.addrs[decision.validAddrs] = table->tuples[ii].origRowID;
                decision.validAddrs++;
                decision.isActionSwap[decision.validAddrs] = true;
                decision.addrs[decision.validAddrs] = table->tuples[ii].remappedRowID;
                decision.validAddrs++;
                // printf("Installing for row %lld, evicted tuple is <%lld, %lld>\n", 
                //         __origRowID, table->tuples[ii].origRowID, table->tuples[ii].remappedRowID);
                table->rowHasEntry[table->tuples[ii].origRowID] = 0;
                table->rowHasEntry[table->tuples[ii].remappedRowID] = 0;
                table->tuples[ii].origRowID = 0;
                table->tuples[ii].remappedRowID = 0;
                table->valid_tuples--;
                break;
            }
        }
        assert(ii<numTuples);
    }
    assert(table->valid_tuples <= maxValidTuples);

    if (swap_op == CLEAN_INSTALL) {
        s_clean_install++;
    }
    else if (swap_op == UNSWAP) {
        s_only_unswap++;
    }
    else if (swap_op == RESWAP) {
        s_clean_reswap++;
    }
    else {
        s_dirty_reswap++;
    }
}

Addr Randomized_Row_Swap::generateRandomRowAddr(Addr avoidRow, uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    RIT *table = &(tables[channelID][rankID][bankID]);
    Addr randRow = numRows;
    int max_tries = 10;
    while (max_tries) {
        max_tries--;
        uint32_t guessRow = rand()%numRows;
        if (guessRow != avoidRow && table->rowHasEntry[guessRow] == 0 && 
            detector->access(guessRow, bankID, rankID, channelID, /*Functional lookup*/ true) != true) {
            randRow = guessRow;
            break;
        }
    }
    if (randRow == numRows) {
        s_randRowGenerationFailed++;
        for (uint32_t ii = 0; ii < numRows; ii++) {
            if (ii != avoidRow && table->rowHasEntry[ii] == 0 && 
                detector->access(ii, bankID, rankID, channelID, /*Functional lookup*/ true) != true) {
                randRow = ii;
                break;
            } 
        }
    }

    assert(randRow < numRows);
    return randRow;
}

void Randomized_Row_Swap::reset() {
    for (int ii = 0; ii < numChannels; ii++) {
        for (int jj = 0; jj < numRanks; jj++) {
            for (int kk = 0; kk < numBanks; kk++) {
                for (int ll = 0; ll < numTuples; ll++) {
                    tables[ii][jj][kk].tuples[ll].locked = false;
                }
            }
        }
    }
    detector->reset();
    s_num_resets++;

    uint32_t numDRAMRows = numRows*numBanks*numRanks*numChannels;
    for (int rval = 0; rval < numDRAMRows; rval++) {
        if (numAccessesToRow[rval] >= threshold) {
            s_numRowAccesses_RH++;
        }
        int logval = 1;
        for (int i = 0; i < 4; i++) {
            int next_logval = 10*logval;
            if (numAccessesToRow[rval] >= logval && (numAccessesToRow[rval] < next_logval || i == 3)) {
                s_numLogRowAccesses[i]++;
                break;
            }
            logval = next_logval;
        }
        numAccessesToRow[rval] = 0;
    }
}

void Randomized_Row_Swap::resetDecision() {
    decision.validAddrs = 0;
    decision.remappedAddr = -1;
    decision.mitigate = false;
    decision.delay = 0;
    decision.MC_delay = 0;
    decision.SRAM_delay = 0;
    for (int i = 0; i < 6; i++) {
        decision.isActionSwap[i] = false;
        decision.addrs[i] = false;
    }
}

Decision& Randomized_Row_Swap::access(Tick currTick, Addr rowAddr, uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    s_num_accesses++;
    resetDecision();
    if (currTick - last_reset >= 64000000000) {
        last_reset = currTick;
        reset();
    }

    numAccessesToRow[bankID + numBanks*rowAddr]++;

    // decision.remappedAddr = getRemappedAddr(rowAddr, bankID, rankID, channelID);
    decision.mitigate = detector->access(rowAddr, bankID, rankID, channelID);
    decision.SRAM_delay += SRAM_RIT_accDelay;
    if (decision.mitigate) {
        insertTuple(rowAddr, bankID, rankID, channelID);
        decision.delay = (swapDelay)*decision.validAddrs/2;
    }
    return decision;
}

/***************************************************************************
 * CBF BEGINS
 * *************************************************************************/

void MurmurHash3_x64_128 ( const void * key, const int len, const uint32_t seed, void * out );

CountingBloomFilter::CountingBloomFilter(uint32_t __numEntries, uint32_t __maxCount, uint8_t __numHashes) {
    numHashes = __numHashes;
    maxCount = __maxCount;
    numEntries = __numEntries;
    entries = (uint32_t *) calloc (numEntries, sizeof(uint32_t));
}

std::array<uint64_t, 2> RQhash(const uint8_t *data, std::size_t len) {
    std::array<uint64_t, 2> hashValue;
    MurmurHash3_x64_128(data, len, 0xdeadbeef, hashValue.data());
    return hashValue;
}

inline uint64_t nthHash(uint8_t n, uint64_t hashA, uint64_t hashB, uint64_t filterSize) {
    return (hashA + n * hashB) % filterSize;
}

void CountingBloomFilter::add(uint32_t rowAddr) {
    uint8_t data[4] = { uint8_t(rowAddr & 0xff), uint8_t((rowAddr >> 8) & 0xff), 
                        uint8_t((rowAddr >> 16) & 0xff), uint8_t((rowAddr >> 24) & 0xff) };
    auto hashValues = RQhash(data, 4);
    for (int n = 0; n < numHashes; n++) {
        entries[nthHash(n, hashValues[0], hashValues[1], numEntries)]++;
    }
}

bool CountingBloomFilter::query(uint32_t rowAddr) {
    uint8_t data[4] = { uint8_t(rowAddr & 0xff), uint8_t((rowAddr >> 8) & 0xff), 
                        uint8_t((rowAddr >> 16) & 0xff), uint8_t((rowAddr >> 24) & 0xff) };
    auto hashValues = RQhash(data, 4);
    for (int n = 0; n < numHashes; n++) {
        if (!entries[nthHash(n, hashValues[0], hashValues[1], numEntries)]) {
            return false;
        }
    }
    return true;
}

void CountingBloomFilter::del(uint32_t rowAddr) {
    uint8_t data[4] = { uint8_t(rowAddr & 0xff), uint8_t((rowAddr >> 8) & 0xff), 
                        uint8_t((rowAddr >> 16) & 0xff), uint8_t((rowAddr >> 24) & 0xff) };
    auto hashValues = RQhash(data, 4);
    for (int n = 0; n < numHashes; n++) {
        if (entries[nthHash(n, hashValues[0], hashValues[1], numEntries)]) {
            entries[nthHash(n, hashValues[0], hashValues[1], numEntries)]--;
        }
    }
}

/***************************************************************************
 * CBF ENDS
 * *************************************************************************/

/***************************************************************************
 * FUNCTIONAL CACHE BEGINS
 * *************************************************************************/

/**************************************************
 * Constructs a set-associative cache level
**************************************************/
Functional_Cache::Functional_Cache(uint32_t sets, uint32_t ways){
    m_blk_offset = 5 /* BLK_OFFSET */;
    m_sets = sets;
    m_ways = ways;

    m_cache = (FuncCacheBlock **)calloc(sets, sizeof(FuncCacheBlock *));
    assert (m_cache != NULL);
    for (int i = 0; i < m_sets; i++){
        m_cache[i] = (FuncCacheBlock *)calloc(ways, sizeof(FuncCacheBlock));
    }

    s_lookups = 0;
    s_hits = 0;
    s_fills = 0;
    s_invalidations = 0;
    s_evictions = 0;
}

/**************************************************
 * Returns blk_way if addr exists in cache and updates 
 * LRU if lookup_type == LOOKUP_AND_UPDATE
**************************************************/
uint32_t Functional_Cache::Lookup(uint64_t addr, uint32_t lookup_type = LOOKUP_AND_UPDATE){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    uint64_t tag_bits = addr;
    bool hit = false;
    uint32_t blk_way = m_ways;

    for (int way = 0; way < m_ways; way++){
        if (m_cache[set][way].valid && m_cache[set][way].tag == tag_bits){
            hit = true;
            blk_way = way;
            break;
        }
    }

    if (lookup_type == LOOKUP_AND_UPDATE){
        s_lookups++;
        if (hit){
            s_hits++;
            m_cache[set][blk_way].rrip = 3;
        }
    }

    return blk_way;
}

/**************************************************
 * Returns blk_way if addr exists in cache 
**************************************************/
uint32_t Functional_Cache::PartialLookup(uint64_t addr){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    uint64_t partial_tag = addr/partial_lookup_factor;
    uint32_t blk_way = m_ways;

    for (int way = 0; way < m_ways; way++){
        if (m_cache[set][way].valid && 
            (m_cache[set][way].tag/partial_lookup_factor) == partial_tag){
            blk_way = way;
            break;
        }
    }

    if (blk_way < m_ways) {
        m_cache[set][blk_way].rrip = 3;
    }

    return blk_way;
}

/**************************************************
 * Updates LRU and fills block at (set, blk_way)
**************************************************/
void Functional_Cache::Fill(uint64_t addr, uint32_t blk_way){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    uint64_t tag_bits = addr;
    assert(m_cache[set][blk_way].valid == false);

    m_cache[set][blk_way].valid = true;
    m_cache[set][blk_way].tag = tag_bits;
    m_cache[set][blk_way].rrip = 1;
    m_cache[set][blk_way].dirty = false;
    m_cache[set][blk_way].onlyOneRowRemapped = true;
    s_fills++;

    return;
}

/**************************************************
 * Updates LRU and invalidates block at (set, blk_way)
**************************************************/
void Functional_Cache::Invalidate(uint64_t addr, uint32_t blk_way){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    if (m_cache[set][blk_way].valid == false){
        return;
    }

    m_cache[set][blk_way].valid = false;
    m_cache[set][blk_way].tag = 0;
    m_cache[set][blk_way].rrip = 0;
    m_cache[set][blk_way].dirty = false;
    m_cache[set][blk_way].onlyOneRowRemapped = false;
    s_invalidations++;

    return;
}

/**************************************************
 * Returns victim way in set which is either any one
 * invalid way or LRU way if none are invalid
**************************************************/
uint32_t Functional_Cache::Find_Victim(uint64_t addr){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    uint32_t victim_way = m_ways;

    for (int way = 0; way < m_ways; way++){
        if (m_cache[set][way].valid == false){
            victim_way = way;
            break;
        }
    }

    if (victim_way == m_ways){
        s_evictions++;
        int loop_count = 4;
        while (loop_count) {
            for (int way = 0; way < m_ways; way++){
                if (m_cache[set][way].rrip == 0){
                    victim_way = way;
                    break;
                }
            }
            if (victim_way < m_ways) {
                break;
            }
            for (int way = 0; way < m_ways; way++){
                m_cache[set][way].rrip--;
            }
            loop_count--;
        }
    }

    assert(victim_way < m_ways);

    return victim_way;
}

/**************************************************
 * Returns true if block at (set, blk_way) is valid
**************************************************/
bool Functional_Cache::Is_Block_Valid(uint64_t addr, uint32_t blk_way){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    
    return m_cache[set][blk_way].valid;
}

/**************************************************
 * Returns block aligned address
 * the block at (set, blk_way) must be valid
**************************************************/
uint64_t Functional_Cache::Get_Block_Addr(uint64_t addr, uint32_t blk_way){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    assert(m_cache[set][blk_way].valid == true);

    return (m_cache[set][blk_way].tag);
}

/**************************************************
 * Returns the way of block corresponding to addr
 * the block must exist in the set
**************************************************/
uint32_t Functional_Cache::Get_Block_Way(uint64_t addr){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    uint64_t tag_bits = addr;
    uint32_t blk_way = m_ways;

    for (int way = 0; way < m_ways; way++){
        if (m_cache[set][way].valid && m_cache[set][way].tag == tag_bits){
            blk_way = way;
            break;
        }
    }
    assert (blk_way < m_ways);
    return blk_way;
}

void Functional_Cache::SetORRBit(uint64_t addr, bool bitvalue){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    uint64_t partial_tag = addr/partial_lookup_factor;
    for (int way = 0; way < m_ways; way++){
        if (m_cache[set][way].valid && 
            (m_cache[set][way].tag/partial_lookup_factor) == partial_tag){
            m_cache[set][way].onlyOneRowRemapped = bitvalue;
        }
    }
}

bool Functional_Cache::QueryORRBit(uint64_t addr, uint32_t blk_way){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    assert(m_cache[set][blk_way].valid == true);
    return m_cache[set][blk_way].onlyOneRowRemapped;
}

void Functional_Cache::SetDirtyBit(uint64_t addr, uint32_t blk_way){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    assert(m_cache[set][blk_way].valid == true);
    m_cache[set][blk_way].dirty = true;
}

bool Functional_Cache::QueryDirtyBit(uint64_t addr, uint32_t blk_way){
    uint32_t set = (addr >> m_blk_offset) % m_sets;
    assert(m_cache[set][blk_way].valid == true);
    return m_cache[set][blk_way].dirty;
}

void Functional_Cache::Print_Stats(uint32_t level){
    // cout << right << setw(15) << "L" << level << " Stats" << endl;
    // cout << left;
    // cout << setw(30) << "Lookups: " << s_lookups << endl;
    // cout << setw(30) << "Hits: " << s_hits << endl;
    // cout << setw(30) << "Misses: " << s_lookups - s_hits << endl;
    // cout << setw(30) << "Fills: " << s_fills << endl;
    // cout << setw(30) << "Evictions: " << s_evictions << endl;
    // cout << setw(30) << "Invalidations: " << s_invalidations << endl;
}

/***************************************************************************
 * FUNCTIONAL CACHE ENDS
 * *************************************************************************/

Row_Quarantine::Row_Quarantine(uint32_t __numQREntries, uint32_t __numRows, uint32_t __threshold,
                                uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels) {
    // A single, global Q region
    numQREntries = __numQREntries;
    threshold = __threshold;
    // numRows is effectively numDRAMRows
    numRows = __numRows*__numBanks*__numRanks*__numChannels;    
    numBanks = 1;
    numRanks = 1;
    numChannels = 1;
    last_reset = 0;

    indirection  = (IndirectionTables ***) calloc (numChannels, sizeof(IndirectionTables **));
    for (int ii = 0; ii < numChannels; ii++) {
        indirection[ii] = (IndirectionTables **) calloc (numRanks, sizeof(IndirectionTables *));
        for (int jj = 0; jj < numRanks; jj++) {
            indirection[ii][jj] = (IndirectionTables *) calloc (numBanks, sizeof(IndirectionTables));
            for (int kk = 0; kk < numBanks; kk++) {
                indirection[ii][jj][kk].row_To_QR_T = (Entry *) calloc (numRows, sizeof(Entry));
                indirection[ii][jj][kk].QR_To_Row_T = (Entry *) calloc (numQREntries, sizeof(Entry));
                indirection[ii][jj][kk].nextFreeQR_ID = 0;
                indirection[ii][jj][kk].prevQRHead_ID = 0;
                indirection[ii][jj][kk].QRTail_ID = 0;
            }
        }
    }
    srand(0xdeadbeef);

    uint32_t numDRAMRows = numRows*numBanks*numRanks*numChannels;
    randRowID = (uint32_t *) calloc (numDRAMRows, sizeof(uint32_t));
    for (int ii = 0; ii < numDRAMRows; ii++) {
        randRowID[ii] = ii;
    }
    for (int ii = 0; ii < numDRAMRows; ii++) {
        int jj = rand()%numDRAMRows;
        uint32_t temp = randRowID[ii];
        randRowID[ii] = randRowID[jj];
        randRowID[jj] = temp;
    }

    bitvector = (uint32_t *)calloc(numDRAMRows, sizeof(uint32_t));
    numAccessesToRow = (uint32_t *)calloc(numDRAMRows, sizeof(uint32_t));
    ORR_CBF = (uint32_t *)calloc(numDRAMRows, sizeof(uint32_t));
    prev_drain_cache_miss_val = 0; 

    s_remapped_row_accessed = 0;
    s_move_to_qr = 0;
    s_move_within_qr = 0;
    s_move_to_qr_remove = 0;
    s_move_within_qr_remove = 0;
    s_drain_qr = 0;
    s_num_resets = 0;
    s_num_accesses = 0;
    s_btv_true_positives = 0;
    s_btv_false_positives = 0;
    s_btv_true_negatives = 0;
    s_cache_hit = 0;
    s_cache_partial_hit_orr_set = 0;
    s_cache_partial_hit_orr_unset = 0;
    s_cache_miss = 0;
    s_QR_CumulativeOccupancy = 0;
    s_numRowAccesses_RH = 0;
    for (int i = 0; i < 4; i++) {
        s_numLogRowAccesses[i] = 0;
        s_btv_residency[i] = 0;
    }
}

void Row_Quarantine::resetDecision() {
    decision.validAddrs = 0;
    decision.remappedAddr = -1;
    decision.mitigate = false;
    decision.delay = 0;
    decision.MC_delay = 0;
    decision.SRAM_delay = 0;
    for (int i = 0; i < 6; i++) {
        decision.isActionSwap[i] = false;
        decision.addrs[i] = false;
    }
}

uint32_t Row_Quarantine::generateCBFID(Addr rowID, uint8_t bankID, 
                                        uint8_t rankID, uint8_t channelID) {
    return (bankID + numBanks*rowID);
}

void Row_Quarantine::decodeAddr(Addr addr, DecodedAddr *mapping) {
    // Hardcoded for detector
    uint32_t rowAddr = addr/8192; 
    numAccessesToRow[rowAddr]++;
    rowAddr = randRowID[rowAddr];
    mapping->channelID = 0;
    mapping->rankID = 0;
    mapping->bankID = rowAddr % 16;
    rowAddr /= 16;
    mapping->rowID = (rowAddr % (numRows/16)); // numRows is actually numDRAMRows
    return;
}

Decision& Row_Quarantine::access(Tick currTick, /*Byte-aligned address */ Addr addr, 
                            /* Not required for RQ */ uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    s_num_accesses++;
    resetDecision();

    if (currTick - last_reset >= 64000000000) {
        last_reset = currTick;
        reset();
    }

    addQCheckDelay(randRowID[addr/8192], 0, 0, 0);
    
    // Detector is still per-bank, so use decoded address
    DecodedAddr mapping;
    decodeAddr(addr, &mapping);
    decision.mitigate = detector->access(mapping.rowID, mapping.bankID, 0, 0);

    if (decision.mitigate) {
        quarantineRow(randRowID[addr/8192], 0, 0, 0);
    }
    else if ((s_cache_miss + s_cache_partial_hit_orr_unset - prev_drain_cache_miss_val) >= drain_cache_miss_threshold) {
        prev_drain_cache_miss_val = s_cache_miss + s_cache_partial_hit_orr_unset;
        drainRow();
    }

    return decision;
}

bool Row_Quarantine::removeRowFromCache(Addr index, Addr &evicted_rowID, Addr remove_rowID) {
    // Remove row from cache, if it exists
    uint32_t blk_way = cache->Lookup(index, ONLY_LOOKUP);
    bool isDirty = false;
    evicted_rowID = 0;
    if (blk_way < cache->m_ways) {
        // row is in cache, invalidate it and write-back if required
        isDirty = cache->QueryDirtyBit(index, blk_way);
        evicted_rowID = cache->Get_Block_Addr(index, blk_way);
        cache->Invalidate(index, blk_way);
        if (isDirty) {
            s_cache_dirty_evicts++;
        }
        else {
            s_cache_clean_evicts++;
        }
    }
    return isDirty;
}

bool Row_Quarantine::insertRowInCache(Addr index, Addr &evicted_rowID, bool setDirty = false) {
    bool isDirty = false;
    uint64_t blk_way = cache->Find_Victim(index);
    evicted_rowID = 0;
    if (cache->Is_Block_Valid(index, blk_way)) {
        isDirty = cache->QueryDirtyBit(index, blk_way);
        evicted_rowID = cache->Get_Block_Addr(index, blk_way);
        if (isDirty) {
            s_cache_dirty_evicts++;
        }
        else {
            s_cache_clean_evicts++;
        }
    }
    cache->Invalidate(index, blk_way);
    cache->Fill(index, blk_way);
    if (setDirty) {
        cache->SetDirtyBit(index, blk_way);
    }
    return isDirty;
}

void Row_Quarantine::addQCheckDelay(Addr rowID, uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    IndirectionTables *indirStructs = &(indirection[channelID][rankID][bankID]);
    uint32_t index = generateCBFID(rowID, bankID, rankID, channelID);
    decision.SRAM_delay += SRAM_CBF_accDelay;
    if ((filter && filter->query(index)) || (bitvector && bitvector[index/rows_per_btv_bit])) {
        // A is possibly remapped, check cache for entry and ORR
        if (cache) {
            decision.SRAM_delay += SRAM_Cache_accDelay;
            if (cache->Lookup(index) < cache->m_ways) {
                // Found row remapping info in cache
                s_cache_hit++;
            }
            else {
                uint32_t RQT_cacheline_way = cache->PartialLookup(index);
                bool cache_miss = false;
                if (RQT_cacheline_way < cache->m_ways) {
                    // Found RQT cacheline remapping info
                    if (ORR_CBF[index/rows_per_btv_bit] == 1) {
                        // ORR bit is set, A is not remapped
                        s_cache_partial_hit_orr_set++;
                    }
                    else {
                        // A may be remapped, check {Row->QR} table
                        cache_miss = true;
                        s_cache_partial_hit_orr_unset++;
                    }
                }
                else {
                    // No remapping info in cache, check {Row->QR} table
                    cache_miss = true;
                    s_cache_miss++;
                }
                if (cache_miss) {
                    if (virtualize_RIT) {
                        // We will have 2 row buffer conflicts, so
                        // Delay is 90ns, the fine-tuning is done at DRAM side
                        decision.delay += 2*accDelay;
                    }
                    else {
                        decision.SRAM_delay += SRAM_RQT_accDelay;
                    }
                    // Check if a single remapped entry exists in group and insert it
                    uint32_t inserted_rowID = numRows;
                    if (indirStructs->row_To_QR_T[rowID].valid) {
                        s_cache_miss_entry_inserts++;
                        inserted_rowID = rowID;
                    }
                    else {
                        for (uint32_t i = (rowID/rows_per_btv_bit)*rows_per_btv_bit; 
                            i < (rowID/rows_per_btv_bit)*rows_per_btv_bit + rows_per_btv_bit; i++) {
                            if (indirStructs->row_To_QR_T[i].valid) {
                                inserted_rowID = i;
                                break;
                            }
                        }
                        if (ORR_CBF[index/rows_per_btv_bit] == 1) {
                            s_cache_miss_line_entry_inserts++;
                        }
                    }
                    if (inserted_rowID < numRows && 
                        (inserted_rowID == rowID || (ORR_CBF[index/rows_per_btv_bit] == 1))) {
                        // Insert entry in cache
                        index = generateCBFID(inserted_rowID, bankID, rankID, channelID);
                        Addr evicted_rowID;
                        bool isDirty = insertRowInCache(index, evicted_rowID);
                        if (virtualize_RIT && isDirty && 
                            (rowID/32 != evicted_rowID/32)) {
                            // Update evicted entry in {Row->QR} table
                            // We first fetch it, then re-write updated value
                            decision.delay += 2*accDelay;
                        }
                    }
                }
            }
        }
        else {
            // Check {Row->QR} table
            if (virtualize_RIT) {
                // We will have 2 row buffer conflicts, so
                // Delay is 90ns, the fine-tuning is done at DRAM side
                decision.delay += 2*accDelay;
            }
            else {
                decision.SRAM_delay += SRAM_RQT_accDelay;
            }
        }

        // We now have the mapping A->Qj, if it exists
        if (filter && indirStructs->row_To_QR_T[rowID].valid) {
            filter->s_truePositives++;
        } 
        else if (filter) {
            filter->s_falsePositives++;
        }
        else if (indirStructs->row_To_QR_T[rowID].valid) {
            s_btv_true_positives++;
        }
        else {
            s_btv_false_positives++;
        }
    }
    else {
        if (filter) {
            filter->s_trueNegatives++;
        }
        else {
            s_btv_true_negatives++;
        }
        // CBF or BTV cannot give false negative
        assert(indirStructs->row_To_QR_T[rowID].valid == false);
    }
}

bool Row_Quarantine::removeRow(Addr QRT_id, uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    IndirectionTables *indirStructs = &(indirection[channelID][rankID][bankID]);
    // Remove row from Qi
    uint32_t remove_rowID = indirStructs->QR_To_Row_T[QRT_id].value; // get B
    indirStructs->QR_To_Row_T[QRT_id].valid = false;
    // Remove B from Qi
    // Reset B->Qi mapping in {Row->QR} table
    assert(indirStructs->row_To_QR_T[remove_rowID].valid);
    indirStructs->row_To_QR_T[remove_rowID].valid = false; // reset B->Qi
    // Reset CBF entry for B
    uint32_t index = generateCBFID(remove_rowID, bankID, rankID, channelID);
    if (filter) {
        assert(filter->query(index));
        filter->del(index);
    }
    else {
        assert(bitvector[index/rows_per_btv_bit]);
        bitvector[index/rows_per_btv_bit]--;
    }
    bool isDirty = false;
    if (cache) {
        Addr evicted_rowID;
        isDirty = removeRowFromCache(index, evicted_rowID, remove_rowID);
        ORR_CBF[index/rows_per_btv_bit]--;
    }
    return isDirty;
}

void Row_Quarantine::drainRow() {
    IndirectionTables *indirStructs = &(indirection[0][0][0]);
    // Fetch {QR->Row} table entry to get rowID
    printf("[RQ] DRAIN function- HEAD: %ld, PREV-HEAD: %ld, TAIL: %ld VALID? %d\n",
            indirStructs->nextFreeQR_ID, indirStructs->prevQRHead_ID, 
            indirStructs->QRTail_ID, indirStructs->QR_To_Row_T[indirStructs->QRTail_ID].valid);
    if (virtualize_RIT) {
        decision.delay += accDelay;
    }
    if ((indirStructs->QRTail_ID != indirStructs->prevQRHead_ID) && 
        indirStructs->QR_To_Row_T[indirStructs->QRTail_ID].valid) {
        // Tail pointer is valid, remove rowID from QR
        s_drain_qr++;
        bool isCacheDirty = removeRow(indirStructs->QRTail_ID, 0, 0, 0);
        decision.delay += moveDelay;
        // Add delay to update RQT if cached entry is dirty
        if (cache && virtualize_RIT && isCacheDirty) {
            decision.delay += accDelay;
        }
        else if (virtualize_RIT) {
            decision.delay += accDelay;
        }
        // Add delay to update QRT
        if (virtualize_RIT) {
            decision.delay += accDelay;
        }
        indirStructs->QRTail_ID = (indirStructs->QRTail_ID + 1)%numQREntries;
    }
}

void Row_Quarantine::quarantineRow(Addr rowID, uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    IndirectionTables *indirStructs = &(indirection[channelID][rankID][bankID]);
    uint8_t move_op = MOVE_TO_QR;

    /************************
     * Move row A, which is possibly already in Qj, to Qi
     * If Qi has row B, remove it
     * **********************/

    // If A is in Qj (we already have this info, don't need to add access delay)
    // update {Row->QR} table with A->Qi mapping and add update delay accordingly 
    bool origRowQuaranteed = false;
    uint32_t orig_RowQRID;
    if (indirStructs->row_To_QR_T[rowID].valid) {
        move_op |= MOVE_WITHIN_QR;
        // A is in Qj
        orig_RowQRID = indirStructs->row_To_QR_T[rowID].value; // get Qj
        origRowQuaranteed = true;
    }
    else {
        // A doesn't yet have an entry, set CBF entry
        uint32_t index = generateCBFID(rowID, bankID, rankID, channelID);
        if (filter) {
            filter->add(index);
        }
        else {
            // Set BTV entry
            bitvector[index/rows_per_btv_bit]++;
            ORR_CBF[index/rows_per_btv_bit]++;
        }
    }
    indirStructs->row_To_QR_T[rowID].valid = true;
    indirStructs->row_To_QR_T[rowID].value = indirStructs->nextFreeQR_ID; // put A->Qi, reset A->Qj

    if (cache) {
        // Add A in cache
        uint32_t index = generateCBFID(rowID, bankID, rankID, channelID);
        uint32_t blk_way = cache->Lookup(index, ONLY_LOOKUP);
        if (blk_way == cache->m_ways) {
            // // A is not in cache, insert A->Qi
            Addr evicted_rowID;
            bool isDirty = insertRowInCache(index, evicted_rowID, /*mark dirty*/ true);
            // Update evicted entry in {Row->QR} table
            // We first fetch it, then re-write updated value
            if (virtualize_RIT && isDirty) {
                decision.delay += 2*accDelay;
            }
        }
        else {
            cache->SetDirtyBit(index, blk_way);
        }
    }
    else if (virtualize_RIT) {
        // This is the update delay for A's entry in {Row->QR} table
        decision.delay += accDelay;
    }

    // Check if B is in Qi and add access delay for {QR->Row} table
    if (virtualize_RIT) {
        decision.delay += accDelay;
    }
    if (indirStructs->QR_To_Row_T[indirStructs->nextFreeQR_ID].valid) {
        // Qi has B, remove it
        move_op |= REMOVE;
        uint32_t remove_rowID = indirStructs->QR_To_Row_T[indirStructs->nextFreeQR_ID].value; // get B
        bool isCacheDirty = removeRow(indirStructs->nextFreeQR_ID, bankID, rankID, channelID);
        decision.delay += moveDelay;
        if (cache && virtualize_RIT && isCacheDirty && 
                (origRowQuaranteed && (rowID/32) != (remove_rowID/32))) {
            decision.delay += accDelay;
        }
        else if (virtualize_RIT && (origRowQuaranteed && (rowID/32) != (remove_rowID/32))) {
            // Add update delay depending on whether {Row->QR} table 
            // has B->Qi and A->Qj are in same cacheline 
            // 2B per entry, so 32 entries per cacheline
            decision.delay += accDelay;
        }
    }

    // Move A to Qi
    decision.delay += moveDelay;
    // Update Qi mapping in {QR->Row} table
    if (virtualize_RIT) {
        decision.delay += accDelay;
    }
    indirStructs->QR_To_Row_T[indirStructs->nextFreeQR_ID].valid = true;
    indirStructs->QR_To_Row_T[indirStructs->nextFreeQR_ID].value = rowID; // put Qi->A, reset Qi->B
    // Add update delay depending on whether {QR->Row} table has Qi->B and Qj->A are in same cacheline
    if (origRowQuaranteed) {
        // 4B per entry, so 16 entries per cacheline
        if (virtualize_RIT && ((indirStructs->nextFreeQR_ID/16) != (orig_RowQRID/16))) {
            decision.delay += accDelay;
        }
        assert(indirStructs->QR_To_Row_T[orig_RowQRID].valid);
        indirStructs->QR_To_Row_T[orig_RowQRID].valid = false; // reset Qj->A
    }

    if (indirStructs->nextFreeQR_ID == indirStructs->QRTail_ID && 
        indirStructs->nextFreeQR_ID != indirStructs->prevQRHead_ID) {
        // Head wrapped around and reached tail, increment it
        indirStructs->QRTail_ID = (indirStructs->QRTail_ID + 1)%numQREntries;
    }
    // Increment next free QR ID
    indirStructs->nextFreeQR_ID = (indirStructs->nextFreeQR_ID + 1)%numQREntries;


    if (move_op == MOVE_TO_QR) {
        s_move_to_qr++;
    }
    else if (move_op == MOVE_WITHIN_QR) {
        s_move_within_qr++;
    }
    else if (move_op == (MOVE_TO_QR|REMOVE)) {
        s_move_to_qr_remove++;
    }
    else {
        s_move_within_qr_remove++;
    }
}

void Row_Quarantine::reset() {
    detector->reset();
    s_num_resets++;
    for (int ii = 0; ii < numChannels; ii++) {
        for (int jj = 0; jj < numRanks; jj++) {
            for (int kk = 0; kk < numBanks; kk++) {
                IndirectionTables *indirStructs = &(indirection[ii][jj][kk]);
                for (int ll = 0; ll < numQREntries; ll++) {
                    if (indirStructs->QR_To_Row_T[ll].valid) {
                        s_QR_CumulativeOccupancy++;
                    }
                }
                indirStructs->prevQRHead_ID = indirStructs->nextFreeQR_ID;
            }
        }
    }
    uint32_t numDRAMRows = numRows*numBanks*numRanks*numChannels;
    for (int rval = 0; rval < numDRAMRows; rval++) {
        if (numAccessesToRow[rval] >= threshold) {
            s_numRowAccesses_RH++;
        }
        int logval = 1;
        for (int i = 0; i < 4; i++) {
            int next_logval = 10*logval;
            if (numAccessesToRow[rval] >= logval && (numAccessesToRow[rval] < next_logval || i == 3)) {
                s_numLogRowAccesses[i]++;
                break;
            }
            logval = next_logval;
        }
        numAccessesToRow[rval] = 0;
        if (rval < numDRAMRows/rows_per_btv_bit) {
            s_btv_residency[bitvector[rval] > 3 ? 3 : bitvector[rval]]++;
        }
    }
}

BlockHammer::BlockHammer(uint32_t __threshold, uint32_t __numRows, uint32_t __BLThreshold,
                                uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels) {
    threshold = __threshold;
    BlackListThreshold = __BLThreshold;
    numRows = __numRows;    
    numBanks = __numBanks;
    numRanks = __numRanks;
    numChannels = __numChannels;
    last_reset = 0;

    uint32_t numDRAMRows = numRows*numBanks*numRanks*numChannels;
    numAccessesToRow = (uint32_t *)calloc(numDRAMRows, sizeof(uint32_t));

    s_numRowAccesses_RH = 0;
    for (int i = 0; i < 4; i++) {
        s_numLogRowAccesses[i] = 0;
    }
}

void BlockHammer::resetDecision() {
    decision.validAddrs = 0;
    decision.remappedAddr = -1;
    decision.mitigate = false;
    decision.delay = 0;
    decision.SRAM_delay = 0;
    decision.MC_delay = 0;
    for (int i = 0; i < 6; i++) {
        decision.isActionSwap[i] = false;
        decision.addrs[i] = false;
    }
}

Decision& BlockHammer::access(Tick currTick, Addr rowID, uint8_t bankID, uint8_t rankID, uint8_t channelID) {
    s_num_accesses++;
    resetDecision();
    if (currTick - last_reset >= 64000000000) {
        last_reset = currTick;
        reset();
    }

    uint32_t DRAMRowID = bankID + numBanks*rowID;
    numAccessesToRow[DRAMRowID]++;

    if (numAccessesToRow[DRAMRowID] >= BlackListThreshold) {
        Tick ACTs_Remaining;
        if (threshold > numAccessesToRow[DRAMRowID] - 1) {
            ACTs_Remaining = threshold - numAccessesToRow[DRAMRowID] - 1;
        }
        else {
            ACTs_Remaining = 1;
        }
        decision.MC_delay = (64000000000 + last_reset - currTick)/ACTs_Remaining;
        decision.mitigate = true;
    }

    decision.SRAM_delay += SRAM_CBF_accDelay;
    return decision;
}

void BlockHammer::reset() {
    s_num_resets++;

    uint32_t numDRAMRows = numRows*numBanks*numRanks*numChannels;
    for (int rval = 0; rval < numDRAMRows; rval++) {
        if (numAccessesToRow[rval] >= threshold) {
            s_numRowAccesses_RH++;
        }
        if (numAccessesToRow[rval] >= BlackListThreshold) {
            s_numLogRowAccesses[3]++;
        }
        int logval = 1;
        for (int i = 0; i < 3; i++) {
            int next_logval = 10*logval;
            if (numAccessesToRow[rval] >= logval && (numAccessesToRow[rval] < next_logval || i == 2)) {
                s_numLogRowAccesses[i]++;
                break;
            }
            logval = next_logval;
        }
        numAccessesToRow[rval] = 0;
    }
}

/***************************************************************************
 * Hashing function: murmur hash 3 BEGINS
 * *************************************************************************/

// Other compilers

#define	FORCE_INLINE inline __attribute__((always_inline))

inline uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}

inline uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

FORCE_INLINE uint32_t getblock32 ( const uint32_t * p, int i )
{
  return p[i];
}

FORCE_INLINE uint64_t getblock64 ( const uint64_t * p, int i )
{
  return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

FORCE_INLINE uint32_t fmix32 ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

//----------

FORCE_INLINE uint64_t fmix64 ( uint64_t k )
{
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

//-----------------------------------------------------------------------------

void MurmurHash3_x64_128 ( const void * key, const int len,
                           const uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(int i = 0; i < nblocks; i++)
  {
    uint64_t k1 = getblock64(blocks,i*2+0);
    uint64_t k2 = getblock64(blocks,i*2+1);

    k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= ((uint64_t)tail[14]) << 48;
  case 14: k2 ^= ((uint64_t)tail[13]) << 40;
  case 13: k2 ^= ((uint64_t)tail[12]) << 32;
  case 12: k2 ^= ((uint64_t)tail[11]) << 24;
  case 11: k2 ^= ((uint64_t)tail[10]) << 16;
  case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
  case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
           k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
  case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
  case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
  case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
  case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
  case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
  case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
  case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
           k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  ((uint64_t*)out)[0] = h1;
  ((uint64_t*)out)[1] = h2;
}

/***************************************************************************
 * Hashing function: murmur hash 3 ENDS
 * *************************************************************************/
