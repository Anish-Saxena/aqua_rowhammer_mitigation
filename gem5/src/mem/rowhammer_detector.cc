#include "mem/rowhammer_detector.hh"

Misra_Gries::Misra_Gries(uint32_t __numEntries, uint32_t __threshold, uint32_t __numRows,
                        uint8_t __numBanks, uint8_t __numRanks, uint8_t __numChannels){
    trackers  = (Tracker ***) calloc (__numChannels, sizeof(Tracker **));
    for (int ii = 0; ii < __numChannels; ii++) {
        trackers[ii] = (Tracker **) calloc (__numRanks, sizeof(Tracker *));
        for (int jj = 0; jj < __numRanks; jj++) {
            trackers[ii][jj] = new Tracker[__numBanks];
            for (int kk = 0; kk < __numBanks; kk++) {
                trackers[ii][jj][kk].entries = (Tracker_Entry *) calloc (__numEntries, sizeof(Tracker_Entry));
                trackers[ii][jj][kk].rowHasEntry = (uint32_t *) calloc(__numRows, sizeof(uint32_t));
            }
        }
    }
    threshold = __threshold;
    numEntries = __numEntries;
    numBanks = __numBanks;
    numRanks = __numRanks;
    numChannels = __numChannels;
    numRows = __numRows;
    s_numUniqRows = 0;
}

void Misra_Gries::reset(){
    for (int ii = 0; ii < numChannels; ii++) {
        for (int jj = 0; jj < numRanks; jj++) {
            for (int kk = 0; kk < numBanks; kk++) {
                trackers[ii][jj][kk].s_num_reset++;
                trackers[ii][jj][kk].s_glob_spill_count += trackers[ii][jj][kk].spill_counter;
                trackers[ii][jj][kk].spill_counter = 0;
                for (int ll = 0; ll < numEntries; ll++) {
                    trackers[ii][jj][kk].entries[ll].valid = false;
                    trackers[ii][jj][kk].entries[ll].count = 0;
                    trackers[ii][jj][kk].entries[ll].addr = 0;
                }
                std::fill_n(trackers[ii][jj][kk].rowHasEntry, numRows, 0);
            }

        }
    }
}

bool Misra_Gries::access(Addr rowAddr, uint8_t bankID, uint8_t rankID, 
                            uint8_t channelID,  bool isFuncLookup = false) {

    Tracker *tracker = &(trackers[channelID][rankID][bankID]);
    if (isFuncLookup == false) tracker->s_num_access++;

    bool aggressor_detected = false;
    bool entry_found = false;

    if (tracker->rowHasEntry[rowAddr]) {
        uint32_t entryID = tracker->rowHasEntry[rowAddr]-1;
        if (!(tracker->entries[entryID].valid == true && 
                tracker->entries[entryID].addr == rowAddr)) {
            assert(0);
        }

        if (isFuncLookup == false) {
            tracker->entries[entryID].count++;
            if (tracker->entries[entryID].count % threshold == 0) {
                aggressor_detected = true;
                if (tracker->uniq_rows.insert(rowAddr).second) {
                    s_numUniqRows++;
                }
            }
        }
        entry_found = true;
    }

    if (entry_found == false && isFuncLookup == false) {
        for (uint32_t ii = 0; ii < numEntries; ii++) {
            if (tracker->entries[ii].count == tracker->spill_counter) {
                if (tracker->entries[ii].valid) {
                    tracker->rowHasEntry[tracker->entries[ii].addr] = 0;
                }
                tracker->entries[ii].addr = rowAddr;
                tracker->entries[ii].count++;
                tracker->entries[ii].valid = true;
                tracker->s_num_install++;
                tracker->rowHasEntry[rowAddr] = ii+1;
                if (tracker->entries[ii].count % threshold == 0) {
                    aggressor_detected = true;
                    if (tracker->uniq_rows.insert(rowAddr).second) {
                        s_numUniqRows++;
                    }
                }
                entry_found = true;
                break;
            }
            else if (tracker->entries[ii].valid == false) {
                assert(0);
            }
        }
        if (entry_found == false) {
            // Table is full and entries have value greater than spill counter
            tracker->spill_counter++;
        }
    }

    if(aggressor_detected == true && isFuncLookup == false) {
        tracker->s_aggressors++;
    }
    
    if (isFuncLookup == false) {
        return aggressor_detected;
    }
    else {
        return entry_found;
    }
}

void Misra_Gries::print_stats(){
    ;
}

