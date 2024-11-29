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
    AddressPointerMap address_pointer_pair; 
    AddressTimerMap   address_timer_pair;   
    uns count;
    uns most_recent_pointer;
    //constructor 
    EIT_Entry()
    {
        most_recent_pointer = 0;
        count = 0;
        address_pointer_pair.clear();
        address_timer_pair.clear();
    }

    // from current_addr we get several pairs and get pointer from next_addr
    //  [(B,P0), (C, P1), (F,P2)]  C -> P1
    uns get_ghb_pointer(Addr next_addr)
    {
            return address_pointer_pair[next_addr];

    }

    //insert new pair
    void update(Addr next_addr, uns pointer) {

        if(address_pointer_pair.find(next_addr) == address_pointer_pair.end()){

            address_pointer_pair[next_addr] = pointer;
            address_timer_pair[next_addr] = 1;
        }
            
        if(most_recent_pointer == 0)
            most_recent_pointer = pointer;

        if(address_timer_pair[next_addr] %4 == 0)
            address_pointer_pair[next_addr] = pointer;

        if(count %8 == 0)
            most_recent_pointer = pointer;
        count++;
        address_timer_pair[next_addr]++;

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


    void prefetch_from_poinbuf(uns8 proc_id, Addr loadPC) {
        uns maxpref = 8;
        size_t readcount = std::min(maxpref, static_cast<uns>(PointBuf.size()));
        for(size_t i = 0; i < readcount; i++) {
            Addr addr = PointBuf.front();
            PointBuf.erase(PointBuf.begin());
            Addr lineIndex     = addr >> LOG2(DCACHE_LINE_SIZE);
            if(pref_addto_ul1req_queue_set(proc_id, lineIndex, hwp_info->id,
                                      0, loadPC, 0, FALSE) == FALSE) 
            printf("error\n");
        }
    }

    void find_new_stream(uns8 proc_id, Addr lineAddr, Addr loadPC) {
        //first address is empty 
        if(first_address == 0) {
            first_address = lineAddr;
            //no match return
            if(EIT.find(first_address) == EIT.end())
                return;



            uns start = EIT[first_address].most_recent_pointer;

            PointBuf.clear();
            for(uns i = 1; i < 128 && start + i < GHB.size(); i++) {
                PointBuf.push_back(GHB[start + i]);
            }

            prefetch_from_poinbuf(proc_id, loadPC);

            return;
        }

        //Has first_addr and match second_addr
        second_address = lineAddr;

        //if first+second match
        if(EIT[first_address].address_pointer_pair.find(second_address) != EIT[first_address].address_pointer_pair.end()) {
            uns start = EIT[first_address].get_ghb_pointer(second_address);
            PointBuf.clear();
            for(uns i = 1; i < 128 && start + i < GHB.size(); i++) {
                PointBuf.push_back(GHB[start + i]);
            }
            first_address = lineAddr;
            prefetch_from_poinbuf(proc_id, loadPC);

            return;
        } else {
            first_address = lineAddr;
            second_address = 0;
            if(EIT.find(first_address) == EIT.end())
                return;

            uns start = EIT[first_address].most_recent_pointer;

            PointBuf.clear();
            for(uns i = 1; i < 128 && start + i < GHB.size(); i++) {
                PointBuf.push_back(GHB[start + i]);
            }

            prefetch_from_poinbuf(proc_id, loadPC);

            return;
        }
    }

    void pref_domino_replay(uns8 proc_id, Addr lineAddr, Addr loadPC, Flag is_hit) {

        /** if prefetch hit, then prefetch from poinbuf */
        if(is_hit == TRUE) {
            first_address = 0;
            prefetch_from_poinbuf(proc_id, loadPC);
            return;
        }

        /**cache miss */
        if(is_hit == FALSE)
            find_new_stream(proc_id, lineAddr, loadPC);
    }

    void pref_domino_record(Addr lineAddr) {
        //initial state: no history
        if(last_address == 0) {
            last_address = lineAddr;
            GHB.push_back(lineAddr);
            return;
        }

        GHB.push_back(lineAddr);
        if(EIT.find(last_address) == EIT.end()) {
            EIT[last_address] = EIT_Entry();
            EIT[last_address].update(lineAddr, GHB.size()-1);
            last_address = lineAddr;
            return;
        }

        EIT[last_address].update(lineAddr, GHB.size()-1);
        last_address = lineAddr;

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