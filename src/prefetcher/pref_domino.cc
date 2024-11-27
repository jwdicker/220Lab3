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

#include <vector>
#include <map>
#include <cassert>
#include <iostream>


typedef std::map<Addr, uns> AddressPointerMap;

struct EIT_Entry {
    AddressPointerMap address_pointer_pair; // a [(B,P0), (C, P1), (F,P2)]
    uns timer;
    Addr most_recent_addr;
    //constructor 
    EIT_Entry()
    {
        timer = 0;
        address_pointer_pair.clear();
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

    //remove oldest if out of range
    void remove_oldest() {

    }

    //insert new pair
    void update(Addr next_addr, uns pointer) {

    }
}

