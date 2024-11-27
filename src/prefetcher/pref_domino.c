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

/*
   stride prefetcher : Stride prefetcher based on Abraham's ICS'04 paper
   - "Effective Stream-Based and Execution-Based Data Prefetching"

   Divides memory in regions and then does multi-stride prefetching
*/
/*
 * This prefetcher is implemented in a unified way where all cores share the same prefetcher instance.
 * As a result, the proc_id received as a parameter is ignored.
 */
/**************************************************************************************/
/* Macros */
#define DEBUG(args...) _DEBUG(DEBUG_PREF_AMPM, ##args)

void pref_ampm_init(HWP* hwp) {
  if(!PREF_AMPM_ON){
    return;     
  }

  init_ampm(hwp);
  int i;
  for(i=0; i<AMPM_PAGE_COUNT; i++){
    ampm_pages_ul1[i].page = 0;
    ampm_pages_ul1[i].lru = 0;
    int j;
    for(j=0; j<64; j++){
      ampm_pages_ul1[i].access_map[j] = 0;
      ampm_pages_ul1[i].pf_map[j] = 0;
	  }
  }
}

void init_ampm(HWP* hwp){
  hwp->hwp_info->enabled   = TRUE;
  hwp_ptr = hwp;
}
void pref_ampm_ul1_hit(uns8 proc_id, Addr lineAddr, Addr loadPC,
                         uns32 global_hist) {
  pref_ampm_train(lineAddr, loadPC, TRUE);
}

void pref_ampm_ul1_miss(uns8 proc_id, Addr lineAddr, Addr loadPC,
                          uns32 global_hist){
  pref_ampm_train(lineAddr, loadPC, FALSE);
}

void pref_ampm_train(Addr lineAddr, Addr loadPC, Flag is_hit) {
  ampm_page_t* ampm_pages;
  ampm_pages = ampm_pages_ul1;


  Addr cl_address = lineAddr>>6;
  Addr page = cl_address>>6;
  uns64 page_offset = cl_address&63;

  // check to see if we have a page hit
  int page_index = -1;
  int i;
  for(i=0; i<AMPM_PAGE_COUNT; i++){
    if(ampm_pages[i].page == page){
	    page_index = i;
	    break;
    }
  }

  if(page_index == -1){
    // the page was not found, so we must replace an old page with this new page
    // find the oldest page
    int lru_index = 0;
    unsigned long long int lru_cycle = ampm_pages[lru_index].lru;
    int i;
    for(i=0; i<AMPM_PAGE_COUNT; i++){
      if(ampm_pages[i].lru < lru_cycle){
          lru_index = i;
          lru_cycle = ampm_pages[lru_index].lru;
        }
	  }
    page_index = lru_index;

    // reset the oldest page
    ampm_pages[page_index].page = page;
    for(i=0; i<64; i++)
    {
      ampm_pages[page_index].access_map[i] = 0;
      ampm_pages[page_index].pf_map[i] = 0;
    }
  }

  // update LRU
  ampm_pages[page_index].lru = cycle_count;

  // mark the access map
  ampm_pages[page_index].access_map[page_offset] = 1;

  // positive prefetching
  int count_prefetches = 0;
  for(i=1; i<=16; i++){
    int check_index1 = page_offset - i;
    int check_index2 = page_offset - 2*i;
    int pf_index = page_offset + i;
    if(check_index2 < 0)      {
      break;
    }

    if(pf_index > 63){
      break;
    }

    if(count_prefetches >= PREFETCH_DEGREE){
	    break;
	  }

    if(ampm_pages[page_index].access_map[pf_index] == 1){
      // don't prefetch something that's already been demand accessed
      continue;
    }

    if(ampm_pages[page_index].pf_map[pf_index] == 1){
      // don't prefetch something that's alrady been prefetched
      continue;
    }

    if((ampm_pages[page_index].access_map[check_index1]==1) && (ampm_pages[page_index].access_map[check_index2]==1)){
	  // we found the stride repeated twice, so issue a prefetch
	    Addr pf_address = (page<<12)+(pf_index<<6);

      // check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
      // TODO
      pref_addto_ul1req_queue(0, pf_address >> 6, hwp_ptr->hwp_info->id);
 

      // mark the prefetched line so we don't prefetch it again
      ampm_pages[page_index].pf_map[pf_index] = 1;
      count_prefetches++;
    }
  }

  // negative prefetching
  count_prefetches = 0;
  for(i=1; i<=16; i++)
  {
    int check_index1 = page_offset + i;
    int check_index2 = page_offset + 2*i;
    int pf_index = page_offset - i;

    if(check_index2 > 63){
      break;
    }

    if(pf_index < 0){
      break;  
	  }

    if(count_prefetches >= PREFETCH_DEGREE){
      break;
    }

    if(ampm_pages[page_index].access_map[pf_index] == 1){
      // don't prefetch something that's already been demand accessed
      continue;
	  }

    if(ampm_pages[page_index].pf_map[pf_index] == 1){
      // don't prefetch something that's alrady been prefetched
      continue;
    }

    if((ampm_pages[page_index].access_map[check_index1]==1) && (ampm_pages[page_index].access_map[check_index2]==1)){
      // we found the stride repeated twice, so issue a prefetch
      Addr pf_address = (page<<12)+(pf_index<<6);

      // check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
      //TODO
      pref_addto_ul1req_queue(0, pf_address >> 6, hwp_ptr->hwp_info->id);

      // mark the prefetched line so we don't prefetch it again
      ampm_pages[page_index].pf_map[pf_index] = 1;
      count_prefetches++;
    }
  }
}
