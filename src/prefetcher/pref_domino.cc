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
#include "statistics.h"
#include "prefetcher/pref_domino.param.h"
#include <vector>
#include <map>
#include <cassert>
#include <iostream>


typedef std::map<Addr, uns> AddressPointerMap;
typedef std::map<Addr, uns> AddressTimerMap;


struct EIT_Entry {
    AddressPointerMap address_pointer_pair; // a [(B,P0), (C, P1), (F,P2)]
    AddressTimerMap   address_timer_pair;   // a [(B,T0), (C,T1)]
    uns timer;
    Addr most_recent_addr;

    //constructor 
    EIT_Entry()
    {
        timer = 0;
        most_recent_addr = 0;
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

