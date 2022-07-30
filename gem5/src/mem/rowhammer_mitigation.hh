#ifndef RH_MITIGATION
#define RH_MITIGATION

#include "mem/rowhammer_detector.hh"

using namespace std;

typedef struct __Decision {
    bool                mitigate;
    bool                isActionSwap[6];
    uint32_t            validAddrs;
    Addr                addrs[6];
    Addr                remappedAddr;
    Tick                delay;
    Tick                SRAM_delay;
    Tick                MC_delay; // BlockHammer
} Decision;

typedef struct __DecodedAddr {
    uint32_t            rowID;
    uint8_t             bankID;
    uint8_t             rankID;
    uint8_t             channelID;
} DecodedAddr;

typedef struct FuncCacheBlock{
    uint64_t tag;
    bool valid;
    bool dirty;
    bool onlyOneRowRemapped;
    uint32_t rrip;
} FuncCacheBlock;

enum LOOKUP_TYPE{
    ONLY_LOOKUP,
    LOOKUP_AND_UPDATE
};

class Functional_Cache{
private:
    uint32_t m_blk_offset;
    FuncCacheBlock **m_cache;

    uint64_t s_lookups;
    uint64_t s_hits;
    uint64_t s_fills;
    uint64_t s_invalidations;
    uint64_t s_evictions;

public:
    uint32_t m_sets;
    uint32_t m_ways;
    uint32_t partial_lookup_factor;
    Functional_Cache(uint32_t sets, uint32_t ways);
    uint32_t Lookup(uint64_t addr, uint32_t lookup_type);
    uint32_t PartialLookup(uint64_t addr);
    void Invalidate(uint64_t addr, uint32_t blk_way);
    void Fill(uint64_t addr, uint32_t blk_way);
    uint32_t Find_Victim(uint64_t addr);
    bool Is_Block_Valid(uint64_t addr, uint32_t blk_way);
    uint64_t Get_Block_Addr(uint64_t addr, uint32_t blk_way);
    uint32_t Get_Block_Way(uint64_t addr);
    void SetDirtyBit(uint64_t addr, uint32_t blk_way);
    bool QueryDirtyBit(uint64_t addr, uint32_t blk_way);
    void SetORRBit(uint64_t addr, bool bitvalue);
    bool QueryORRBit(uint64_t addr, uint32_t blk_way);
    void Print_Stats(uint32_t level);
};

typedef class CountingBloomFilter {
private:
    uint8_t                 numHashes;
    uint32_t                maxCount;
    uint32_t                numEntries;
    uint32_t                *entries;

public:
    uint64_t                s_truePositives;
    uint64_t                s_falsePositives;
    uint64_t                s_trueNegatives;

    CountingBloomFilter(uint32_t __numEntries, uint32_t __maxCount, uint8_t __numHashes);
    void add(uint32_t rowAddr);
    bool query(uint32_t rowAddr);
    void del(uint32_t rowAddr);
} CountingBloomFilter;

class RH_Mitigation {
protected:
    uint32_t            numRows;
    uint8_t             numBanks;
    uint8_t             numRanks;
    uint8_t             numChannels;
    Tick                last_reset;
    uint32_t            threshold;

public:
    RH_Detector         *detector;
    Decision            decision;
    CountingBloomFilter *filter; // RQ
    uint32_t            rows_per_btv_bit;
    uint32_t            *bitvector; // RQ
    uint32_t            *ORR_CBF;
    uint32_t            *numAccessesToRow;
    Functional_Cache    *cache; // RQ
    Tick                moveDelay; // RQ
    Tick                accDelay; // RQ
    Tick                swapDelay; // RRS
    bool                virtualize_RIT; // RQ
    Tick                SRAM_RQT_accDelay; // RQ
    Tick                SRAM_QRT_accDelay; // RQ
    Tick                SRAM_Cache_accDelay; // RQ
    Tick                SRAM_CBF_accDelay; // RRS
    Tick                SRAM_RIT_accDelay; // RRS
    uint64_t            prev_drain_cache_miss_val; // RQ
    uint64_t            drain_cache_miss_threshold; // RQ

    uint64_t            s_remapped_row_accessed;
    uint64_t            s_clean_install;
    uint64_t            s_only_unswap;
    uint64_t            s_clean_reswap;
    uint64_t            s_dirty_reswap;
    uint64_t            s_num_resets;
    uint64_t            s_num_accesses;
    uint64_t            s_move_to_qr;
    uint64_t            s_move_within_qr;
    uint64_t            s_move_to_qr_remove;
    uint64_t            s_move_within_qr_remove;
    uint64_t            s_drain_qr;
    uint64_t            s_btv_true_positives;
    uint64_t            s_btv_false_positives;
    uint64_t            s_btv_true_negatives;
    uint64_t            s_cache_hit;
    uint64_t            s_cache_partial_hit_orr_set;
    uint64_t            s_cache_partial_hit_orr_unset;
    uint64_t            s_cache_miss;
    uint64_t            s_cache_miss_entry_inserts;
    uint64_t            s_cache_miss_line_entry_inserts;
    uint64_t            s_cache_clean_evicts;
    uint64_t            s_cache_dirty_evicts;
    uint64_t            s_btv_residency[4];
    uint64_t            s_numLogRowAccesses[4];
    uint64_t            s_numRowAccesses_RH;
    uint64_t            s_QR_CumulativeOccupancy;
    uint64_t            s_randRowGenerationFailed;

    virtual Decision& access(Tick currTick, Addr addr, 
                                uint8_t bankID, uint8_t rankID, uint8_t channelID) = 0;
    virtual void reset() = 0;
};

class Randomized_Row_Swap : public RH_Mitigation {
private:
    enum SWAP_TYPE {
        CLEAN_INSTALL = 0x0,
        UNSWAP = 0x1,
        RESWAP = 0x2
    };

    typedef struct __RIT_Entry {
        bool            valid;
        bool            locked;
        Addr            origRowID;
        Addr            remappedRowID;
    } RIT_Entry;
    
    typedef struct __RIT {
        RIT_Entry       *tuples;
        uint32_t        *rowHasEntry;
        uint32_t        valid_tuples;
    } RIT;

    RIT                 ***tables;
    uint32_t            maxValidTuples;
    uint32_t            numTuples;

    Addr generateRandomRowAddr(Addr avoidRow, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    Addr getRemappedAddr(Addr addr, uint8_t bankID, uint8_t rankID, uint8_t channelID, bool isFuncLookup);
    void insertTuple(Addr origRowID, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void resetDecision();

public:
    Randomized_Row_Swap(uint32_t demandTuples, uint32_t numRows, uint32_t threshold,
                        uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels);
    Decision& access(Tick currTick, Addr addr, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void reset();
};

class Row_Quarantine : public RH_Mitigation {
private:
    enum MOVE_TYPE {
        MOVE_TO_QR = 0x0,
        MOVE_WITHIN_QR = 0x1,
        REMOVE = 0x2
    };

    typedef struct __Entry {
        bool            valid;
        Addr            value;
    } Entry;
    
    typedef struct __IndirectionTables {
        Entry           *row_To_QR_T;
        Entry           *QR_To_Row_T;
        uint32_t        nextFreeQR_ID;
        uint32_t        prevQRHead_ID;
        uint32_t        QRTail_ID;
    } IndirectionTables;

    uint32_t            *randRowID;
    IndirectionTables   ***indirection;
    uint32_t            numQREntries;

    uint32_t generateCBFID(Addr rowID, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void decodeAddr(Addr addr, DecodedAddr *mapping);
    bool insertRowInCache(Addr index, Addr &evicted_rowID, bool setDirty);
    bool removeRowFromCache(Addr index, Addr &evicted_rowID, Addr remove_rowID);
    void drainRow();
    bool removeRow(Addr QRT_id, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void quarantineRow(Addr rowID, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void addQCheckDelay(Addr rowID, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void resetDecision();

public: 
    Row_Quarantine(uint32_t __numQREntries, uint32_t numRows, uint32_t threshold,
                        uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels);
    Decision& access(Tick currTick, Addr addr, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void reset();
};

class BlockHammer : public RH_Mitigation {
private:
    Tick                BlackListThreshold;
    void resetDecision();

public:
    BlockHammer(uint32_t threshold, uint32_t __numRows, uint32_t __BLThreshold,
                        uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels);
    Decision& access(Tick currTick, Addr rowID, uint8_t bankID, uint8_t rankID, uint8_t channelID);
    void reset();
};

#endif 