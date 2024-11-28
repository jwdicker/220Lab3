#ifndef __PREF_DOMINO_H__
#define __PREF_DOMINO_H__

#include "pref_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void pref_domino_init(HWP* hwp);
void pref_domino_ul1_prefhit(uns8 proc_id, Addr lineAddr, Addr loadPC, uns32 global_hist);
void pref_domino_ul1_miss(uns8 proc_id, Addr lineAddr, Addr loadPC, uns32 global_hist);
#ifdef __cplusplus
}
#endif

#endif // __PREF_DOMINO_H__