#ifndef __PREF_DOMINO_H__
#define __PREF_DOMINO_H__

#include "pref_common.h"
#define AMPM_PAGE_COUNT 64
#define PREFETCH_DEGREE 2

// typedef struct ampm_page
// {
//   // page address
//   uns64 page;

//   // The access map itself.
//   // Each element is set when the corresponding cache line is accessed.
//   // The whole structure is analyzed to make prefetching decisions.
//   // While this is coded as an integer array, it is used conceptually as a single 64-bit vector.
//   int access_map[64];

//   // This map represents cache lines in this page that have already been prefetched.
//   // We will only prefetch lines that haven't already been either demand accessed or prefetched.
//   int pf_map[64];

//   // used for page replacement
//   uns64 lru;
// } ampm_page_t;

// ampm_page_t ampm_pages_ul1[AMPM_PAGE_COUNT];
// HWP* hwp_ptr;

typedef struct EIT_Entry










/*************************************************************/
/* HWP Interface */
void pref_ampm_init(HWP* hwp);

void pref_ampm_ul1_miss(uns8 proc_id, Addr lineAddr, Addr loadPC,
                          uns32 global_hist);
void pref_ampm_ul1_hit(uns8 proc_id, Addr lineAddr, Addr loadPC,
                         uns32 global_hist);
/*************************************************************/
/* Internal Function */
void init_ampm(HWP* hwp);
void pref_ampm_train(Addr lineAddr, Addr loadPC, Flag is_hit);

#endif 
