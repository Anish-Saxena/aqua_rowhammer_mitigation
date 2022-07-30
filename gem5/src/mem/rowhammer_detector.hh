#ifndef RH_DETECTOR
#define RH_DETECTOR

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <time.h>       /* time */
#include "base/types.hh"
#include <unordered_set>

class RH_Detector {
public:
    virtual bool access(Addr addr, uint8_t bankID, uint8_t rankID, 
                uint8_t channelID, bool isFuncLookup = false) = 0;
    virtual void reset() = 0;
    virtual void print_stats() = 0;
    uint64_t            s_numUniqRows;
};

class Misra_Gries : public RH_Detector {
private:
    typedef struct __Tracker_Entry {
        bool            valid;
        Addr            addr;
        uint32_t        count;
    } Tracker_Entry;
    
    class Tracker {
    public:
        Tracker_Entry       *entries;
        uint32_t            spill_counter;
        std::unordered_set<Addr>  uniq_rows;
        uint32_t            *rowHasEntry;

        uint32_t            s_num_reset;        //-- how many times was the tracker reset
        uint32_t            s_glob_spill_count; //-- what is the total spill_count over time

        //---- Update below statistics in mgries_access() ----
        uint64_t            s_num_access;  //-- how many times was the tracker called
        uint64_t            s_num_install; //-- how many times did the tracker install rowIDs 
        uint64_t            s_aggressors; //-- how many times did the tracker flag an aggressor
    };

    Tracker             ***trackers;
    uint32_t            threshold;
    uint32_t            numEntries;
    uint32_t            numRows;
    uint8_t             numBanks;
    uint8_t             numRanks;
    uint8_t             numChannels;

public:
    Misra_Gries(uint32_t __numEntries, uint32_t threshold, uint32_t __numRows,
                uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels);
    bool access(Addr addr, uint8_t bankID, uint8_t rankID, uint8_t channelID, bool isFuncLookup);
    void reset();
    void print_stats();
};

#endif // MGRIES_H