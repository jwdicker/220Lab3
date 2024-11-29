#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"

#include "globals/assert.h"
#include "globals/utils.h"
#include "op.h"

#include "core.param.h"
#include "debug/debug.param.h"
#include "general.param.h"
#include "libs/hash_lib.h"
#include "libs/list_lib.h"
#include "memory/memory.param.h"
#include "prefetcher/pref.param.h"
#include "prefetcher/pref_common.h"
#include "prefetcher/pref_domino.h"
#include "prefetcher/pref_domino.param.h"
#include "statistics.h"

#include <vector>
#include <map>
#include <cassert>
#include <iostream>

using namespace std;
typedef std::map<Addr, uns> AddressPointerMap;
typedef std::map<Addr, uns> AddressTimerMap;


struct EIT_Entry {
    AddressPointerMap address_pointer_pair; // a [(B,P0), (C, P1), (F,P2)]
    AddressTimerMap   address_timer_pair;   // a [(B,T0), (C,T1)]
    uns timer;
    Addr most_recent_addr;

    uns most_recent_pointer;
    uns count;

    //constructor 
    EIT_Entry()
    {
        timer = 0;
        most_recent_addr = 0;
        count = 0;
        address_pointer_pair.clear();
        address_timer_pair.clear();
    }

    // from current_addr we get several pairs and get pointer from next_addr
    //  [(B,P0), (C, P1), (F,P2)]  C -> P1
    uns get_ghb_pointer(Addr next_addr)
    {
        if(address_pointer_pair.find(next_addr) != address_pointer_pair.end())
            return address_pointer_pair[next_addr];

        assert(address_pointer_pair.find(most_recent_addr) != address_pointer_pair.end());
        return address_pointer_pair[most_recent_addr];
    }

    //remove oldest
    void remove_oldest() {
        AddressTimerMap::iterator it = address_pointer_pair.begin();
        Addr replace_addr = it->first;
        uns oldest = it->second;
        for(AddressTimerMap::iterator it = address_timer_pair.begin(); it != address_timer_pair.end(); it++) 
        {
            if(oldest > it->second) {
                oldest = it->second;
                replace_addr = it->first;
            }
        }
        assert(address_pointer_pair.find(replace_addr) != address_pointer_pair.end());
        address_pointer_pair.erase(replace_addr);
        address_timer_pair.erase(replace_addr);
    }

    //insert new pair
    void update(Addr next_addr, uns pointer) {
        timer = cycle_count;

        if(address_pointer_pair.find(next_addr) == address_pointer_pair.end()) {
            if(address_pointer_pair.size() >= PREF_DOMINO_PAIR_N) {
                remove_oldest();
                assert(address_pointer_pair.size() <= PREF_DOMINO_PAIR_N);
                assert(address_timer_pair.size() <= PREF_DOMINO_PAIR_N);
            }
        }
        address_pointer_pair[next_addr] = pointer;
        address_timer_pair[next_addr] = timer;
        most_recent_addr = next_addr;
    }
};

struct Domino_prefetcher_t 
{
    HWP_Info* hwp_info;
    vector<Addr> GHB;
    vector<Addr> PointBuf;
    map<Addr, EIT_Entry> EIT;
    //use for replay
    Addr first_address;
    Addr second_address;
    //use for record
    Addr last_address;

    void init_domino_core(HWP* hwp) {
        hwp_info = hwp->hwp_info;
        hwp_info->enabled = TRUE;
        GHB.clear();
        EIT.clear();
        PointBuf.clear();
        first_address = 0;
        second_address = 0;
        last_address = 0;
    }


    void find_new_stream(uns8 proc_id, Addr lineAddr, Addr loadPC) {
        //first address is empty 
        //if(first_address == 0) {
            first_address = lineAddr;
            if(EIT.find(first_address) == EIT.end())
                return;
            uns start = EIT[first_address].most_recent_pointer;

            PointBuf.clear();
            for(uns i = 1; i < 32 && start + i < GHB.size(); i++) {
                PointBuf.push_back(GHB[start + i]);
            }

            uns maxpref = 16;
            size_t readcount = std::min(maxpref, static_cast<uns>(PointBuf.size()));
            for(size_t i = 0; i < readcount; i++) {
                Addr addr = PointBuf.front();
                PointBuf.erase(PointBuf.begin());
                Addr lineIndex     = addr >> LOG2(DCACHE_LINE_SIZE);
               if(pref_addto_ul1req_queue_set(proc_id, lineIndex, hwp_info->id,
                                      0, loadPC, 0, FALSE) == FALSE) // FIXME
                printf("error\n");
            }

        //}

        //Has first_addr and match second_addr

    }

    void pref_domino_replay(uns8 proc_id, Addr lineAddr, Addr loadPC, Flag is_hit) {

        /** if prefetch hit, then prefetch from poinbuf */

        /**cache miss */
        find_new_stream(proc_id, lineAddr, loadPC);
    }

    void pref_domino_record(Addr lineAddr) {
        //initial state: no history
        // if(last_address == 0) {
        //     last_address = lineAddr;
        //     GHB.push_back(lineAddr);
        // }
        GHB.push_back(lineAddr);
        if(EIT.find(lineAddr) == EIT.end())
            EIT[lineAddr] = EIT_Entry();
        
        if(EIT[lineAddr].count%16 == 0) {
            EIT[lineAddr].most_recent_pointer = GHB.size() - 1;
            EIT[lineAddr].count++;
        }

    }

    void pref_domino_train(uns8 proc_id, Addr lineAddr, Addr loadPC, Flag is_hit) {
        
        /**first replay then record */
        pref_domino_replay(proc_id, lineAddr, loadPC, is_hit);

       
        pref_domino_record(lineAddr);
    }
    
};

//every core has a domino prefetcher
std::vector<Domino_prefetcher_t> prefetchers;


void check_flag() 
{
    if(!PREF_DOMINO_ON)
        return;
}

void pref_domino_init(HWP* hwp) {
    check_flag();
    //target LLC
    prefetchers.resize(NUM_CORES);
    if(PREF_UL1_ON){
        for(uns i = 0; i < NUM_CORES; i++) {
            prefetchers[i].init_domino_core(hwp);
        }
    }
}


void pref_domino_ul1_prefhit(uns8 proc_id, Addr lineAddr, Addr loadPC, uns32 global_hist) {
    prefetchers[proc_id].pref_domino_train(proc_id, lineAddr, loadPC, TRUE);
}

void pref_domino_ul1_miss(uns8 proc_id, Addr lineAddr, Addr loadPC, uns32 global_hist) {
    prefetchers[proc_id].pref_domino_train(proc_id, lineAddr, loadPC, FALSE);
}