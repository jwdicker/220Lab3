/***************************************************************************************
 * File         : domino.h
 * Author       : Jacob Dickerman
 * Date         : 11.21.2024
 * Description  : Store branch count and branch misprediction count for each branch.
 *                Used to identify candidates for dual path prefetching into the uop cache.
 ***************************************************************************************/

#ifndef __DOMINO_H__
#define __DOMINO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "global_types.h"

// Domino structure. Manages the History Table, Index Table, and prefetching
typedef struct Domino
{
  // Continuous memory for both tables
  Addr* tables;

  // Enhanced Index Table (EIT)
  Addr* EIT_Start;  // Pointer to the start of the table
  int EIT_size;     // Size of the EIT

  // History Table (HT)
  Addr* HT_Start;   // Pointer to the start of the table
  int HT_size;      // Size of the HT
};

/* General functions for the Domino struct */
// Initializer
Domino* init_domino(int eit_size, int ht_size);

// Deconstructor
void delete_domino(Domino** d);


/* Functions for the EIT */
Addr* eit_index_domino(Addr a);
void eit_update_domino(Addr* a);

/* Functions for the HT */
void ht_insert_domino(Addr a);
Addr** ht_fetch_domino(Addr a);

#ifdef __cplusplus
}
#endif

#endif